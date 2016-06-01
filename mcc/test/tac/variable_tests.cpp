#include <gtest/gtest.h>

#include <string>

#include "mcc/tac/variable.h"

namespace mcc {
namespace tac {
TEST(Variable, Id) {
  Variable v1 = Variable(Type::FLOAT);
  Variable v2 = Variable(Type::FLOAT);

  EXPECT_EQ(v1.getId() + 1, v2.getId());
}

TEST(Variable, Leaf) {
  Variable v = Variable(Type::INT);
  EXPECT_EQ(true, v.isLeaf());
}

TEST(Variable, Type) {
  Variable v = Variable(Type::INT);
  EXPECT_EQ(Type::INT, v.getType());
}

TEST(Variable, Value) {
  std::string testValue = "testVar0:0:0";

  Variable v = Variable(Type::FLOAT, "testVar", std::make_shared<Scope>(0, 0));

  EXPECT_EQ(testValue, v.getValue());
}

TEST(Variable, Name) {
  Variable v1 = Variable(Type::FLOAT);

  EXPECT_EQ("$t" + std::to_string(v1.getId()), v1.getName());
}

TEST(Variable, IsTemporary) {
  Variable v1 = Variable(Type::FLOAT);

  EXPECT_EQ(true, v1.isTemporary());

  Variable v2 = Variable(Type::FLOAT, "testVar", std::make_shared<Scope>(0, 0));

  EXPECT_EQ(false, v2.isTemporary());
}

TEST(Variable, IsArray) {
  Variable v1 = Variable(Type::FLOAT);

  EXPECT_EQ(false, v1.isArray());
}
}
}
