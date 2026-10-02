
#pragma once

#include <guis/menus/base/Menu.h>
#include "views/ISoftPatchingNotifier.h"

class MenuSearchLicenceSelector : public Menu
                               , private IActionTriggered
{
  public:
    MenuSearchLicenceSelector(WindowManager& window,
                              SystemManager& systemManager,
                              FileData& game,
                              std::vector <String> licences);

  private:
    std::vector <String> mLicences;

    //! System manager reference
    SystemManager& mSystemManager;

    //! System reference
    SystemData* mSystemData;

    //! Build menu items
    void BuildMenuItems() final;

    //! Build footer string
    static String GetFooter(FileData& game);

    /*
     * IActionTriggered imlplementation
     */
    void MenuActionTriggered(ItemAction& item, int id) final;
};



