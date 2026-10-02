//
// Created by bkg2k on 22/10/24.
//

#include "SimpleExpressionEvaluator.h"
#include <utils/Log.h>

bool SimpleExpressionEvaluator::Evaluate(SimpleTokenizer& tokenizer, int deepLevel)
{
  bool result = false;
  bool inverter = false;
  for(SimpleTokenizer::Type previousType = tokenizer.TokenType();
      previousType != SimpleTokenizer::Type::End;
      previousType = tokenizer.Unary() ? previousType : tokenizer.TokenType())
    switch(tokenizer.Next())
    {
      case SimpleTokenizer::Type::Close:
      {
        if (deepLevel == 0)
        { LOG(LogDebug) << "[Theme] Unexpected ) at index " << tokenizer.Index() << " in conditionnal string " << tokenizer.ParsedString(); }
        return result;
      }
      case SimpleTokenizer::Type::Open:
      case SimpleTokenizer::Type::Identifier:
      {
        // Evaluate token
        bool evaluated = tokenizer.TokenType() == SimpleTokenizer::Type::Identifier ?
                         mEvaluator.EvaluateIdentifier(tokenizer.Token()) :
                         Evaluate(tokenizer, deepLevel + 1);
        // Not?
        if (inverter) evaluated = !evaluated;
        // Apply operation
        if      (previousType == SimpleTokenizer::Type::And  ) result &= evaluated;
        else if (previousType == SimpleTokenizer::Type::Or   ) result |= evaluated;
        else if (previousType == SimpleTokenizer::Type::Start ||
                 previousType == SimpleTokenizer::Type::Open) result = evaluated;
        else { LOG(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); return false; }
        break;
      }
      case SimpleTokenizer::Type::And:
      case SimpleTokenizer::Type::Or:
      {
        if (previousType != SimpleTokenizer::Type::Identifier &&
            previousType != SimpleTokenizer::Type::Close)
        { LOG(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); return false; }
        inverter = false;
        break;
      }
      case SimpleTokenizer::Type::Not:
      {
        if (previousType != SimpleTokenizer::Type::And &&
            previousType != SimpleTokenizer::Type::Or &&
            previousType != SimpleTokenizer::Type::Start &&
            previousType != SimpleTokenizer::Type::Open)
        { LOG(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); return false; }
        inverter = !inverter;
        break;
      }
      case SimpleTokenizer::Type::Error:
      {
        { LOG(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); }
        return false;
      }
      case SimpleTokenizer::Type::Start:
      {
        { LOG(LogDebug) << "[Theme] Internal logic error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); }
        return false;
      }
      case SimpleTokenizer::Type::End:
      {
        if (deepLevel != 0)
        { LOG(LogDebug) << "[Theme] Missing ) at index " << tokenizer.Index() << " in conditionnal string " << tokenizer.ParsedString(); }
        if (previousType != SimpleTokenizer::Type::Identifier &&
            previousType != SimpleTokenizer::Type::Close)
        { LOG(LogDebug) << "[Theme] Missing operand at index " << tokenizer.Index() << " in conditionnal string " << tokenizer.ParsedString(); }
        break;
      }
    }
  // Hu?
  return result;
}
