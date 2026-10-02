#include <utils/locale/LocaleHelper.h>
#include <guis/GuiInputConfig.h>

#include "GuiMsgBoxScroll.h"
#include "IDeviceConfigured.h"
#include "hardware/RGPProxy.h"
#include "input/InputEvent.h"
#include "input/InputManager.h"

namespace { constexpr int sProxyWaitTimeoutMs = 5000; constexpr int sProxyPollIntervalMs = 100; }

HashMap<PadTypes::Type, GuiInputConfig::PadImageDescriptor>& GuiInputConfig::GetPadDefinitions()
{
  static HashMap<PadTypes::Type, PadImageDescriptor> sPadDefinitions
  {
  // 8bitDo
  {
    PadTypes::HeightBitdo,
    {
      .PadAboveFrame             = Path(":/pads/8bitdo/manette_8bitdo.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(":/pads/8bitdo/joy_left_neutral.svg"),
      .JoystickRightNeutral      = Path(":/pads/8bitdo/joy_right_neutral.svg"),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(":/pads/8bitdo/joy_left_left_up_active.svg"),
      .JoystickLeftUpRight       = Path(":/pads/8bitdo/joy_left_right_up_active.svg"),
      .JoystickLeftDownLeft      = Path(":/pads/8bitdo/joy_left_left_down_active.svg"),
      .JoystickLeftDownRight     = Path(":/pads/8bitdo/joy_left_right_down_active.svg"),
      .JoystickRightUpLeft       = Path(":/pads/8bitdo/joy_right_left_up_active.svg"),
      .JoystickRightUpRight      = Path(":/pads/8bitdo/joy_right_right_up_active.svg"),
      .JoystickRightDownLeft     = Path(":/pads/8bitdo/joy_right_left_down_active.svg"),
      .JoystickRightDownRight    = Path(":/pads/8bitdo/joy_right_right_down_active.svg"),
      .HatTop                    = { .mPath=Path(":/pads/8bitdo/hat_up_active.svg"),          .mHint=_("DPAD UP"),            .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/8bitdo/hat_right_active.svg"),       .mHint=_("DPAD RIGHT"),         .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/8bitdo/hat_down_active.svg"),        .mHint=_("DPAD DOWN"),          .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/8bitdo/hat_left_active.svg"),        .mHint=_("DPAD LEFT"),          .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(":/pads/8bitdo/joy_left_up_active.svg"),     .mHint=_("LEFT STICK UP"),      .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(":/pads/8bitdo/joy_left_right_active.svg"),  .mHint=_("LEFT STICK RIGHT"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(":/pads/8bitdo/joy_left_down_active.svg"),   .mHint=_("LEFT STICK DOWN"),    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(":/pads/8bitdo/joy_left_left_active.svg"),   .mHint=_("LEFT STICK LEFT"),    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(":/pads/8bitdo/joy_right_up_active.svg"),    .mHint=_("RIGHT STICK UP"),     .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(":/pads/8bitdo/joy_right_right_active.svg"), .mHint=_("RIGHT STICK RIGHT"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(":/pads/8bitdo/joy_right_down_active.svg"),  .mHint=_("RIGHT STICK DOWN"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(":/pads/8bitdo/joy_right_left_active.svg"),  .mHint=_("RIGHT STICK LEFT"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/8bitdo/btn_a_active.svg"),           .mHint="A",                  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/8bitdo/btn_b_active.svg"),           .mHint="B",                  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(":/pads/8bitdo/btn_x_active.svg"),           .mHint="X",                  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/8bitdo/btn_y_active.svg"),           .mHint="Y",                  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(), .mHint="Z", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/8bitdo/btn_l1_active.svg"),          .mHint="L1",                 .mHintLong=_("L1 (Left Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/8bitdo/btn_r1_active.svg"),          .mHint="R1",                 .mHintLong=_("R1 (Right Shoulder)"),       .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(":/pads/8bitdo/btn_l2_active.svg"),          .mHint="L2",                 .mHintLong=_("L2 (Left Trigger)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(":/pads/8bitdo/btn_r2_active.svg"),          .mHint="R2",                 .mHintLong=_("R2 (Right Trigger)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(":/pads/8bitdo/joy_l3_active.svg"),          .mHint="L3",                 .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(":/pads/8bitdo/joy_r3_active.svg"),          .mHint="R3",                 .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/8bitdo/btn_start_active.svg"),       .mHint="START",              .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/8bitdo/btn_select_active.svg"),      .mHint="SELECT",             .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonHotkey              = { .mPath=Path(":/pads/8bitdo/btn_hotkey_active.svg"),      .mHint=String("**").Append(_("HEART")).Append("**\n").Append(_("Skip if absent")),              .mHintLong=String("**").Append(_("HEART (Hotkey)")).Append("**\n").Append(_("Skip if physically absent")),            .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // XBox
  {
    PadTypes::XBox,
    {
      .PadAboveFrame             = Path(":/pads/xbox/manette_xbox.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(":/pads/xbox/joystick_left_neutral.svg"),
      .JoystickRightNeutral      = Path(":/pads/xbox/joystick_right_neutral.svg"),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(":/pads/xbox/joystick_left_left_up_active.svg"),
      .JoystickLeftUpRight       = Path(":/pads/xbox/joystick_left_right_up_active.svg"),
      .JoystickLeftDownLeft      = Path(":/pads/xbox/joystick_left_left_down_active.svg"),
      .JoystickLeftDownRight     = Path(":/pads/xbox/joystick_left_right_down_active.svg"),
      .JoystickRightUpLeft       = Path(":/pads/xbox/joystick_right_left_up_active.svg"),
      .JoystickRightUpRight      = Path(":/pads/xbox/joystick_right_right_up_active.svg"),
      .JoystickRightDownLeft     = Path(":/pads/xbox/joystick_right_left_down_active.svg"),
      .JoystickRightDownRight    = Path(":/pads/xbox/joystick_right_right_down_active.svg"),
      .HatTop                    = { .mPath=Path(":/pads/xbox/hat_up_active.svg"),               .mHint=_("DPAD UP"),           .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/xbox/hat_right_active.svg"),            .mHint=_("DPAD RIGHT"),        .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/xbox/hat_down_active.svg"),             .mHint=_("DPAD DOWN"),         .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/xbox/hat_left_active.svg"),             .mHint=_("DPAD LEFT"),         .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(":/pads/xbox/joystick_left_up_active.svg"),     .mHint=_("LEFT STICK UP"),     .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(":/pads/xbox/joystick_left_right_active.svg"),  .mHint=_("LEFT STICK RIGHT"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(":/pads/xbox/joystick_left_down_active.svg"),   .mHint=_("LEFT STICK DOWN"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(":/pads/xbox/joystick_left_left_active.svg"),   .mHint=_("LEFT STICK LEFT"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(":/pads/xbox/joystick_right_up_active.svg"),    .mHint=_("RIGHT STICK UP"),    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(":/pads/xbox/joystick_right_right_active.svg"), .mHint=_("RIGHT STICK RIGHT"), .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(":/pads/xbox/joystick_right_down_active.svg"),  .mHint=_("RIGHT STICK DOWN"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(":/pads/xbox/joystick_right_left_active.svg"),  .mHint=_("RIGHT STICK LEFT"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                = { .mPath=Path(":/pads/xbox/btn_a_active.svg"),                .mHint="A",                    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth               = { .mPath=Path(":/pads/xbox/btn_b_active.svg"),                .mHint="B",                    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime           = { .mPath=Path(),                                              .mHint="C",                    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth               = { .mPath=Path(":/pads/xbox/btn_x_active.svg"),                .mHint="X",                    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                = { .mPath=Path(":/pads/xbox/btn_y_active.svg"),                .mHint="Y",                    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime          = { .mPath=Path(),                                              .mHint="Z",                    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/xbox/btn_lb_active.svg"),               .mHint="LB",                   .mHintLong=_("LB (Left Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/xbox/btn_rb_active.svg"),               .mHint="RB",                   .mHintLong=_("RB (Right Shoulder)"),       .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(":/pads/xbox/btn_lt_active.svg"),               .mHint="LT",                   .mHintLong=_("LT (Left Trigger)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(":/pads/xbox/btn_rt_active.svg"),               .mHint="RT",                   .mHintLong=_("RT (Right Trigger)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(":/pads/xbox/btn_l3_active.svg")     ,          .mHint="L3",                   .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(":/pads/xbox/btn_r3_active.svg"),               .mHint="R3",                   .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/xbox/bnt_start_active.svg"),            .mHint="MENU",                 .mHintLong=_("MENU (Start)"),              .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/xbox/btn_select_active.svg"),           .mHint="VIEW",                 .mHintLong=_("VIEW (Select)"),             .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonHotkey              = { .mPath=Path(":/pads/xbox/btn_home_active.svg"),             .mHint=String("**").Append(_("GUIDE")).Append("**\n").Append(_("Skip if absent")),              .mHintLong=String("**").Append(_("GUIDE (Hotkey)")).Append("**\n").Append(_("Skip if physically absent")),            .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Playstation
  {
    PadTypes::Playstation,
    {
      .PadAboveFrame             = Path(":/pads/sony/manette_ps4.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(":/pads/sony/joystick_left_neutral.svg"),
      .JoystickRightNeutral      = Path(":/pads/sony/joystick_right_neutral.svg"),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(":/pads/sony/joystick_left_left_up_active.svg"),
      .JoystickLeftUpRight       = Path(":/pads/sony/joystick_left_right_up_active.svg"),
      .JoystickLeftDownLeft      = Path(":/pads/sony/joystick_left_left_down_active.svg"),
      .JoystickLeftDownRight     = Path(":/pads/sony/joystick_left_right_down_active.svg"),
      .JoystickRightUpLeft       = Path(":/pads/sony/joystick_right_left_up_active.svg"),
      .JoystickRightUpRight      = Path(":/pads/sony/joystick_right_right_up_active.svg"),
      .JoystickRightDownLeft     = Path(":/pads/sony/joystick_right_left_down_active.svg"),
      .JoystickRightDownRight    = Path(":/pads/sony/joystick_right_right_down_active.svg"),
      .HatTop                    = { .mPath=Path(":/pads/sony/hat_up_active.svg"),               .mHint=_("DPAD UP"),           .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/sony/hat_right_active.svg"),            .mHint=_("DPAD RIGHT"),        .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/sony/hat_down_active.svg"),             .mHint=_("DPAD DOWN"),         .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/sony/hat_left_active.svg"),             .mHint=_("DPAD LEFT"),         .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(":/pads/sony/joystick_left_up_active.svg"),     .mHint=_("LEFT STICK UP"),     .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(":/pads/sony/joystick_left_right_active.svg"),  .mHint=_("LEFT STICK RIGHT"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(":/pads/sony/joystick_left_down_active.svg"),   .mHint=_("LEFT STICK DOWN"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(":/pads/sony/joystick_left_left_active.svg"),   .mHint=_("LEFT STICK LEFT"),   .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(":/pads/sony/joystick_right_up_active.svg"),    .mHint=_("RIGHT STICK UP"),    .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(":/pads/sony/joystick_right_right_active.svg"), .mHint=_("RIGHT STICK RIGHT"), .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(":/pads/sony/joystick_right_down_active.svg"),  .mHint=_("RIGHT STICK DOWN"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(":/pads/sony/joystick_right_left_active.svg"),  .mHint=_("RIGHT STICK LEFT"),  .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/sony/btn_round_active.svg"),            .mHint=_("CIRCLE"),            .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/sony/btn_cross_active.svg"),            .mHint=_("CROSS"),             .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(":/pads/sony/btn_triangle_active.svg"),         .mHint=_("TRIANGLE"),          .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/sony/btn_square_active.svg"),           .mHint=_("SQUARE"),            .mHintLong=String::Empty,               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(), .mHint="Z", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/sony/btn_l1_active.svg"),               .mHint="L1",                .mHintLong=_("L1 (Left Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/sony/btn_r1_active.svg"),               .mHint="R1",                .mHintLong=_("R1 (Right Shoulder)"),       .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(":/pads/sony/btn_l2_active.svg"),               .mHint="L2",                .mHintLong=_("L2 (Left Trigger)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(":/pads/sony/btn_r2_active.svg"),               .mHint="R2",                .mHintLong=_("R2 (Right Trigger)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(":/pads/sony/joystick_l3_active.svg"),          .mHint="L3",                .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(":/pads/sony/joystick_r3_active.svg"),          .mHint="R3",                .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/sony/btn_option_active.svg"),           .mHint="OPTIONS",           .mHintLong=_("OPTIONS (Start)"),           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/sony/btn_share_active.svg"),            .mHint="SHARE",             .mHintLong=_("SHARE (Select)"),            .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonHotkey              = { .mPath=Path(":/pads/sony/btn_home_active.svg"),             .mHint=String("**").Append(_("PS BUTTON")).Append("**\n").Append(_("Skip if absent")),         .mHintLong=String("**").Append(_("PS BUTTON (Hotkey)")).Append("**\n").Append(_("Skip if physically absent")),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Arcade 8 buttons
  {
    PadTypes::Arcade8,
    {
      .PadAboveFrame             = Path(":/pads/arcade8/arcade_stick.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(":/pads/arcade8/joy_neutral.svg"),
      .JoystickLeftNeutral       = Path(),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(":/pads/arcade8/joy_up_left_active.svg"),
      .DPadUpRight               = Path(":/pads/arcade8/joy_up_right_active.svg"),
      .DPadDownLeft              = Path(":/pads/arcade8/joy_down_left_active.svg"),
      .DPadDownRight             = Path(":/pads/arcade8/joy_down_right_active.svg"),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/arcade8/joy_up_active.svg"),     .mHint=_("STICK UP"),          .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/arcade8/joy_right_active.svg"),  .mHint=_("STICK RIGHT"),       .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/arcade8/joy_down_active.svg"),   .mHint=_("STICK DOWN"),        .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/arcade8/joy_left_active.svg"),   .mHint=_("STICK LEFT"),        .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(),                                       .mHint=_("LEFT STICK UP"),     .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(),                                       .mHint=_("LEFT STICK RIGHT"),  .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(),                                       .mHint=_("LEFT STICK DOWN"),   .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(),                                       .mHint=_("LEFT STICK LEFT"),   .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(),                                       .mHint=_("RIGHT STICK UP"),    .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(),                                       .mHint=_("RIGHT STICK RIGHT"), .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(),                                       .mHint=_("RIGHT STICK DOWN"),  .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(),                                       .mHint=_("RIGHT STICK LEFT"),  .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/arcade8/btn_2_active.svg"),      .mHint=_("TOP ROW 2"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/arcade8/btn_1_active.svg"),      .mHint=_("TOP ROW 1"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(":/pads/arcade8/btn_4_active.svg"),      .mHint=_("TOP ROW 4"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/arcade8/btn_3_active.svg"),      .mHint=_("TOP ROW 3"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(), .mHint="Z", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/arcade8/btn_5_active.svg"),      .mHint=_("BOTTOM ROW 1"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/arcade8/btn_6_active.svg"),      .mHint=_("BOTTOM ROW 2"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(":/pads/arcade8/btn_7_active.svg"),      .mHint=_("BOTTOM ROW 3"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(":/pads/arcade8/btn_8_active.svg"),      .mHint=_("BOTTOM ROW 4"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                       .mHint="L3",                .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                       .mHint="R3",                .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/arcade8/btn_start_active.svg"),  .mHint="START",             .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/arcade8/btn_select_active.svg"), .mHint=String("**SELECT**\n").Append(_("Skip if absent")),            .mHintLong=String("**SELECT**\n").Append(_("Skip if physically absent")), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonHotkey              = { .mPath=Path(":/pads/arcade8/btn_hotkey_active.svg"), .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),            .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Arcade 6 buttons
  {
    PadTypes::Arcade6,
    {
      .PadAboveFrame             = Path(":/pads/arcade6/arcade_stick.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(":/pads/arcade6/joy_neutral.svg"),
      .JoystickLeftNeutral       = Path(),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(":/pads/arcade6/joy_up_left_active.svg"),
      .DPadUpRight               = Path(":/pads/arcade6/joy_up_right_active.svg"),
      .DPadDownLeft              = Path(":/pads/arcade6/joy_down_leftt_active.svg"),
      .DPadDownRight             = Path(":/pads/arcade6/joy_down_right_active.svg"),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/arcade6/joy_up_active.svg"),     .mHint=_("STICK UP"),          .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/arcade6/joy_right_active.svg"),  .mHint=_("STICK RIGHT"),       .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/arcade6/joy_down_active.svg"),   .mHint=_("STICK DOWN"),        .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/arcade6/joy_left_active.svg"),   .mHint=_("STICK LEFT"),        .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(),                                       .mHint=_("LEFT STICK UP"),     .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(),                                       .mHint=_("LEFT STICK RIGHT"),  .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(),                                       .mHint=_("LEFT STICK DOWN"),   .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(),                                       .mHint=_("LEFT STICK LEFT"),   .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(),                                       .mHint=_("RIGHT STICK UP"),    .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(),                                       .mHint=_("RIGHT STICK RIGHT"), .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(),                                       .mHint=_("RIGHT STICK DOWN"),  .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(),                                       .mHint=_("RIGHT STICK LEFT"),  .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/arcade6/btn_2_active.svg"),      .mHint=_("TOP ROW 2"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/arcade6/btn_1_active.svg"),      .mHint=_("TOP ROW 1"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(":/pads/arcade6/btn_4_active.svg"),      .mHint=_("BOTTOM ROW 1"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/arcade6/btn_3_active.svg"),      .mHint=_("TOP ROW 3"),         .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(), .mHint="Z", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/arcade6/btn_5_active.svg"),      .mHint=_("BOTTOM ROW 2"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/arcade6/btn_6_active.svg"),      .mHint=_("BOTTOM ROW 3"),      .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(),                                       .mHint="L2",                .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(),                                       .mHint="R2",                .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                       .mHint="L3",                .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                       .mHint="R3",                .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/arcade6/btn_start_active.svg"),  .mHint="START",             .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/arcade6/btn_select_active.svg"), .mHint=String("**SELECT**\n").Append(_("Skip if absent")),            .mHintLong=String("**SELECT**\n").Append(_("Skip if physically absent")), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonHotkey              = { .mPath=Path(":/pads/arcade6/btn_hotkey_active.svg"), .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),            .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Arcade 4 buttons
  {
    PadTypes::Arcade4,
    {
      .PadAboveFrame             = Path(":/pads/arcade4/arcade_stick.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(":/pads/arcade4/joy_active.svg"),
      .JoystickLeftNeutral       = Path(),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(":/pads/arcade4/joy_up_left_active.svg"),
      .DPadUpRight               = Path(":/pads/arcade4/joy_up_right_active.svg"),
      .DPadDownLeft              = Path(":/pads/arcade4/joy_down_leftt_active.svg"),
      .DPadDownRight             = Path(":/pads/arcade4/joy_down_right_active.svg"),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/arcade4/joy_up_active.svg"),     .mHint=_("STICK UP"),          .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/arcade4/joy_right_active.svg"),  .mHint=_("STICK RIGHT"),       .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/arcade4/joy_down_active.svg"),   .mHint=_("STICK DOWN"),        .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/arcade4/joy_left_active.svg"),   .mHint=_("STICK LEFT"),        .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(),                                       .mHint=_("LEFT STICK UP"),     .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(),                                       .mHint=_("LEFT STICK RIGHT"),  .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(),                                       .mHint=_("LEFT STICK DOWN"),   .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(),                                       .mHint=_("LEFT STICK LEFT"),   .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(),                                       .mHint=_("RIGHT STICK UP"),    .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(),                                       .mHint=_("RIGHT STICK RIGHT"), .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(),                                       .mHint=_("RIGHT STICK DOWN"),  .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(),                                       .mHint=_("RIGHT STICK LEFT"),  .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/arcade4/btn_2_active.svg"),      .mHint=_("BUTTON 2"),          .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/arcade4/btn_1_active.svg"),      .mHint=_("BUTTON 1"),          .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(":/pads/arcade4/btn_4_active.svg"),      .mHint=_("BUTTON 4"),          .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/arcade4/btn_3_active.svg"),      .mHint=_("BUTTON 3"),          .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(), .mHint="Z", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(),                                       .mHint="L1",                .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(),                                       .mHint="R1",                .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(),                                       .mHint="L2",                .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(),                                       .mHint="R2",                .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                       .mHint="L3",                .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                       .mHint="R3",                .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/arcade4/btn_start_active.svg"),  .mHint="START",             .mHintLong=String::Empty,           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/arcade4/btn_select_active.svg"), .mHint=String("**SELECT**\n").Append(_("Skip if absent")),            .mHintLong=String("**SELECT**\n").Append(_("Skip if physically absent")),           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonHotkey              = { .mPath=Path(":/pads/arcade4/btn_hotkey_active.svg"), .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),            .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")),           .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Gamecube
  {
    PadTypes::Gamecube,
    {
      .PadAboveFrame             = Path(":/pads/gc/manette_gamecube.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(":/pads/gc/joy_center.svg"),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/gc/hat_up_active.svg"),                .mHint=_("DPAD UP"),            .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/gc/hat_right_active.svg"),             .mHint=_("DPAD RIGHT"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/gc/hat_down_active.svg"),              .mHint=_("DPAD DOWN"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/gc/hat_left_active.svg"),              .mHint=_("DPAD LEFT"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(":/pads/gc/joy_up_active.svg"),                .mHint=_("MAIN STICK UP"),      .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(":/pads/gc/joy_right_active.svg"),             .mHint=_("MAIN STICK RIGHT"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(":/pads/gc/joy_down_active.svg"),              .mHint=_("MAIN STICK DOWN"),    .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(":/pads/gc/joy_left_active.svg"),              .mHint=_("MAIN STICK LEFT"),    .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(":/pads/gc/btn_action_up_active.svg"),         .mHint=_("C-STICK UP"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(":/pads/gc/btn_action_right_active.svg"),      .mHint=_("C-STICK RIGHT"),      .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(":/pads/gc/btn_action_down_active.svg"),       .mHint=_("C-STICK DOWN"),       .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(":/pads/gc/btn_action_left_active.svg"),       .mHint=_("C-STICK LEFT"),       .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/gc/btn_a_active.svg"),                 .mHint="A",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/gc/btn_b_active.svg"),                 .mHint="B",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(":/pads/gc/btn_x_active.svg"),                 .mHint="X",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/gc/btn_y_active.svg"),                 .mHint="Y",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(), .mHint="Z", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/gc/btn_l_active.svg"),                 .mHint="L",                     .mHintLong=_("L (Left Shoulder)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/gc/btn_r_active.svg"),                 .mHint="R",                     .mHintLong=_("R (Right Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(":/pads/gc/btn_z_active.svg"),                 .mHint="Z",                     .mHintLong=_("Z (Trigger)"),               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(),                                             .mHint="R2",                    .mHintLong=_("R2 (Right Trigger)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                             .mHint="L3",                    .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                             .mHint="R3",                    .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/gc/btn_start_active.svg"),             .mHint="START",              .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(),                                             .mHint=String("**SELECT**\n").Append(_("Skip if absent")),             .mHintLong=String("**SELECT**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonHotkey              = { .mPath=Path(),                                             .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),             .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Megadrive 6 buttons
  {
    PadTypes::Megadrive6,
    {
      .PadAboveFrame             = Path(":/pads/megadrive6/manette_megadrive.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/megadrive6/Hat_up_active.svg"),        .mHint=_("DPAD UP"),            .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/megadrive6/Hat_right_active.svg"),     .mHint=_("DPAD RIGHT"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/megadrive6/Hat_down_active.svg"),      .mHint=_("DPAD DOWN"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/megadrive6/Hat_left_active.svg"),      .mHint=_("DPAD LEFT"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(),                                             .mHint=_("LEFT STICK UP"),      .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(),                                             .mHint=_("LEFT STICK RIGHT"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(),                                             .mHint=_("LEFT STICK DOWN"),    .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(),                                             .mHint=_("LEFT STICK LEFT"),    .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(),                                             .mHint=_("RIGHT STICK UP"),     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(),                                             .mHint=_("RIGHT STICK RIGHT"),  .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(),                                             .mHint=_("RIGHT STICK DOWN"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(),                                             .mHint=_("RIGHT STICK LEFT"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/megadrive6/btn_b_active.svg"),         .mHint="A",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/megadrive6/btn_a_active.svg"),         .mHint="B",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(":/pads/megadrive6/btn_c_active.svg"),         .mHint="C",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonNorth                   = { .mPath=Path(":/pads/megadrive6/btn_y_active.svg"),         .mHint="X",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(":/pads/megadrive6/btn_x_active.svg"),         .mHint="Y",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(":/pads/megadrive6/btn_z_active.svg"),         .mHint="Z",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(),                                             .mHint="L1",                    .mHintLong=_("L1 (Left Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(),                                             .mHint="R1",                    .mHintLong=_("R1 (Right Shoulder)"),       .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(":/pads/megadrive6/btn_l_active.svg"),         .mHint="L",                     .mHintLong=_("L (Left Shoulder)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(":/pads/megadrive6/btn_r_active.svg"),         .mHint="R",                     .mHintLong=_("R (Right Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                             .mHint="L3",                    .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                             .mHint="R3",                    .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/megadrive6/btn_start_active.svg"),     .mHint="START",              .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/megadrive6/btn_select_active.svg"),    .mHint="SELECT",             .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonHotkey              = { .mPath=Path(),                                             .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),             .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // Megadrive 3 buttons (only the manette image is available, no per-item SVG)
  {
    PadTypes::Megadrive3,
    {
      .PadAboveFrame             = Path(":/pads/megadrive3/manette_megadrive_3.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/megadrive3/hat_up_active.svg"),         .mHint=_("DPAD UP"),            .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/megadrive3/hat_right_active.svg"),      .mHint=_("DPAD RIGHT"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/megadrive3/hat_down_active.svg"),       .mHint=_("DPAD DOWN"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/megadrive3/hat_left_active.svg"),       .mHint=_("DPAD LEFT"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(),                                             .mHint=_("LEFT STICK UP"),      .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(),                                             .mHint=_("LEFT STICK RIGHT"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(),                                             .mHint=_("LEFT STICK DOWN"),    .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(),                                             .mHint=_("LEFT STICK LEFT"),    .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(),                                             .mHint=_("RIGHT STICK UP"),     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(),                                             .mHint=_("RIGHT STICK RIGHT"),  .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(),                                             .mHint=_("RIGHT STICK DOWN"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(),                                             .mHint=_("RIGHT STICK LEFT"),   .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/megadrive3/btn_b_active.svg"),          .mHint="A",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/megadrive3/btn_a_active.svg"),          .mHint="B",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(":/pads/megadrive3/btn_c_active.svg"),          .mHint="C",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonNorth                   = { .mPath=Path(),                                             .mHint="X",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(),                                             .mHint="Y",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(),                                             .mHint="Z",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(),                                             .mHint="L1",                    .mHintLong=_("L1 (Left Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(),                                             .mHint="R1",                    .mHintLong=_("R1 (Right Shoulder)"),       .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(),                                             .mHint="L2",                    .mHintLong=_("L2 (Left Trigger)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(),                                             .mHint="R2",                    .mHintLong=_("R2 (Right Trigger)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                             .mHint="L3",                    .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                             .mHint="R3",                    .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/megadrive3/btn_start_active.svg"),      .mHint="START",              .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(":/pads/megadrive3/btn_select_active.svg"),     .mHint=String("**SELECT**\n").Append(_("Skip if absent")),             .mHintLong=String("**SELECT**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonHotkey              = { .mPath=Path(),                                             .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),             .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  // N64
  {
    PadTypes::n64,
    {
      .PadAboveFrame             = Path(":/pads/n64/manette_n64.svg"),
      .PadBelowFrame             = Path(),
      .DPadNeutral               = Path(),
      .JoystickLeftNeutral       = Path(),
      .JoystickRightNeutral      = Path(),
      .DPadUpLeft                = Path(),
      .DPadUpRight               = Path(),
      .DPadDownLeft              = Path(),
      .DPadDownRight             = Path(),
      .JoystickLeftUpLeft        = Path(),
      .JoystickLeftUpRight       = Path(),
      .JoystickLeftDownLeft      = Path(),
      .JoystickLeftDownRight     = Path(),
      .JoystickRightUpLeft       = Path(),
      .JoystickRightUpRight      = Path(),
      .JoystickRightDownLeft     = Path(),
      .JoystickRightDownRight    = Path(),
      .HatTop                    = { .mPath=Path(":/pads/n64/hat_up_active.svg"),               .mHint=_("DPAD UP"),            .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatRight                  = { .mPath=Path(":/pads/n64/hat_right_active.svg"),            .mHint=_("DPAD RIGHT"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatBottom                 = { .mPath=Path(":/pads/n64/hat_down_active.svg"),             .mHint=_("DPAD DOWN"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .HatLeft                   = { .mPath=Path(":/pads/n64/hat_left_active.svg"),             .mHint=_("DPAD LEFT"),          .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Hat,  .mBelow=false, .mSkippable=false },
      .JoystickLeftUp            = { .mPath=Path(":/pads/n64/joy_up_active.svg"),               .mHint=_("STICK UP"),           .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftRight         = { .mPath=Path(":/pads/n64/joy_right_active.svg"),            .mHint=_("STICK RIGHT"),        .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftBottom        = { .mPath=Path(":/pads/n64/joy_down_active.svg"),             .mHint=_("STICK DOWN"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickLeftLeft          = { .mPath=Path(":/pads/n64/joy_left_active.svg"),             .mHint=_("STICK LEFT"),         .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Axis, .mBelow=false, .mSkippable=true  },
      .JoystickRightUp           = { .mPath=Path(":/pads/n64/arrow_up_active.svg"),             .mHint=_("C UP"),               .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .JoystickRightRight        = { .mPath=Path(":/pads/n64/arrow_right_active.svg"),          .mHint=_("C RIGHT"),            .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .JoystickRightBottom       = { .mPath=Path(":/pads/n64/arrow_down_active.svg"),           .mHint=_("C DOWN"),             .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .JoystickRightLeft         = { .mPath=Path(":/pads/n64/arrow_left_active.svg"),           .mHint=_("C LEFT"),             .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonEast                   = { .mPath=Path(":/pads/n64/btn_a_active.svg"),                .mHint="A",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSouth                   = { .mPath=Path(":/pads/n64/btn_b_active.svg"),                .mHint="B",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonEastPrime                   = { .mPath=Path(), .mHint="C", .mHintLong=String::Empty, .mPrefered=InputEvent::EventType::Key, .mBelow=false, .mSkippable=true  },
      .ButtonNorth                   = { .mPath=Path(),                                             .mHint="X",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonWest                   = { .mPath=Path(),                                             .mHint="Y",                     .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonNorthPrime                   = { .mPath=Path(":/pads/n64/btn_z_active.svg"),                .mHint="Z",                     .mHintLong=_("Z (Trigger)"),               .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL1                  = { .mPath=Path(":/pads/n64/btn_l_active.svg"),                .mHint="L",                     .mHintLong=_("L (Left Shoulder)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR1                  = { .mPath=Path(":/pads/n64/btn_r_active.svg"),                .mHint="R",                     .mHintLong=_("R (Right Shoulder)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL2                  = { .mPath=Path(),                                             .mHint="L2",                    .mHintLong=_("L2 (Left Trigger)"),         .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR2                  = { .mPath=Path(),                                             .mHint="R2",                    .mHintLong=_("R2 (Right Trigger)"),        .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonL3                  = { .mPath=Path(),                                             .mHint="L3",                    .mHintLong=_("L3 (Left Joystick Click)"),  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonR3                  = { .mPath=Path(),                                             .mHint="R3",                    .mHintLong=_("R3 (Right Joystick Click)"), .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonStart               = { .mPath=Path(":/pads/n64/btn_start_active.svg"),            .mHint="START",              .mHintLong=String::Empty,                  .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=false },
      .ButtonSelect              = { .mPath=Path(),                                             .mHint=String("**SELECT**\n").Append(_("Skip if absent")),             .mHintLong=String("**SELECT**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
      .ButtonHotkey              = { .mPath=Path(),                                             .mHint=String("**HOTKEY**\n").Append(_("Skip if absent")),             .mHintLong=String("**HOTKEY**\n").Append(_("Skip if physically absent")),    .mPrefered=InputEvent::EventType::Key,  .mBelow=false, .mSkippable=true  },
    }
  },
  };
  return sPadDefinitions;
}

void GuiInputConfig::FillImage(PadImageDescriptor& descriptor)
{
  mPics->Clear();
  mPics->setImage((int)PadChunks::PadAboveFrame       , descriptor.PadAboveFrame             );
  mPics->setImage((int)PadChunks::PadBelowFrame       , descriptor.PadAboveFrame             );
  mPics->setImage((int)PadChunks::DPadNeutral         , descriptor.DPadNeutral               );
  mPics->setImage((int)PadChunks::JoystickLeftNeutral , descriptor.JoystickLeftNeutral       );
  mPics->setImage((int)PadChunks::JoystickRightNeutral, descriptor.JoystickRightNeutral      );
  mPics->setImage((int)PadChunks::DPadUpLeft            , descriptor.DPadUpLeft                );
  mPics->setImage((int)PadChunks::DPadUpRight           , descriptor.DPadUpRight               );
  mPics->setImage((int)PadChunks::DPadDownLeft          , descriptor.DPadDownLeft              );
  mPics->setImage((int)PadChunks::DPadDownRight         , descriptor.DPadDownRight             );
  mPics->setImage((int)PadChunks::JoystickLeftUpLeft    , descriptor.JoystickLeftUpLeft        );
  mPics->setImage((int)PadChunks::JoystickLeftUpRight   , descriptor.JoystickLeftUpRight       );
  mPics->setImage((int)PadChunks::JoystickLeftDownLeft  , descriptor.JoystickLeftDownLeft      );
  mPics->setImage((int)PadChunks::JoystickLeftDownRight , descriptor.JoystickLeftDownRight     );
  mPics->setImage((int)PadChunks::JoystickRightUpLeft   , descriptor.JoystickRightUpLeft       );
  mPics->setImage((int)PadChunks::JoystickRightUpRight  , descriptor.JoystickRightUpRight      );
  mPics->setImage((int)PadChunks::JoystickRightDownLeft , descriptor.JoystickRightDownLeft     );
  mPics->setImage((int)PadChunks::JoystickRightDownRight, descriptor.JoystickRightDownRight    );
  mPics->setImage((int)PadChunks::HatTop              , descriptor.HatTop.mPath              );
  mPics->setImage((int)PadChunks::HatRight            , descriptor.HatRight.mPath            );
  mPics->setImage((int)PadChunks::HatBottom           , descriptor.HatBottom.mPath           );
  mPics->setImage((int)PadChunks::HatLeft             , descriptor.HatLeft.mPath             );
  mPics->setImage((int)PadChunks::JoystickLeftUp      , descriptor.JoystickLeftUp.mPath      );
  mPics->setImage((int)PadChunks::JoystickLeftRight   , descriptor.JoystickLeftRight.mPath   );
  mPics->setImage((int)PadChunks::JoystickLeftBottom  , descriptor.JoystickLeftBottom.mPath  );
  mPics->setImage((int)PadChunks::JoystickLeftLeft    , descriptor.JoystickLeftLeft.mPath    );
  mPics->setImage((int)PadChunks::JoystickRightUp     , descriptor.JoystickRightUp.mPath     );
  mPics->setImage((int)PadChunks::JoystickRightRight  , descriptor.JoystickRightRight.mPath  );
  mPics->setImage((int)PadChunks::JoystickRightBottom , descriptor.JoystickRightBottom.mPath );
  mPics->setImage((int)PadChunks::JoystickRightLeft   , descriptor.JoystickRightLeft.mPath   );
  mPics->setImage((int)PadChunks::ButtonEast             , descriptor.ButtonEast.mPath             );
  mPics->setImage((int)PadChunks::ButtonSouth             , descriptor.ButtonSouth.mPath             );
  mPics->setImage((int)PadChunks::ButtonEastPrime             , descriptor.ButtonEastPrime.mPath             );
  mPics->setImage((int)PadChunks::ButtonNorth             , descriptor.ButtonNorth.mPath             );
  mPics->setImage((int)PadChunks::ButtonWest             , descriptor.ButtonWest.mPath             );
  mPics->setImage((int)PadChunks::ButtonNorthPrime             , descriptor.ButtonNorthPrime.mPath             );
  mPics->setImage((int)PadChunks::ButtonL1            , descriptor.ButtonL1.mPath            );
  mPics->setImage((int)PadChunks::ButtonR1            , descriptor.ButtonR1.mPath            );
  mPics->setImage((int)PadChunks::ButtonL2            , descriptor.ButtonL2.mPath            );
  mPics->setImage((int)PadChunks::ButtonR2            , descriptor.ButtonR2.mPath            );
  mPics->setImage((int)PadChunks::ButtonL3            , descriptor.ButtonL3.mPath            );
  mPics->setImage((int)PadChunks::ButtonR3            , descriptor.ButtonR3.mPath            );
  mPics->setImage((int)PadChunks::ButtonStart         , descriptor.ButtonStart.mPath         );
  mPics->setImage((int)PadChunks::ButtonSelect        , descriptor.ButtonSelect.mPath        );
  mPics->setImage((int)PadChunks::ButtonHotkey        , descriptor.ButtonHotkey.mPath        );
}

void GuiInputConfig::HighlightImage()
{
  const PadImageItem* item = CurrentItem();
  if (item == nullptr) return;

  // Hide all chunks
  for (int i = (int)PadChunks::__Count; --i >= 0; ) mPics->SetVisible(i, false);
  // Show frame & neutral joysticks
  mPics->SetVisible((int)(item->mBelow ? PadChunks::PadBelowFrame : PadChunks::PadAboveFrame), true)
        .SetVisible((int)PadChunks::JoystickLeftNeutral, !item->mBelow)
        .SetVisible((int)PadChunks::JoystickRightNeutral, !item->mBelow);

  // Phase-specific highlight of the current chunk:
  //   Wait        : slow blink (sWaitBlinkPeriod cycle)
  //   Action      : solid pressed until the user releases
  //   Display     : blink 2x faster than Wait, under the capture overlay
  //   AlreadyUsed : solid pressed under the red "ALREADY USED" overlay
  bool showChunk = false;
  switch (mConfigPhase)
  {
    case ConfigPhase::Wait:    showChunk = (mPhaseElapsedMs / (sWaitBlinkPeriod / 2)) % 2 == 0; break;
    case ConfigPhase::Display: showChunk = (mPhaseElapsedMs / (sWaitBlinkPeriod / 8)) % 2 == 0; break;
    case ConfigPhase::Action:
    case ConfigPhase::AlreadyUsed: showChunk = true; break;
  }
  mPics->SetVisible((int)mCurrentChunk, showChunk);

  // Arcade pads ship a DPadNeutral stick image; show it whenever no DPad
  // direction is on screen — covers blink-off frames AND non-DPad chunks
  // across every config phase.
  const PadImageDescriptor& desc = GetPadDefinitions()[mTargetType];
  if (!desc.DPadNeutral.IsEmpty())
  {
    bool dpadDirShown = showChunk && (mCurrentChunk == PadChunks::HatTop ||
                                      mCurrentChunk == PadChunks::HatRight ||
                                      mCurrentChunk == PadChunks::HatBottom ||
                                      mCurrentChunk == PadChunks::HatLeft);
    mPics->SetVisible((int)PadChunks::DPadNeutral, !dpadDirShown);
  }
}

bool GuiInputConfig::IsChunkActiveInTesting(PadChunks chunk) const
{
  InputDevice::Entry entry = ChunkToEntryMap().get_or_return_default(chunk);
  if (!mTargetDevice->IsSet(entry)) return false;

  InputEvent binding;
  if (!mTargetDevice->GetEntryConfiguration(entry, binding)) return false;

  switch (binding.Type())
  {
    case InputEvent::EventType::Button:
    case InputEvent::EventType::Key:
      return (mActiveButtons & (1 << (binding.Id() & (sMaxButtons - 1)))) != 0;

    case InputEvent::EventType::Axis:
    {
      // 2 bits signed binary number: 11 = -1, 01 = 1, 0 = 0, 10 not used
      int currentSign = (mActiveAxis >> (2 * binding.Id())) & 0x3;
      // L2/R2 triggers, TryAssignToCurrent forces a positive binding value, but the
      // physical sign at press time depends on the axis's neutral. Light up on any
      // deflection.
      if (entry == InputDevice::Entry::L2 || entry == InputDevice::Entry::R2)
        return currentSign != 0;
      // Walkthrough order is Up, Left, Bottom, Right per joystick: Up and Left
      // own the binding (configured first), Bottom and Right are auto-skipped
      // and need their expected currentSign inverted in Testing.
      bool oppositeStickChunk = chunk == PadChunks::JoystickLeftBottom || chunk == PadChunks::JoystickLeftRight ||
                                chunk == PadChunks::JoystickRightBottom || chunk == PadChunks::JoystickRightRight;
      // mActiveAxis encodes negative as 3 and positive as 1 (see RecordRawInput),
      // so the binding's stored sign maps the same way.
      int bindingSign = binding.Value() < 0 ? 3 : 1;
      int expected = oppositeStickChunk ? (bindingSign == 1 ? 3 : 1) : bindingSign;
      return currentSign == expected;
    }

    case InputEvent::EventType::Hat:
    {
      int hatId = binding.Id() & (sMaxHats - 1);
      // mLastHatDirection holds 0 when neutral or the latest pure direction otherwise,
      // so diagonals never light up two chunks at once.
      return (mLastHatDirection[hatId] & binding.Value()) != 0;
    }

    case InputEvent::EventType::Unknown:
    case InputEvent::EventType::MouseButton:
    case InputEvent::EventType::MouseWheel:
    default:
      return false;
  }
}

void GuiInputConfig::HighlightTesting()
{
  // Hide everything, then re-show the persistent frame.
  for (int i = (int)PadChunks::__Count; --i >= 0; ) mPics->SetVisible(i, false);
  mPics->SetVisible((int)PadChunks::PadAboveFrame, true);

  // Sample stick + DPAD directional state up-front so we can collapse a cardinal
  // pair into a diagonal overlay when the descriptor provides one.
  bool hUp     = IsChunkActiveInTesting(PadChunks::HatTop);
  bool hRight  = IsChunkActiveInTesting(PadChunks::HatRight);
  bool hDown   = IsChunkActiveInTesting(PadChunks::HatBottom);
  bool hLeft   = IsChunkActiveInTesting(PadChunks::HatLeft);
  bool lUp     = IsChunkActiveInTesting(PadChunks::JoystickLeftUp);
  bool lRight  = IsChunkActiveInTesting(PadChunks::JoystickLeftRight);
  bool lDown   = IsChunkActiveInTesting(PadChunks::JoystickLeftBottom);
  bool lLeft   = IsChunkActiveInTesting(PadChunks::JoystickLeftLeft);
  bool rUp     = IsChunkActiveInTesting(PadChunks::JoystickRightUp);
  bool rRight  = IsChunkActiveInTesting(PadChunks::JoystickRightRight);
  bool rDown   = IsChunkActiveInTesting(PadChunks::JoystickRightBottom);
  bool rLeft   = IsChunkActiveInTesting(PadChunks::JoystickRightLeft);

  // Light up buttons normally; DPAD + sticks handled below.
  for (int c = (int)PadChunks::ButtonSouth; c < (int)PadChunks::__Count; ++c)
  {
    if (c >= (int)PadChunks::HatTop && c <= (int)PadChunks::JoystickRightLeft) continue;
    if (IsChunkActiveInTesting((PadChunks)c))
      mPics->SetVisible(c, true);
  }

  const PadImageDescriptor& desc = GetPadDefinitions()[mTargetType];
  auto renderStick = [this](bool up, bool right, bool down, bool left,
                            PadChunks cUp, PadChunks cRight, PadChunks cDown, PadChunks cLeft,
                            PadChunks cUpLeft,    const Path& pUpLeft,
                            PadChunks cUpRight,   const Path& pUpRight,
                            PadChunks cDownLeft,  const Path& pDownLeft,
                            PadChunks cDownRight, const Path& pDownRight)
  {
    if (up && left   && !pUpLeft.IsEmpty())    { mPics->SetVisible((int)cUpLeft,    true); return; }
    if (up && right  && !pUpRight.IsEmpty())   { mPics->SetVisible((int)cUpRight,   true); return; }
    if (down && left && !pDownLeft.IsEmpty())  { mPics->SetVisible((int)cDownLeft,  true); return; }
    if (down && right&& !pDownRight.IsEmpty()) { mPics->SetVisible((int)cDownRight, true); return; }
    if (up)    mPics->SetVisible((int)cUp,    true);
    if (right) mPics->SetVisible((int)cRight, true);
    if (down)  mPics->SetVisible((int)cDown,  true);
    if (left)  mPics->SetVisible((int)cLeft,  true);
  };
  renderStick(hUp, hRight, hDown, hLeft,
              PadChunks::HatTop, PadChunks::HatRight,
              PadChunks::HatBottom, PadChunks::HatLeft,
              PadChunks::DPadUpLeft,    desc.DPadUpLeft,
              PadChunks::DPadUpRight,   desc.DPadUpRight,
              PadChunks::DPadDownLeft,  desc.DPadDownLeft,
              PadChunks::DPadDownRight, desc.DPadDownRight);
  renderStick(lUp, lRight, lDown, lLeft,
              PadChunks::JoystickLeftUp, PadChunks::JoystickLeftRight,
              PadChunks::JoystickLeftBottom, PadChunks::JoystickLeftLeft,
              PadChunks::JoystickLeftUpLeft,    desc.JoystickLeftUpLeft,
              PadChunks::JoystickLeftUpRight,   desc.JoystickLeftUpRight,
              PadChunks::JoystickLeftDownLeft,  desc.JoystickLeftDownLeft,
              PadChunks::JoystickLeftDownRight, desc.JoystickLeftDownRight);
  renderStick(rUp, rRight, rDown, rLeft,
              PadChunks::JoystickRightUp, PadChunks::JoystickRightRight,
              PadChunks::JoystickRightBottom, PadChunks::JoystickRightLeft,
              PadChunks::JoystickRightUpLeft,    desc.JoystickRightUpLeft,
              PadChunks::JoystickRightUpRight,   desc.JoystickRightUpRight,
              PadChunks::JoystickRightDownLeft,  desc.JoystickRightDownLeft,
              PadChunks::JoystickRightDownRight, desc.JoystickRightDownRight);

  mPics->SetVisible((int)PadChunks::JoystickLeftNeutral,  !(lUp || lRight || lDown || lLeft))
        .SetVisible((int)PadChunks::JoystickRightNeutral, !(rUp || rRight || rDown || rLeft))
        .SetVisible((int)PadChunks::DPadNeutral,          !(hUp || hRight || hDown || hLeft));
}

GuiInputConfig::~GuiInputConfig()
{
  // Always restore the kernel R.G.P. exit-combo toggles to their pre-config
  // state, whether the wizard was completed, cancelled, timed-out or aborted
  // by a hot-unplug. Safe even if the module wasn't loaded (state.valid=false).
  RGPProxy::Instance().RestoreExitCombos(mSavedExitCombos);

  // mTargetDevice is set to nullptr once proxification snapshots the bindings,
  // because the kernel grab triggers SDL_JOYDEVICEREMOVED and InputManager
  // deletes every InputDevice — leaving the original mTargetDevice dangling.
  if (mTargetDevice != nullptr) mTargetDevice->SetConfiguringState(false);
  InputManager::Instance().RemoveNotificationInterface(this);
}

GuiInputConfig::GuiInputConfig(WindowManager&window, InputDevice* target, PadTypes::Type targedType, IDeviceConfigured* callback, bool testOnly)
  : Gui(window)
  , mGrid(window, Vector2i(3, 5))
  , mPreviousConfig(*target)
  , mTargetDevice(target)
  , mCallback(callback)
  , mTargetType((targedType))
  , mCurrentChunk(PadChunks::HatTop)
  , mGlobalState(testOnly ? GlobalState::Testing : GlobalState::Configuring)
  , mConfigPhase(ConfigPhase::Wait)
  , mPhaseElapsedMs(0)
  , mStartHoldMs(-1)
  , mDisplayWidthCoef(0.f)
  , mActiveButtons(0)
  , mActiveAxis(0)
  , mActiveHats(0)
  , mLastHatDirection { 0 }
  , mButtonReferenceTime { 0 }
  , mNeutralPositionSet(true)  // assume neutral at GUI open
  , mFirstEventReceived(true)
  , mWaitNeutralSeen(true)     // matches NeutralPosition() == true at startup
  , mCancelled(false)
  , mTestOnly(testOnly)
  , mProxifyInProgress(false)
  , mSourceUid(0)
  , mProxyWaitMs(0)
  , mProxyPollMs(0)
  , mSourceBindingsBits(0)
{
  memset(&mButtonReferenceTime, 0, sizeof(mButtonReferenceTime));

  mTargetDevice->SetConfiguringState(true);
  { LOG(LogInfo) << "[GuiInput] " << (mTestOnly ? "Testing" : "Configuring")
                 << " device " << mTargetDevice->Index() << " (" << mTargetDevice->Name() << ")."; }

  // Disable kernel R.G.P. exit combos (L3+R3 / long-press START) for the
  // duration of the GUI; the destructor will restore the snapshot. Without
  // this, pressing one of those during the config wizard would terminate
  // the running emulator. No-op when the kernel module isn't loaded.
  mSavedExitCombos = RGPProxy::Instance().DisableExitCombos();

  // Test mode preserves the existing bindings (loaded from es_input.cfg) so the
  // user can verify them. Configuration mode wipes them clean to start fresh.
  if (!mTestOnly) mTargetDevice->ClearAll();

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  addChild(&mGrid);

  mTitle = std::make_shared<TextScrollComponent>(mWindow, mTestOnly ? _("TESTING") : _("CONFIGURING"), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 0), false, true);

  String deviceName = _("KEYBOARD");
  if (mTargetDevice->Identifier() != InputEvent::sKeyboardDevice)
    deviceName = _("GAMEPAD %i").Replace("%i", mTargetDevice->Name());

  mSubtitle1 = std::make_shared<TextScrollComponent>(mWindow, deviceName.UpperCaseUTF8(), menuTheme.Text().font, menuTheme.Footer().color, ::Alignment::Center);
  mGrid.setEntry(mSubtitle1, Vector2i(1, 1), false, true);

  mSubtitle2 = std::make_shared<MarkdownText>(mWindow, "", *menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::TopCenter);
  mGrid.setEntry(mSubtitle2, Vector2i(1, 2), false, true);

  mCancel = std::make_shared<TextScrollComponent>(mWindow, _("Press any button for 5s to cancel !"), menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::Center);
  mGrid.setEntry(mCancel, Vector2i(1, 4), false, true);

  mPics = std::make_shared<StackablePictureComponent>(mWindow, ImgProps::KeepRatio);
  //const MenuThemeData& menu = ThemeManager::Instance().Menu();
  mGrid.setEntry(mPics, Vector2i(1, 3), true, true);

  float width = Renderer::Instance().Is480pOrLower() ? Renderer::Instance().DisplayWidthAsFloat() * 0.80f : Renderer::Instance().DisplayWidthAsFloat() * 0.60f;
  float height = Renderer::Instance().Is480pOrLower() ? Renderer::Instance().DisplayHeightAsFloat() * 0.70f : Renderer::Instance().DisplayHeightAsFloat() * 0.85f;
  setSize(width, height);
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

  FillImage(GetPadDefinitions()[mTargetType]);

  InputManager::Instance().AddNotificationInterface(this);
}

void GuiInputConfig::onSizeChanged()
{
  // update grid
  mGrid.setSize(mSize);

  // Subtitle 2 hosts the phase instructions in 2 lines max , over that, MarkdownText autoscrolls vertically.
  float smallFontHeight = (float)ThemeManager::Instance().Menu().SmallText().font->Height();
  mGrid.setColWidthPerc(0, 0.02f);
  mGrid.setColWidthPerc(1, 0.f);
  mGrid.setColWidthPerc(2, 0.02f);
  mGrid.setRowHeightPerc(0, (float)mTitle->getFont()->Height() / mSize.y());
  mGrid.setRowHeightPerc(1, (float)mSubtitle1->getFont()->Height() / mSize.y());
  mGrid.setRowHeightPerc(2, 3.f * smallFontHeight / mSize.y());
  mGrid.setRowHeightPerc(4, (float)mCancel->getFont()->Height() * 1.5f / mSize.y());
}

bool GuiInputConfig::TryAssignToCurrent(InputEvent input)
{
  InputDevice::Entry entry = CurrentEntry();

  // If the input is already mapped to a different entry, except for Hotkey (typically Select).
  if (entry != InputDevice::Entry::Hotkey &&
      mTargetDevice->GetMatchedEntry(input) != InputDevice::Entry::None &&
      !mTargetDevice->IsMatching(entry, input))
    return false;

  // Rewrite axis values
  if (input.Type() == InputEvent::EventType::Axis)
    input = InputEvent(input.Device(), input.Type(), input.Id(), input.Value() < 0 ? -1 : input.Value() > 0 ? 1 : 0, input.Code());

  // L2/R2 trigger axes are required to be positive
  if ((entry == InputDevice::Entry::L2) || (entry == InputDevice::Entry::R2))
    if ((input.Type() == InputEvent::EventType::Axis) && (input.Value() < 0))
      input = InputEvent(input.Device(), input.Type(), input.Id(), -input.Value(), input.Code());

  // Set a code while the device is still connected
  input.StoreSDLCode(mTargetDevice->Index());
  mTargetDevice->Set(entry, input);

  { LOG(LogInfo) << "[GuiInput]   Mapping [" << input.ToString() << "] -> chunk " << (int)mCurrentChunk; }

  return true;
}

bool GuiInputConfig::ProcessInput(const InputCompactEvent& event)
{
  // During WaitingProxy mTargetDevice is dangling (the kernel grab triggered
  // SDL_JOYDEVICEREMOVED → InputManager::ClearAllConfigurations). Drop every
  // input until the proxy appears or the timeout fires; Update() drives the
  // state transition on its own.
  if (mGlobalState == GlobalState::WaitingProxy) return true;

  // Events from other devices: in test mode we still consume them (return true)
  // to block shortcut leaks (a press on the keyboard or another pad would
  // otherwise bubble up to the search bar / menu navigation). In config mode
  // we let them pass through so other GUIs keep working.
  if(&event.Device() != mTargetDevice)
    return mTestOnly;

  if (mCancelled)
  {
    if (mTargetDevice->CheckNeutralPosition()) Close();
    return true;
  }

  // 8bitdo DInput bug
  if (mFirstEventReceived)
  {
    event.Device().RecordAxisNeutralPosition();
    mFirstEventReceived = false;
  }

  // Neutral position has been reached?
  if (!mNeutralPositionSet)
  {
    if (mTargetDevice->CheckNeutralPosition(event.Device().Name() == "Namco GunCon 2"))
    {
      mNeutralPositionSet = true;
      mWaitNeutralSeen = true;
    }
    else return true;
  }

  // Always record raw input , feeds NeutralPosition(), mEventList and the
  // 5s cancel-on-hold detection regardless of current state.
  RecordRawInput(event.RawEvent());

  switch (mGlobalState)
  {
    case GlobalState::Configuring:  HandleConfiguringInput();      break;
    case GlobalState::Testing:      HandleTestingInput(event);     break;
    case GlobalState::WaitingProxy: break; // unreachable: early-returned above
  }

  return true;
}

void GuiInputConfig::HandleConfiguringInput()
{
  switch (mConfigPhase)
  {
    case ConfigPhase::Wait:
      if (NeutralPosition())
      {
        mWaitNeutralSeen = true;
        mEventList.clear();
      }
      else if (mWaitNeutralSeen)
      {
        EnterPhase(ConfigPhase::Action);
      }
      break;
    case ConfigPhase::Action:
      if (!mEventList.empty() && NeutralPosition()) ProcessEvents();
      break;
    case ConfigPhase::Display:
    case ConfigPhase::AlreadyUsed:
      break;
  }
}

void GuiInputConfig::HandleTestingInput(const InputCompactEvent& event)
{
  if (event.HotkeyStartPressed())
  {
    if (mTestOnly) { Close(); return; }
    { LOG(LogDebug) << "[GuiInput][RGP] HotkeyStartPressed — ShouldProxify=" << (ShouldProxify() ? "true" : "false")
                   << " Select=" << (mTargetDevice->IsSet(InputDevice::Entry::Select) ? "set" : "unset")
                   << " Hotkey=" << (mTargetDevice->IsSet(InputDevice::Entry::Hotkey) ? "set" : "unset"); }
    if (ShouldProxify()) { StartProxification(); return; }
    InputManager::Instance().WriteDeviceXmlConfiguration(mTargetDevice->SetPadType(mTargetType));
    if (mCallback != nullptr) mCallback->DeviceConfigured(*mTargetDevice);
    Close();
    return;
  }
  // Start press arms the long-press counter; release cancels it.
  if (event.StartPressed())  mStartHoldMs = 0;
  if (event.StartReleased()) mStartHoldMs = -1;
}

bool GuiInputConfig::ShouldProxify() const
{
  // Proxify only when both Select and Hotkey are missing — that's the case
  // where the kernel module synthesises them from START combos. If the user
  // mapped at least one of them we keep the bare config.
  return !mTargetDevice->IsSet(InputDevice::Entry::Select)
      && !mTargetDevice->IsSet(InputDevice::Entry::Hotkey);
}

void GuiInputConfig::StartProxification()
{
  mProxifyInProgress = true;
  mSourceName = mTargetDevice->Name();
  mSourceUid = RGPProxy::GetUid(mTargetDevice->Index());
  { LOG(LogDebug) << "[GuiInput][RGP] StartProxification: name='" << mSourceName
                 << "' sdlIndex=" << mTargetDevice->Index() << " kernelUid=" << mSourceUid; }
  // Snapshot the bindings now: the kernel grab will trigger SDL_JOYDEVICEREMOVED,
  // InputManager::Refresh will delete every InputDevice in mIdToDevices, and
  // mTargetDevice will dangle. After this point we never touch mTargetDevice.
  mSourceBindingsBits = 0;
  for (int i = 0; i < (int)InputDevice::Entry::__Count; ++i)
  {
    auto entry = (InputDevice::Entry)i;
    if (!mTargetDevice->IsSet(entry)) continue;
    InputEvent ev;
    if (mTargetDevice->GetEntryConfiguration(entry, ev))
    {
      mSourceBindings[i] = ev;
      mSourceBindingsBits |= (1 << i);
    }
  }
  { LOG(LogDebug) << "[GuiInput] No Select/Hotkey set — requesting R.G.P. proxy for '"
                 << mSourceName << "' (uid=" << mSourceUid << ")."; }
  // GetUid returns 0 when the kernel module doesn't track the device. Without
  // a UID we can't reliably pair the proxy back to the source, so fail fast.
  if (mSourceUid == 0)
  {
    mWindow.pushGui(new GuiMsgBoxScroll(mWindow, _("R.G.P. unavailable"),
      _("The R.G.P. kernel module did not assign a UID to your controller. "
        "Proxification cannot proceed — your bindings will not be saved."),
      _("OK"), [] {}));
    mProxifyInProgress = false;
    Close();
    return;
  }
  // Last call that uses mTargetDevice: clear its "configuring" flag now while
  // the pointer is still valid. After the AddDeviceFilter().Apply() below the
  // kernel grab will fire SDL_JOYDEVICEREMOVED → ClearAllConfigurations() and
  // mTargetDevice becomes dangling.
  mTargetDevice->SetConfiguringState(false);
  mTargetDevice = nullptr;
  // AddDeviceFilter returns bool (true if newly added, false if already there
   // or persistence failed); Apply() pushes to sysfs separately.
  RGPProxy::Instance().AddDeviceFilter(mSourceName);
  RGPProxy::Instance().Apply();
  mProxyWaitMs = 0;
  mProxyPollMs = 0;
  EnterGlobalState(GlobalState::WaitingProxy);
}

void GuiInputConfig::FinalizeProxification(int proxyIndex)
{
  // mTargetDevice is dangling here (deleted by InputManager::ClearAllConfigurations
  // during the REMOVED→Refresh that the kernel grab triggered). We use
  // mSourceBindings/mSourceBindingsBits, snapshotted in StartProxification.
  InputDevice& proxyDev = InputManager::Instance().GetDeviceConfigurationFromIndex(proxyIndex);
  for (int i = 0; i < (int)InputDevice::Entry::__Count; ++i)
    if ((mSourceBindingsBits & (1 << i)) != 0)
      proxyDev.Set((InputDevice::Entry)i, mSourceBindings[i]);
  { LOG(LogDebug) << "[GuiInput] R.G.P. proxy ready at index " << proxyIndex
                 << " ('" << proxyDev.Name() << "'), saving config."; }
  InputManager::Instance().WriteDeviceXmlConfiguration(proxyDev.SetPadType(mTargetType));
  if (mCallback != nullptr) mCallback->DeviceConfigured(proxyDev);
  mProxifyInProgress = false;
  Close();
}

void GuiInputConfig::ProxificationTimeout()
{
  { LOG(LogError) << "[GuiInput] R.G.P. proxy did not appear within "
                  << sProxyWaitTimeoutMs << "ms for '" << mSourceName
                  << "' (uid=" << mSourceUid << "). Aborting save."; }
  mWindow.pushGui(new GuiMsgBoxScroll(mWindow, _("R.G.P. timeout"),
    _("The R.G.P. proxy device did not appear in time. Your bindings have "
      "not been saved. Try again or check the kernel module status."),
    _("OK"), [] {}));
  mProxifyInProgress = false;
  Close();
}

bool GuiInputConfig::CollectHelpItems(Help &help)
{
    help.Set(Help::Cancel(), _("CANCEL"));
    return true;
}

void GuiInputConfig::ProcessEvents()
{
  std::vector<InputEvent> events = std::move(mEventList);
  mEventList.clear();

  // Step back via DPad-Left (only available once Left has been mapped).
  if (mTargetDevice->IsSet(InputDevice::Entry::Left))
  {
    for (auto& input : events)
      if (mTargetDevice->IsMatching(InputDevice::Entry::Left, input))
      {
        StepBack();
        return;
      }
  }

  // Skip via DPad-Right (only available once Right has been mapped)
  if (IsCurrentChunkSkippable() && mTargetDevice->IsSet(InputDevice::Entry::Right))
  {
    for (auto& input : events)
      if (mTargetDevice->IsMatching(InputDevice::Entry::Right, input))
      {
        // Skipping Hotkey auto-clones the Select binding when present; otherwise
        // fall through to the plain skip — Testing state handles the no-Select /
        // no-Hotkey case via long-press START.
        if (mCurrentChunk == PadChunks::ButtonHotkey)
        {
          InputEvent selectEvent;
          if (mTargetDevice->GetEntryConfiguration(InputDevice::Entry::Select, selectEvent))
          {
            selectEvent.StoreSDLCode(mTargetDevice->Index());
            mTargetDevice->Set(InputDevice::Entry::Hotkey, selectEvent);
            mLastCapturedText = _("SAME AS SELECT");
            { LOG(LogInfo) << "[GuiInput]   Hotkey skipped, cloned from Select"; }
            EnterPhase(ConfigPhase::Display);
            return;
          }
        }
        mLastCapturedText = _("(skipped)");
        { LOG(LogInfo) << "[GuiInput]   Skipping chunk " << (int)mCurrentChunk; }
        EnterPhase(ConfigPhase::Display);
        return;
      }
  }

  // Try preferred type first, then any type as fallback
  const PadImageItem* item = CurrentItem();
  InputEvent::EventType prefered = item != nullptr ? item->mPrefered : InputEvent::EventType::Button;
  for (auto& input : events)
    if (input.Type() == prefered && TryAssignToCurrent(input))
    {
      mLastCapturedText = input.ToString().UpperCase();
      EnterPhase(ConfigPhase::Display);
      return;
    }
  for (auto& input : events)
    if (TryAssignToCurrent(input))
    {
      mLastCapturedText = input.ToString().UpperCase();
      EnterPhase(ConfigPhase::Display);
      return;
    }

  if (!events.empty())
  {
    const PadImageItem* alreadyItem = nullptr;
    for (const auto& input : events)
      if ((alreadyItem = FindItemForInput(input)) != nullptr) break;
    mLastCapturedText = (alreadyItem != nullptr && !alreadyItem->mHint.empty())
                          ? (_F(_("ALREADY USED BY {0}")) / alreadyItem->mHint).ToString()
                          : _("ALREADY USED");
    EnterPhase(ConfigPhase::AlreadyUsed);
  }
  else
  {
    EnterPhase(ConfigPhase::Wait);
  }
}

void GuiInputConfig::RecordRawInput(const InputEvent& raw)
{
  switch(raw.Type())
  {
    case InputEvent::EventType::Axis:
    {
      // Calculate
      int value = raw.Value();
      if (int neutral = mTargetDevice->NeutralAxisValue(raw.Id()); neutral != 0)
        value = raw.Value() < 0 && neutral > 0 ? raw.Value() : (raw.Value() > 0 && neutral < 0 ? raw.Value() : 0);

      // Affect?
      int mask = ~(0x3 << (2 * raw.Id()));
      int lookup = -1;
      for (int i = (int)mEventList.size(); --i >= 0; )
        if (const InputEvent& event = mEventList[i]; raw.Device() == event.Device() && raw.Id() == event.Id())
          lookup = i;
      // First event is recorded
      if (lookup < 0)
      {
        mEventList.push_back(InputEvent(raw.Device(), raw.Type(), raw.Id(), value));
        LOG(LogDebug) << "[GuiInputConfig] Initial: " << raw.ToString();
      }
      // Other events overwrite initial event iif abs(value) is higher then the previous stored value
      else if (Math::absi(value) > Math::absi(mEventList[lookup].Value()))
      {
        mEventList[lookup] = InputEvent(raw.Device(), raw.Type(), raw.Id(), value);
        LOG(LogDebug) << "[GuiInputConfig] Overwrite: " << raw.ToString();
      }

      // Assign value - 2 bits signed binary number: 11 = -1, 01 = 1, 0 = 0, 10 not used
      mActiveAxis &= mask;
      mActiveAxis |= (value < 0 ? 3 : value > 0 ? 1 : 0) << (2 * raw.Id());
      break;
    }
    case InputEvent::EventType::Button:
    {
      int id = raw.Id() & 0x1F;

      // Affect?
      if (raw.Value() == 0) mEventList.push_back(InputEvent(raw.Device(), raw.Type(), raw.Id(), 1));

      // Assign
      mActiveButtons &= ~(1 << id);
      mActiveButtons |= (raw.Value() << id);

      if (raw.Value() != 0) mButtonReferenceTime[id] = sCancelTime;
      else mButtonReferenceTime[id] = 0;

      break;
    }
    case InputEvent::EventType::Hat:
    {
      int mask = ~(0xF << (4 * raw.Id()));
      int value = raw.Value() << (4 * raw.Id());

      // Affect?
      if ((mActiveHats & ~mask) != 0) // This axis has been activated?
        if (raw.Value() == 0)
          mEventList.push_back(InputEvent(raw.Device(), raw.Type(), raw.Id(), (mActiveHats >> (4 * raw.Id())) & 0x0F));

      // Assign value
      mActiveHats &= mask;
      mActiveHats |= value;

      int hatId = raw.Id() & (sMaxHats - 1);
      if (raw.Value() == 0)                                                                    mLastHatDirection[hatId] = 0;
      else if (raw.Value() == 1 || raw.Value() == 2 || raw.Value() == 4 || raw.Value() == 8)   mLastHatDirection[hatId] = raw.Value();
      break;
    }
    case InputEvent::EventType::Unknown:
    case InputEvent::EventType::Key:
    case InputEvent::EventType::MouseButton:
    case InputEvent::EventType::MouseWheel:
    default: break;
  }
}

const HashMap<GuiInputConfig::PadChunks, InputDevice::Entry>& GuiInputConfig::ChunkToEntryMap()
{
  static const HashMap<PadChunks, InputDevice::Entry> sMap =
  {
    { PadChunks::ButtonSouth,             InputDevice::Entry::South         },
    { PadChunks::ButtonEast,             InputDevice::Entry::East         },
    { PadChunks::ButtonEastPrime,             InputDevice::Entry::EastPrime         },
    { PadChunks::HatTop,              InputDevice::Entry::Up        },
    { PadChunks::HatRight,            InputDevice::Entry::Right     },
    { PadChunks::HatBottom,           InputDevice::Entry::Down      },
    { PadChunks::HatLeft,             InputDevice::Entry::Left      },
    { PadChunks::JoystickLeftUp,      InputDevice::Entry::Joy1AxisV },
    { PadChunks::JoystickLeftLeft,    InputDevice::Entry::Joy1AxisH },
    { PadChunks::JoystickLeftBottom,  InputDevice::Entry::Joy1AxisV },
    { PadChunks::JoystickLeftRight,   InputDevice::Entry::Joy1AxisH },
    { PadChunks::JoystickRightUp,     InputDevice::Entry::Joy2AxisV },
    { PadChunks::JoystickRightLeft,   InputDevice::Entry::Joy2AxisH },
    { PadChunks::JoystickRightBottom, InputDevice::Entry::Joy2AxisV },
    { PadChunks::JoystickRightRight,  InputDevice::Entry::Joy2AxisH },
    { PadChunks::ButtonNorth,             InputDevice::Entry::North         },
    { PadChunks::ButtonWest,             InputDevice::Entry::West         },
    { PadChunks::ButtonNorthPrime,             InputDevice::Entry::NorthPrime         },
    { PadChunks::ButtonL1,            InputDevice::Entry::L1        },
    { PadChunks::ButtonR1,            InputDevice::Entry::R1        },
    { PadChunks::ButtonL2,            InputDevice::Entry::L2        },
    { PadChunks::ButtonR2,            InputDevice::Entry::R2        },
    { PadChunks::ButtonL3,            InputDevice::Entry::L3        },
    { PadChunks::ButtonR3,            InputDevice::Entry::R3        },
    { PadChunks::ButtonStart,         InputDevice::Entry::Start     },
    { PadChunks::ButtonSelect,        InputDevice::Entry::Select    },
    { PadChunks::ButtonHotkey,        InputDevice::Entry::Hotkey    },
  };
  return sMap;
}

InputDevice::Entry GuiInputConfig::CurrentEntry() const
{
  return ChunkToEntryMap().get_or_return_default(mCurrentChunk);
}

const GuiInputConfig::PadImageItem* GuiInputConfig::ItemForChunk(PadChunks chunk) const
{
  PadImageDescriptor& desc = GetPadDefinitions()[mTargetType];
  switch (chunk)
  {
    case PadChunks::ButtonSouth:             return &desc.ButtonSouth;
    case PadChunks::ButtonEast:             return &desc.ButtonEast;
    case PadChunks::ButtonEastPrime:             return &desc.ButtonEastPrime;
    case PadChunks::HatTop:              return &desc.HatTop;
    case PadChunks::HatRight:            return &desc.HatRight;
    case PadChunks::HatBottom:           return &desc.HatBottom;
    case PadChunks::HatLeft:             return &desc.HatLeft;
    case PadChunks::JoystickLeftUp:      return &desc.JoystickLeftUp;
    case PadChunks::JoystickLeftLeft:    return &desc.JoystickLeftLeft;
    case PadChunks::JoystickLeftBottom:  return &desc.JoystickLeftBottom;
    case PadChunks::JoystickLeftRight:   return &desc.JoystickLeftRight;
    case PadChunks::JoystickRightUp:     return &desc.JoystickRightUp;
    case PadChunks::JoystickRightLeft:   return &desc.JoystickRightLeft;
    case PadChunks::JoystickRightBottom: return &desc.JoystickRightBottom;
    case PadChunks::JoystickRightRight:  return &desc.JoystickRightRight;
    case PadChunks::ButtonNorth:             return &desc.ButtonNorth;
    case PadChunks::ButtonWest:             return &desc.ButtonWest;
    case PadChunks::ButtonNorthPrime:             return &desc.ButtonNorthPrime;
    case PadChunks::ButtonL1:            return &desc.ButtonL1;
    case PadChunks::ButtonR1:            return &desc.ButtonR1;
    case PadChunks::ButtonL2:            return &desc.ButtonL2;
    case PadChunks::ButtonR2:            return &desc.ButtonR2;
    case PadChunks::ButtonL3:            return &desc.ButtonL3;
    case PadChunks::ButtonR3:            return &desc.ButtonR3;
    case PadChunks::ButtonStart:         return &desc.ButtonStart;
    case PadChunks::ButtonSelect:        return &desc.ButtonSelect;
    case PadChunks::ButtonHotkey:        return &desc.ButtonHotkey;
    case PadChunks::PadAboveFrame:
    case PadChunks::PadBelowFrame:
    case PadChunks::JoystickLeftNeutral:
    case PadChunks::JoystickRightNeutral:
    case PadChunks::DPadNeutral:
    case PadChunks::DPadUpLeft:
    case PadChunks::DPadUpRight:
    case PadChunks::DPadDownLeft:
    case PadChunks::DPadDownRight:
    case PadChunks::JoystickLeftUpLeft:
    case PadChunks::JoystickLeftUpRight:
    case PadChunks::JoystickLeftDownLeft:
    case PadChunks::JoystickLeftDownRight:
    case PadChunks::JoystickRightUpLeft:
    case PadChunks::JoystickRightUpRight:
    case PadChunks::JoystickRightDownLeft:
    case PadChunks::JoystickRightDownRight:
    case PadChunks::__Count:
    default: return nullptr;
  }
}

const GuiInputConfig::PadImageItem* GuiInputConfig::FindItemForInput(const InputEvent& input) const
{
  InputDevice::Entry entry = mTargetDevice->GetMatchedEntry(input);
  if (entry == InputDevice::Entry::None) return nullptr;

  // Axis entries cover two opposite chunks; match the input sign against the
  // stored binding sign to pick the right one (Up/Right vs Bottom/Left).
  bool axisOpposite = false;
  if (input.Type() == InputEvent::EventType::Axis)
  {
    InputEvent stored;
    if (mTargetDevice->GetEntryConfiguration(entry, stored))
      axisOpposite = (input.Value() < 0) != (stored.Value() < 0);
  }

  PadImageDescriptor& desc = GetPadDefinitions()[mTargetType];
  switch (entry)
  {
    case InputDevice::Entry::South:         return &desc.ButtonSouth;
    case InputDevice::Entry::East:         return &desc.ButtonEast;
    case InputDevice::Entry::EastPrime:         return &desc.ButtonEastPrime;
    case InputDevice::Entry::Up:        return &desc.HatTop;
    case InputDevice::Entry::Right:     return &desc.HatRight;
    case InputDevice::Entry::Down:      return &desc.HatBottom;
    case InputDevice::Entry::Left:      return &desc.HatLeft;
    case InputDevice::Entry::Joy1AxisV: return axisOpposite ? &desc.JoystickLeftBottom  : &desc.JoystickLeftUp;
    case InputDevice::Entry::Joy1AxisH: return axisOpposite ? &desc.JoystickLeftLeft    : &desc.JoystickLeftRight;
    case InputDevice::Entry::Joy2AxisV: return axisOpposite ? &desc.JoystickRightBottom : &desc.JoystickRightUp;
    case InputDevice::Entry::Joy2AxisH: return axisOpposite ? &desc.JoystickRightLeft   : &desc.JoystickRightRight;
    case InputDevice::Entry::North:         return &desc.ButtonNorth;
    case InputDevice::Entry::West:         return &desc.ButtonWest;
    case InputDevice::Entry::NorthPrime:         return &desc.ButtonNorthPrime;
    case InputDevice::Entry::L1:        return &desc.ButtonL1;
    case InputDevice::Entry::R1:        return &desc.ButtonR1;
    case InputDevice::Entry::L2:        return &desc.ButtonL2;
    case InputDevice::Entry::R2:        return &desc.ButtonR2;
    case InputDevice::Entry::L3:        return &desc.ButtonL3;
    case InputDevice::Entry::R3:        return &desc.ButtonR3;
    case InputDevice::Entry::Start:     return &desc.ButtonStart;
    case InputDevice::Entry::Select:    return &desc.ButtonSelect;
    case InputDevice::Entry::Hotkey:    return &desc.ButtonHotkey;
    case InputDevice::Entry::None:
    case InputDevice::Entry::VolumeUp:
    case InputDevice::Entry::VolumeDown:
    case InputDevice::Entry::BrightnessUp:
    case InputDevice::Entry::BrightnessDown:
    case InputDevice::Entry::__Count:
    default: return nullptr;
  }
}

String GuiInputConfig::BuildInstructions() const
{
  if (mGlobalState == GlobalState::WaitingProxy)
    return _("Please wait, R.G.P. proxy starting...");
  if (mGlobalState == GlobalState::Testing)
  {
    if (mTestOnly) return _("**Long-press START** to exit");
    // No Select / Hotkey configured: the kernel proxy turns long-press START
    // into HOTKEY+START at runtime, so surface that to the user instead.
    if (!mTargetDevice->IsSet(InputDevice::Entry::Select) && !mTargetDevice->IsSet(InputDevice::Entry::Hotkey))
      return _("**Long-press START** to validate");
    return _("Press **HOTKEY + START** to validate");
  }

  switch (mConfigPhase)
  {
    case ConfigPhase::Wait:
    {
      String msg = _("Press %LABEL%");
      const PadImageItem* item = CurrentItem();
      String label;
      if (item != nullptr)
      {
        bool useLong = !Renderer::Instance().Is480pOrLower() && !item->mHintLong.empty();
        label = useLong ? item->mHintLong : item->mHint;
        // Single-line labels are auto-bolded
        if (label.find('\n') == String::npos) label = String("**").Append(label).Append("**");
      }
      msg.Replace("%LABEL%", label);
      return msg;
    }
    case ConfigPhase::Action:      return _("Release to confirm");
    case ConfigPhase::Display:     return String(); // overlay shows the captured event
    case ConfigPhase::AlreadyUsed: return String(); // overlay shows "ALREADY USED"
  }
  return String();
}

void GuiInputConfig::UpdateInstructions()
{
  String text = BuildInstructions();
  if (text == mLastInstructions) return;
  mLastInstructions = text;
  mSubtitle2->SetText(text);
}

void GuiInputConfig::UpdateFooter(const String& cancelMessage)
{
  String msg = cancelMessage;
  // Append skip/back hints so the user always sees them in the footer when relevant.
  if (mGlobalState == GlobalState::Configuring && mConfigPhase == ConfigPhase::Wait)
  {
    if (IsCurrentChunkSkippable() && mTargetDevice->IsSet(InputDevice::Entry::Right))
      msg.Append(" - ").Append(_("DPAD RIGHT to skip"));
    if (mTargetDevice->IsSet(InputDevice::Entry::Left) && (int)mCurrentChunk > (int)PadChunks::ButtonSouth)
      msg.Append(" - ").Append(_("DPAD LEFT to go back"));
  }
  mCancel->setText(msg);
}

bool GuiInputConfig::IsCurrentChunkSkippable() const
{
  // Per-pad skippability lives on the chunk's PadImageItem.mSkippable.
  const PadImageItem* item = CurrentItem();
  return item == nullptr || item->mSkippable;
}

void GuiInputConfig::EnterPhase(ConfigPhase phase)
{
  mConfigPhase = phase;
  mPhaseElapsedMs = 0;
  if (phase == ConfigPhase::Wait)
  {
    mEventList.clear();
    mWaitNeutralSeen = NeutralPosition();
  }
  if (phase == ConfigPhase::Display) mDisplayWidthCoef = sDisplayIntroInitialCoef;
}

void GuiInputConfig::EnterGlobalState(GlobalState state)
{
  mGlobalState = state;
  mPhaseElapsedMs = 0;
  mEventList.clear();
  if (state == GlobalState::Testing)
  {
    { LOG(LogInfo) << "[GuiInput] All chunks configured, entering Testing state."; }
    // No Select / Hotkey assigned: warn the user once on the way into Testing
    // that START will carry the special-action role at runtime (long-press
    // exits, START + button = hotkey, START + Validate = SELECT).
    if (!mTargetDevice->IsSet(InputDevice::Entry::Select) && !mTargetDevice->IsSet(InputDevice::Entry::Hotkey))
      mWindow.pushGui(new GuiMsgBoxScroll(mWindow, _("Button combination"),
        _("Your controller has __no SELECT or HOTKEY button__. The START button will be used for special actions:\n"
          "- **Long press on START** = quit the game (START + HOTKEY)\n"
          "- **START + Validate** = SELECT (insert credit in arcade games)\n"
          "- **START + any other button** = HOTKEY + that button"),
        _("OK"), [] {}));
  }
}

void GuiInputConfig::AdvanceChunk()
{
  // Walk forward through PadChunks & skip empty chunks
  while (true)
  {
    int next = (int)mCurrentChunk + 1;
    if (next >= (int)PadChunks::__Count)
    {
      EnterGlobalState(GlobalState::Testing);
      return;
    }
    mCurrentChunk = (PadChunks)next;
    if (mTargetDevice->IsSet(CurrentEntry())) continue;
    const PadImageItem* item = CurrentItem();
    if (item != nullptr && item->mPath.IsEmpty()) continue;
    break;
  }
  EnterPhase(ConfigPhase::Wait);
}

void GuiInputConfig::StepBack()
{
  // Walk backward to the previous valid chunk.
  const auto& chunkMap = ChunkToEntryMap();
  int cur = (int)mCurrentChunk;
  while (cur > (int)PadChunks::ButtonSouth)
  {
    --cur;
    PadChunks candidate = (PadChunks)cur;
    InputDevice::Entry entry = chunkMap.get_or_return_default(candidate);
    bool firstMapper = true;
    for (int c = (int)PadChunks::ButtonSouth; c < cur; ++c)
      if (chunkMap.get_or_return_default((PadChunks)c) == entry) { firstMapper = false; break; }
    if (!firstMapper) continue;
    const PadImageItem* item = ItemForChunk(candidate);
    if (item != nullptr && item->mPath.IsEmpty()) continue;
    mCurrentChunk = candidate;
    mTargetDevice->Unset(entry);
    { LOG(LogInfo) << "[GuiInput]   Stepping back to chunk " << (int)mCurrentChunk; }
    EnterPhase(ConfigPhase::Wait);
    return;
  }
  // Already on the first chunk , refresh Wait so the blink restarts cleanly
  EnterPhase(ConfigPhase::Wait);
}

void GuiInputConfig::Update(int deltaTime)
{
  mPhaseElapsedMs += deltaTime;
  // Display intro animation
  if (mGlobalState == GlobalState::Configuring && mConfigPhase == ConfigPhase::Display && mDisplayWidthCoef > 0.f)
  {
    mDisplayWidthCoef *= sDisplayIntroDecay;
    if (mDisplayWidthCoef < sDisplayIntroFloor) mDisplayWidthCoef = 0.f;
  }
  // HighlightTesting reads mTargetDevice; during WaitingProxy that pointer is
  // dangling (kernel grab → REMOVED → ClearAllConfigurations), so skip both
  // highlight paths and just refresh the instruction text.
  if (mGlobalState == GlobalState::Configuring)        HighlightImage();
  else if (mGlobalState == GlobalState::Testing)       HighlightTesting();
  UpdateInstructions();

  // R.G.P. proxification poll: write happens once the proxy SDL device shows up.
  if (mGlobalState == GlobalState::WaitingProxy)
  {
    mProxyWaitMs += deltaTime;
    mProxyPollMs += deltaTime;
    if (mProxyPollMs >= sProxyPollIntervalMs)
    {
      mProxyPollMs = 0;
      int proxyIdx = RGPProxy::FindProxyForUid(mSourceUid);
      { LOG(LogDebug) << "[GuiInput][RGP] poll @" << mProxyWaitMs << "ms uid=" << mSourceUid
                     << " proxyIdx=" << proxyIdx; }
      if (proxyIdx >= 0) { FinalizeProxification(proxyIdx); return; }
    }
    if (mProxyWaitMs >= sProxyWaitTimeoutMs) { ProxificationTimeout(); return; }
    UpdateFooter(_("Activating R.G.P. proxy..."));
    return;
  }

  // Long-press Start in Testing has full priority: skip the cancel mechanic entirely.
  if (mStartHoldMs >= 0)
  {
    mStartHoldMs += deltaTime;
    if (mStartHoldMs >= sTestingLongPressMs)
    {
      if (mTestOnly) { Close(); return; }
      { LOG(LogDebug) << "[GuiInput][RGP] LongPress validate — ShouldProxify=" << (ShouldProxify() ? "true" : "false")
                     << " Select=" << (mTargetDevice->IsSet(InputDevice::Entry::Select) ? "set" : "unset")
                     << " Hotkey=" << (mTargetDevice->IsSet(InputDevice::Entry::Hotkey) ? "set" : "unset"); }
      if (ShouldProxify()) { StartProxification(); return; }
      InputManager::Instance().WriteDeviceXmlConfiguration(mTargetDevice->SetPadType(mTargetType));
      if (mCallback != nullptr) mCallback->DeviceConfigured(*mTargetDevice);
      Close();
      return;
    }
    UpdateFooter(_("Validating..."));
  }
  else if (mTestOnly)
  {
    // Test mode: long-press START is the ONLY way out. The 5s cancel-on-hold
    // would revert bindings to mPreviousConfig — irrelevant here since we
    // don't save anything.
    UpdateFooter(_("Hold START to exit"));
  }
  else
  {
    String msg = _("Press any button for 5s to cancel !");
    for(int& v : mButtonReferenceTime)
      if (v > 0)
        if (v -= deltaTime; v < sCancelTime - sCancelShowTime)
        {
          msg = _("Cancelling...");
          if (v <= 0)
          {
            v = 0;
            mTargetDevice->LoadFrom(mPreviousConfig);
            mCancelled = true;
          }
        }
    if (mCancelled) msg = _("CANCELLED! Please release all buttons.");
    UpdateFooter(msg);
  }

  // Phase timeouts:
  //   Display     =>> advance to next chunk (or Testing)
  //   AlreadyUsed => back to Wait, user retries on the same chunk
  if (mGlobalState == GlobalState::Configuring && mPhaseElapsedMs >= sDisplayPhaseTime)
  {
    if (mConfigPhase == ConfigPhase::Display)          AdvanceChunk();
    else if (mConfigPhase == ConfigPhase::AlreadyUsed) EnterPhase(ConfigPhase::Wait);
  }
}

void GuiInputConfig::Render(const Transform4x4f& parentTrans)
{
  Gui::Render(parentTrans);

  Transform4x4f trans = (parentTrans * getTransform()).round();
  Renderer::SetMatrix(trans);

  // Display/AlreadyUsed overlay drawn directly so we can apply a 5-pass outline.
  if (mGlobalState == GlobalState::Configuring &&
      (mConfigPhase == ConfigPhase::Display || mConfigPhase == ConfigPhase::AlreadyUsed) &&
      !mLastCapturedText.empty())
  {
    if (NewFont* font = ThemeManager::Instance().Menu().Title().font; font != nullptr)
    {
      Rectangle base(mGrid.getColWidth(0), mGrid.getRowHeight(0, 2), mGrid.getColWidth(1), mGrid.getRowHeight(3));
      bool animating = mConfigPhase == ConfigPhase::Display && mDisplayWidthCoef > 0.f;

      FontEffect effect;
      if (animating) effect.SetHScaling(1.f + mDisplayWidthCoef);

      // Alpha ramps 0 => 255 as the coef decays from initial to 0
      int alpha = animating
        ? (int)(255.f * (1.f - mDisplayWidthCoef / sDisplayIntroInitialCoef))
        : 255;
      if (alpha < 0) alpha = 0; else if (alpha > 255) alpha = 255;

      Colors::ColorARGB white  = ((unsigned)alpha) | 0xFFFFFF00;
      Colors::ColorARGB centre = (mConfigPhase == ConfigPhase::AlreadyUsed)
        ? (((unsigned)alpha) | 0xFF000000)   // red
        : (((unsigned)alpha) | 0x00000000);  // black

      font->DrawText(mLastCapturedText, base, -1.f,  0.f, white,  ::Alignment::Center, effect);
      font->DrawText(mLastCapturedText, base,  1.f,  0.f, white,  ::Alignment::Center, effect);
      font->DrawText(mLastCapturedText, base,  0.f, -1.f, white,  ::Alignment::Center, effect);
      font->DrawText(mLastCapturedText, base,  0.f,  1.f, white,  ::Alignment::Center, effect);
      font->DrawText(mLastCapturedText, base,  0.f,  0.f, centre, ::Alignment::Center, effect);
    }
  }

  // Long-press Start validate (Testing) preempts the cancel bar with a green progress bar.
  if (mGlobalState == GlobalState::Testing && mStartHoldMs >= 0)
  {
    Rectangle area(mGrid.getColWidth(0), mGrid.getRowHeight(0, 3),
                   (mGrid.getColWidth(1) * (float)mStartHoldMs) / (float)sTestingLongPressMs,
                   mGrid.getRowHeight(4));
    Renderer::DrawRectangle(area, 0x00FF0080);
    return;
  }

  int bar = 0;
  for(int& v : mButtonReferenceTime)
    if (v != 0 && v < sCancelTime - sCancelShowTime)
      bar = sCancelTime - v;

  if (bar != 0 || mCancelled)
  {
    Rectangle area(mGrid.getColWidth(0), mGrid.getRowHeight(0, 3), (mGrid.getColWidth(1) * (mCancelled ? sCancelTime :  (float)bar)) / (float)sCancelTime, mGrid.getRowHeight(4));
    Renderer::DrawRectangle(area, 0xFF000080);
  }
}

