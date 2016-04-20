#include <gtest/gtest.h>

#include "mcc/tac/tac.h"

#include <memory>
#include <string>

#include "ast.h"
#include "parser.h"
#include "mcc/tac/type.h"
#include "test_utils.h"

namespace mcc {
  namespace tac {

    TEST(VarTable, VariableLink) {
      auto tree =
          parser::parse(
              R"(
        {
            float x=1.0;
            float y = 3.0;
            x + y;

        })");

      Tac tac = Tac(tree);

      EXPECT_EQ(tac.getVarTable().size(), 3);

      auto x1 = std::static_pointer_cast<Variable>(tac.codeLines[0]->getArg1());
      auto y1 = std::static_pointer_cast<Variable>(tac.codeLines[1]->getArg1());

      auto x2 = std::static_pointer_cast<Variable>(tac.codeLines[2]->getArg1());
      auto y2 = std::static_pointer_cast<Variable>(tac.codeLines[2]->getArg2());

      EXPECT_EQ(x1.get(), x2.get());
      EXPECT_EQ(y1.get(), y2.get());
    }

    TEST(VarTable, VariableShadowing) {
      auto tree =
          parser::parse(
              R"(
        {
            int x=1;

            {
                float x=5.0;
                
                {
                    float x=x+x; 
                }
                
                
            }
        })");

      Tac tac = Tac(tree);

      EXPECT_EQ(tac.getVarTable().size(), 3);

      std::cout << tac.toString() << std::endl;

      auto x1 = std::static_pointer_cast<Variable>(tac.codeLines[0]->getArg1());
      auto x2 = std::static_pointer_cast<Variable>(tac.codeLines[1]->getArg1());
      auto x3 = std::static_pointer_cast<Variable>(tac.codeLines[2]->getArg1());
      auto x4 = std::static_pointer_cast<Variable>(tac.codeLines[2]->getArg2());

      EXPECT_NE(x1.get(), x2.get());
      EXPECT_EQ(x3.get(), x4.get());
      EXPECT_EQ(x2.get(), x3.get());
      EXPECT_EQ(x2.get(), x4.get());

      EXPECT_EQ(Type::INT, x1->getType());
      EXPECT_EQ(Type::FLOAT, x2->getType());
      EXPECT_EQ(Type::FLOAT, x3->getType());
    }

    TEST(VarTable, ScopeIndex) {
      auto tree =
          parser::parse(
              R"(
        {
            int x = 5;

           {
                float x = 3.0;
           }

           {
                int x = 4;
                int y = x;
                
           }

           {
                x = 10;
           }
        })");

      Tac tac = Tac(tree);

      EXPECT_EQ(tac.getVarTable().size(), 5);

      auto x1 = std::static_pointer_cast<Variable>(tac.codeLines[0]->getArg1());
      auto x2 = std::static_pointer_cast<Variable>(tac.codeLines[1]->getArg1());
      auto x3 = std::static_pointer_cast<Variable>(tac.codeLines[2]->getArg1());
      auto x4 = std::static_pointer_cast<Variable>(tac.codeLines[3]->getArg2());
      auto x5 = std::static_pointer_cast<Variable>(tac.codeLines[4]->getArg1());

      EXPECT_EQ(x3.get(), x4.get());
      EXPECT_NE(x1.get(), x2.get());
      EXPECT_NE(x1.get(), x3.get());
      EXPECT_NE(x2.get(), x3.get());
      EXPECT_NE(x1.get(), x5.get());

      EXPECT_EQ(Type::INT, x1->getType());
      EXPECT_EQ(Type::FLOAT, x2->getType());
      EXPECT_EQ(Type::INT, x3->getType());
      EXPECT_EQ(Type::INT, x3->getType());
      EXPECT_EQ(Type::INT, x5->getType());

    }
  }
}

