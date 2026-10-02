//
// Created by bkg2k on 14/12/23.
//
#pragma once

#include <components/base/Component.h>
#include <components/base/Themable.h>

class ThemableComponent : public Component
                        , public Themable
{
  public:
    explicit ThemableComponent(WindowManager& manager)
      : Component(manager)
      , Themable(this)
    {
    }
};
