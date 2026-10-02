//
// Created by bkg2k on 02/06/23.
//

#include "WizardDidYouKnow.h"
#include <input/InputCompactEvent.h>

SimpleWizardBase::Move WizardDidYouKnow::OnKeyReceived(int page, const InputCompactEvent& event)
{
  // East (SNES A) = backward on all pages but the first
  if (event.EastReleased() && page > 0) return Move::Backward;
  // South (SNES B) = forward on all pages but the last
  if (event.SouthReleased() && page < (int)(mPagesTexts.size() - 1)) return Move::Foreward;
  // Start = close on the last page only
  if (event.StartReleased() && page == (int)(mPagesTexts.size() - 1)) return Move::Close;

  return Move::None;
}
