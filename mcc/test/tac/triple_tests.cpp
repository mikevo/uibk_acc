#include <gtest/gtest.h>

#include <string>

#include "mcc/tac/int_literal.h"
#include "mcc/tac/operator.h"
#include "mcc/tac/triple.h"

namespace mcc {
namespace tac {

TEST(Triple, ID_Test) {
  IntLiteral::ptr_t i = std::make_shared<IntLiteral>(42);
  IntLiteral::ptr_t j = std::make_shared<IntLiteral>(42);
  Operator op = Operator(OperatorName::NOT);

  Triple t1 = Triple(op, i);
  Triple t2 = Triple(op, j);

  EXPECT_LT(0, t1.getId());
  EXPECT_EQ(t1.getId() + 1, t2.getId());
}

TEST(Triple, Leaf) {
  IntLiteral::ptr_t i = std::make_shared<IntLiteral>(42);
  Operator op = Operator(OperatorName::NOT);

  Triple t1 = Triple(op, i);

  EXPECT_EQ(false, t1.isLeaf());
}

TEST(Triple, BBDefaultId) {
  IntLiteral::ptr_t i = std::make_shared<IntLiteral>(42);
  Operator op = Operator(OperatorName::NOT);

  Triple t = Triple(op, i);

  EXPECT_EQ(0, t.getBasicBlockId());
}

TEST(Triple, Value) {
  IntLiteral::ptr_t i = std::make_shared<IntLiteral>(42);
  Operator op = Operator(OperatorName::NOT);

  Triple t = Triple(op, i);

  auto id = t.getTargetVariable()->getId();

  EXPECT_EQ("$t" + std::to_string(id), t.getValue());
}

TEST(Triple, Type) {
  IntLiteral::ptr_t i = std::make_shared<IntLiteral>(42);
  Operator op = Operator(OperatorName::NOT);

  Triple t = Triple(op, i);

  EXPECT_EQ(Type::INT, t.getType());
}
}
}
