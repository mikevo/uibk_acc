#include "mcc/lvn/lvn.h"

#include <cassert>
#include <unordered_map>

using namespace mcc::tac;
using namespace mcc::lvn;

namespace mcc {
namespace lvn {
    
std::map<unsigned, Triple::ptr_t> LVN::tempVarAssignments;
    
void LVN::transform(Tac &tac) {
  LVN::tempVarAssignments.clear();
  auto basicBlocks = tac.getBasicBlockIndex();
  std::unordered_map<std::string, std::shared_ptr<Operand>> valueMap;
  unsigned currentPos = 0;

  for (auto block : *basicBlocks.get()) {
    valueMap.clear();

    for (auto triple : block->getBlockMembers()) {
      ++currentPos;
      if (triple->getOperator().getType() == OperatorType::BINARY) {
        std::string valueKey = triple->getArg1()->getValue();
        valueKey.append(triple->getOperator().toString());
        valueKey.append(triple->getArg2()->getValue());

        auto value = valueMap.find(valueKey);

        if (value != valueMap.end()) {
          auto target = triple->getTargetVariable();

          updateTriple(Operator(OperatorName::ASSIGN), target, value->second,
                       *triple);

        } else {
          if (typeid(*triple->getArg1()) == typeid(IntLiteral) &&
              typeid(*triple->getArg2()) == typeid(IntLiteral)) {
            auto result = evaluateInt(*triple);
            auto target = triple->getTargetVariable();
            valueMap.insert(std::make_pair(valueKey, result));
            updateTriple(Operator(OperatorName::ASSIGN), target, result,
                         *triple);
            

          } else if (typeid(*triple->getArg1()) == typeid(FloatLiteral) &&
                     typeid(*triple->getArg2()) == typeid(FloatLiteral)) {
            auto result = evaluateFloat(*triple);
            auto target = triple->getTargetVariable();
            valueMap.insert(std::make_pair(valueKey, result));
            updateTriple(Operator(OperatorName::ASSIGN), target, result,
                         *triple);

          } else {
            auto target = addTempVarAssignment(currentPos, triple->getTargetVariable());
            valueMap.insert(std::make_pair(valueKey, target));
          }
        }
      }
    }
 
    updateTAC(tac);
    
  }
}

template <typename T>
T LVN::evaluateExpression(T arg1, T arg2, OperatorName op) {
  switch (op) {
    case OperatorName::ADD:
      return arg1 + arg2;

    case OperatorName::SUB:
      return arg1 - arg2;

    case OperatorName::MUL:
      return arg1 * arg2;

    case OperatorName::DIV:
      return arg1 / arg2;

    default:
      assert(false && "Unsupported binary operation");
  }
}

void LVN::updateTriple(Operator op, Operand::ptr_t arg1,
                       Operand::ptr_t arg2, Triple &triple) {
  triple.setArg1(arg1);
  triple.setArg2(arg2);
  triple.setOperator(op);
  triple.updateResultType(op);
}

IntLiteral::ptr_t LVN::evaluateInt(Triple &triple) {
  auto arg1 = std::static_pointer_cast<IntLiteral>(triple.getArg1());
  auto arg2 = std::static_pointer_cast<IntLiteral>(triple.getArg2());

  int val1 = arg1->value;
  int val2 = arg2->value;
  int result = evaluateExpression(val1, val2, triple.getOperator().getName());

  return std::make_shared<IntLiteral>(result);
}

FloatLiteral::ptr_t LVN::evaluateFloat(Triple &triple) {
  auto arg1 = std::static_pointer_cast<FloatLiteral>(triple.getArg1());
  auto arg2 = std::static_pointer_cast<FloatLiteral>(triple.getArg2());

  float val1 = arg1->value;
  float val2 = arg2->value;
  float result = evaluateExpression(val1, val2, triple.getOperator().getName());

  return std::make_shared<FloatLiteral>(result);
}

Variable::ptr_t LVN::addTempVarAssignment(unsigned position, 
                                          Variable::ptr_t var) {
    auto tempVar = std::make_shared<Variable>(var->getType());
    auto tempAsgnTriple = std::make_shared<Triple>(Operator(OperatorName::ASSIGN),
            tempVar, var);
    
    LVN::tempVarAssignments.insert(std::make_pair(position, tempAsgnTriple));
    
    return tempVar;
}

void LVN::updateTAC(Tac &tac) {
    unsigned numInserted = 0;
    
    for(auto& triple : LVN::tempVarAssignments) {
        tac.addLine(triple.second, triple.first + numInserted);
        ++numInserted;
    }
    
    tac.createBasicBlockIndex();
    
}
}
}
