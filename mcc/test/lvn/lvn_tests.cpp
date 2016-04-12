#include <gtest/gtest.h>
#include "mcc/tac/tac.h"
#include "mcc/lvn/lvn.h"
#include "ast.h"
#include "parser.h"

using namespace mcc::tac;

namespace mcc {
  namespace lvn {
      TEST(LVN, ConstantExpression) {
       auto tree = parser::parse(R"(
        {
            float x = 3.5 - 1.5; 
            int y = 5 + 3;
            int z = 8 * 5;

        })");

       
      Tac tac;
      tac.convertAst(tree);
      LVN::transform(tac);
      
       std::string expectedValue = "x0:1:0 = 2.000000\n";
       expectedValue.append("y0:1:0 = 8\n");
       expectedValue.append("z0:1:0 = 40");
      
      EXPECT_EQ(expectedValue, tac.toString());
      EXPECT_EQ(3, tac.codeLines.size());    
    }
      
      TEST(LVN, RedundantExpression) {
       auto tree = parser::parse(R"(
        {   
            int x = 5;
            int y = x + 12; 
            int z = x + 12;

        })");

       
      Tac tac;
      tac.convertAst(tree);
      LVN::transform(tac);
      
       std::string expectedValue = "x0:1:0 = 5\n";
       expectedValue.append("y0:1:0 = x0:1:0 + 12\n");
       expectedValue.append("z0:1:0 = y0:1:0");
      
      EXPECT_EQ(expectedValue, tac.toString());
      EXPECT_EQ(3, tac.codeLines.size());    
    }
   
  }
}

