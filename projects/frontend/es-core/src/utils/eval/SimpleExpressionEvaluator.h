//
// Created by bkg2k on 22/10/24.
//
#pragma once

#include <utils/String.h>
#include "SimpleTokenizer.h"

class SimpleExpressionEvaluator
{
  public:
    /*!
     * @brief Identifier evaluator interface
     */
    class IIdentifierEvaluator
    {
      public:
        //! default destructor
        virtual ~IIdentifierEvaluator() = default;

        /*!
         * @brief Must return a boolean evaluation of the given identifier
         * @param identifier String identifier to evaluate
         * @return True or false depending of the identifier evaluation
         */
        virtual bool EvaluateIdentifier(const String& identifier) = 0;
    };

    /*!
     * @brief Constructor
     * @param identifierEvaluator Interface to
     */
    explicit SimpleExpressionEvaluator(IIdentifierEvaluator& identifierEvaluator)
      : mEvaluator(identifierEvaluator)
    {}

    /*!
     * @brief Evaluate the given expression
     * @param expression Expression to evaluate
     * @return Expression evaluation result
     */
    bool Evaluate(const String& expression)
    {
      SimpleTokenizer tokenizer(expression);
      return Evaluate(tokenizer, 0);
    }

  private:
    //! Identifier evaluator interface
    IIdentifierEvaluator& mEvaluator;

    /*!
     * @brief Evaluate the given expression
     * @param tokenizer Tokenized expression
     * @param level Parenthesis level
     * @return Expression evaluation result
     */
    bool Evaluate(SimpleTokenizer& tokenizer, int level);
};
