//
// Created by digitallumberjack on 27/10/25.
//

#pragma once

class CardReaderListener {
  public:
      virtual void CardPlugged(String romPath, String systemUUID) = 0;
      virtual void CardUnplugged() = 0;
};