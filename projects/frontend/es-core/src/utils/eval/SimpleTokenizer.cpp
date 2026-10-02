//
// Created by bkg2k on 16/01/24.
//

#include "SimpleTokenizer.h"
#include "utils/Log.h"

SimpleTokenizer::Type SimpleTokenizer::Next()
{
  // Skip spaces/tabs
  while(mIndex < (int)mString.size())
    if (char c = mString[mIndex]; c == ' ' || c == '\t') mIndex++;
    else break;
  // EOS
  if (mIndex >= (int)mString.size()) return mType = Type::End;
  // Simple tokens
  int previousIndex = mIndex;
  switch(char c = mString[mIndex++])
  {
    case '(': mToken.Assign(c); return mType = Type::Open;
    case ')': mToken.Assign(c); return mType = Type::Close;
    case '!': mToken.Assign(c); return mType = Type::Not;
    case '&': mToken.Assign(c); return mType = Type::And;
    case '|': mToken.Assign(c); return mType = Type::Or;
    case 'a'...'z':
    {
      while(mIndex < (int)mString.size())
        if (c = mString[mIndex]; (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '.')) mIndex++;
        else break;
      mToken.Assign(mString, previousIndex, mIndex - previousIndex);
      return mType = (mToken == "or" ? Type::Or :
                      mToken == "and" ? Type::And :
                      mToken == "not" ? Type::Not :
                      Type::Identifier);
    }
    default: break;
  }
  mIndex = previousIndex;
  return mType = Type::Error;
}
