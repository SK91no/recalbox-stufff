//
// Created by bkg2k on 21/12/2019.
//
#pragma once

#include "Allocator.h"

/*!
@brief Optimized stack with auto-expansion when necessary.
*/
template<typename T> class Stack : private Allocator
{
  #define __GET(i) ((T*)Memory())[i]
  #define __OBJSZ sizeof(T)

  private:
    int fCount;

  public:
    Stack() : Allocator(__OBJSZ, 0, 4, false, true), fCount(0) {}
    explicit Stack(int capacity) : Allocator(__OBJSZ, capacity, 4, false, true), fCount(0) {}
    Stack(const Stack& source) : Allocator(source), fCount(source.fCount) {}
    Stack(Stack&& source) noexcept : Allocator(source), fCount(source.fCount) { source.fCount = 0; }
    Stack& operator = (const Stack& source) { if (&source != this) { Allocator::operator=(source); fCount = source.fCount; } return *this; }
    Stack& operator = (Stack&& source) noexcept { if (&source != this) { Allocator::operator=(source); source.fCount = 0; } return *this; }

    // Iterator - minimal implementation allowing straight for(:) loops
    class Iterator
    {
      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using reference = T&;

        explicit Iterator(T* d): mData(d) {}

        reference operator*() { return *mData; }
        Iterator& operator++() { ++mData; return *this; }
        Iterator& operator--() { --mData; return *this; }
        friend bool operator==(Iterator it1, Iterator it2) { return it1.mData == it2.mData; }
        friend bool operator!=(Iterator it1, Iterator it2) { return it1.mData != it2.mData; }
      private:
        T* mData;
    };

    Iterator begin() const { return Iterator(&__GET(0)); }

    Iterator end() const { return Iterator(&__GET(fCount)); }

    T& operator[] (int index) const
    {
      if ((unsigned int)index >= (unsigned int)(fCount))
      {
        LOG_AND_EXIT("Stack Index out of range!");
      }
      return __GET(index);
    }

    void Push(const T& item)
    {
      if (fCount >= Capacity()) Resize(fCount + 1);
      __GET(fCount++) = item;
    }

    T& Peek() const
    {
      if (fCount == 0) LOG_AND_EXIT("Popped an empty stack!");
      return __GET(fCount - 1);
    }

    T Pop()
    {
      if (fCount == 0) LOG_AND_EXIT("Popped an empty stack!");
      return __GET(--fCount);
    }

    T PopAt(int index)
    {
      if (index >= fCount) LOG_AND_EXIT("Popped an empty stack!");
      T popped = __GET(index);
      if (index < fCount - 1) memmove(&__GET(index), &__GET(index + 1), __OBJSZ * (fCount - index - 1));
      fCount--;
      return popped;
    }

    T Pop(const T& thisOne)
    {
      for(int i=fCount; --i>=0;)
        if (__GET(i) == thisOne) return PopAt(i);
      LOG_AND_EXIT("Popped an empty stack!");
      __builtin_unreachable();
    }

    Stack<T>& PopAndDelete()
    {
      if (fCount > 0) fCount--;
      return *this;
    }

    Stack<T>& PopAndDeleteAt(int index)
    {
      if (index < fCount)
      {
        if (index < fCount - 1) memmove(&__GET(index), &__GET(index + 1), __OBJSZ * (fCount - index - 1));
        fCount--;
      }
      return *this;
    }

    Stack<T>& PopAndDelete(const T& thisOne)
    {
      for(int i=fCount; --i>=0;)
        if (__GET(i) == thisOne) { Pop(i); break; }
      return *this;
    }

    bool Contains(const T& thisOne) const
    {
      for(int i=fCount; --i>=0;)
        if (__GET(i) == thisOne) return true;
      return false;
    }

    Stack<T>& Clear()
    {
      Resize(fCount = 0);
      return *this;
    }

    [[nodiscard]] int Count() const { return fCount; }

    [[nodiscard]] bool Empty() const { return fCount == 0; }

    Stack<T>& Reverse()
    {
      for(int h = fCount, l = -1; --h > ++l; )
      {
        T tmp = __GET(h);
        __GET(h) = __GET(l);
        __GET(l) = tmp;
      }
      return *this;
    }

  #undef __GET
  #undef __OBJSZ
};
