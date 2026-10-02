//
// Created by bkg2k on 25/07/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <utils/sync/ISyncMessageReceiver.h>
#include <utils/storage/Array.h>
#include <utils/os/system/Mutex.h>

class SyncMessageFactory : public StaticLifeCycleControler<SyncMessageFactory>
{
  public:
    /*!
     * @brief Default constructor
     */
    SyncMessageFactory()
      : StaticLifeCycleControler<SyncMessageFactory>("MessageFactory")
    {
    }

    /*!
     * @brief Destructor
     */
    ~SyncMessageFactory()
    {
      for(int i = mAvailableMessages.Count(); --i >= 0; ) delete mAvailableMessages[i];
      for(int i = mPendingMessages.Count(); --i >= 0; ) delete mPendingMessages[i];
    }

    /*!
     * @brief Register a new receiver for message
     * @param receiver New receiver
     * @return Identifier of message that will be sent to the given receiver
     */
    int Register(IUntypedSyncMessageReceiver& receiver)
    {
      Mutex::AutoLock locker(mRegisterLocker);
      // Try to get an empty slot
      for(int i = mIdentifierToReceivers.Count(); --i >= 0;)
        if (mIdentifierToReceivers[i] == nullptr)
        {
          mIdentifierToReceivers(i) = &receiver;
          { LOG(LogTrace) << "[SyncMessage] Registered ID " << i; }
          return i;
        }
      // Create a new slot
      mIdentifierToReceivers.Add(&receiver);
      { LOG(LogTrace) << "[SyncMessage] Registered ID " << mIdentifierToReceivers.Count() - 1; }
      return mIdentifierToReceivers.Count() - 1;
    }

    /*!
     * @brief Unregister a receiver from its identifier
     * @param identifier Identifier
     */
    void Unregister(int identifier)
    {
      Mutex::AutoLock locker(mRegisterLocker);
      // Seek & destroy
      if ((unsigned int)identifier < (unsigned int)mIdentifierToReceivers.Count())
      {
        if (mIdentifierToReceivers[identifier] == nullptr)
        { LOG(LogError) << "[SyncMessage] ID " << identifier << " already unregistered !"; }
        mIdentifierToReceivers(identifier) = nullptr;
        { LOG(LogTrace) << "[SyncMessage] UNRegistered ID " << identifier; }
      }
      else
      { LOG(LogDebug) << "[MessageFactory] Error unregistering identifier #" << identifier; }
    }

    //! Dispatch all pending messages
    void DispatchMessage()
    {
      // Copy pendings into a local list
      // Message to be dispatched are stored in the reverse order
      mPendingLocker.Lock();
      Array<UntypedSyncMessage*> pending(mPendingMessages.Count());
      for(int i = mPendingMessages.Count(); --i >= 0; )
        if (--mPendingMessages[i]->mFrameDelay < 0)
        {
          pending.Add(mPendingMessages[i]);
          mPendingMessages.Delete(i);
        }
      mPendingLocker.UnLock();

      // Dispatch
      {
        Mutex::AutoLock locker(mRegisterLocker);
        // Run through the pending messages in reverse order so that they are dispatched in original order
        for(int i = pending.Count(); --i >= 0; )
        {
          UntypedSyncMessage* message = pending[i];
          IUntypedSyncMessageReceiver* receiver = mIdentifierToReceivers[message->mIdentifier];
          if (receiver != nullptr) // In rare cases, sender may have already been destroyed
          {
            { LOG(LogTrace) << "[SyncMessage] Message to be sent to receiver registered on ID " << message->mIdentifier; }
            receiver->ReceiveUntypedSyncMessage(*message);
          }
        }
      }

      // Recycle
      {
        Mutex::AutoLock locker(mAvailableLocker);
        mAvailableMessages.MoveFrom(pending);
      }
    }

  private:
    //! Array of Receiver. Index of receiver are their associated message identifiers
    Array<IUntypedSyncMessageReceiver*> mIdentifierToReceivers;
    //! Available and in use
    Array<UntypedSyncMessage*> mAvailableMessages;
    //! Queue of pending messages
    Array<UntypedSyncMessage*> mPendingMessages;

    //! Available queue locker
    Mutex mAvailableLocker;
    //! Pending queue locker
    Mutex mPendingLocker;
    //! Registration locker
    Mutex mRegisterLocker;

    /*!
     * @brief Obtain a new message. Until the message is dispatched, it remains "in use"
     * @return New usable message
     */
    UntypedSyncMessage* Obtain()
    {
      {
        Mutex::AutoLock locker(mAvailableLocker);
        if (!mAvailableMessages.Empty())
        {
          UntypedSyncMessage* message = mAvailableMessages.Last();
          mAvailableMessages.DeleteLast();
          return message;
        }
      }

      #ifdef DEBUG
      static int messageCounter = 0;
      { LOG(LogTrace) << "[MessageFactory] Created message #" << ++messageCounter; }
      #endif

      return new UntypedSyncMessage();
    }

    /*!
     * @brief Push a new message
     * @param message Message
     * @param identifier Message identifier
     */
    void Push(UntypedSyncMessage* message, int identifier)
    {
      {
        Mutex::AutoLock locker(mAvailableLocker);
        if (mAvailableMessages.Contains(message))
        {
          { LOG(LogError) << "[MessageFactory] Trying to resend a reycled message! Aborted."; }
          return;
        }
      }
      {
        Mutex::AutoLock locker(mPendingLocker);
        if (!mPendingMessages.Contains(message))
        {
          message->mIdentifier = identifier;
          mPendingMessages.Add(message);
        }
      }
    }

    //! Allow sender to use private methods
    template<IsPodMax T> friend class SyncMessageSender;
};
