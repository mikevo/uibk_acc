#include "mcc/gas/gas.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>
#include <typeinfo>

#include "mcc/gas/x86_instruction_set.h"
#include "mcc/tac/operator.h"
#include "mcc/tac/triple.h"

namespace mcc {

namespace gas {

Gas::Gas(Tac tac) {
  this->functionStackSpaceMap =
      std::make_shared<function_stack_space_map_type>();
  this->variableStackOffsetMap =
      std::make_shared<variable_stack_offset_map_type>();
  this->functionArgSizeMap = std::make_shared<function_arg_size_type>();

  this->convertTac(tac);
}

void Gas::analyzeTac(Tac &tac) {
  Label::ptr_t currentFunctionLabel = nullptr;
  unsigned stackSpace = 0;
  unsigned currentStackOffset = 0;
  for (auto codeLine : tac.codeLines) {
    auto opName = codeLine->getOperator().getName();
    if (opName == OperatorName::POP) {
      auto found = functionArgSizeMap->find(currentFunctionLabel);

      if (found != functionArgSizeMap->end()) {
        found->second = found->second + codeLine->getArg1()->getSize();
      } else {
        (*functionArgSizeMap)[currentFunctionLabel] =
            codeLine->getArg1()->getSize();
      }
    }
    if (opName == OperatorName::LABEL) {
      auto label = std::static_pointer_cast<Label>(codeLine);
      if (label->isFunctionEntry()) {
        // if new function is entered
        if (currentFunctionLabel) {
          this->setFunctionStackSpace(currentFunctionLabel, stackSpace);
          stackSpace = 0;
          currentStackOffset = 0;
        }

        currentFunctionLabel = label;
      }
    } else if (codeLine->containsTargetVar()) {
      auto targetVar = codeLine->getTargetVariable();
      (*variableStackOffsetMap)[targetVar] = currentStackOffset;
      currentStackOffset += targetVar->getSize();

      // if variable not parameter of function
      if (codeLine->getOperator().getName() != OperatorName::POP) {
        stackSpace += targetVar->getSize();
      }
    }
  }

  // add last function
  this->setFunctionStackSpace(currentFunctionLabel, stackSpace);
}
//
// void Gas::setFunctionStackSpace(std::string functionName, unsigned
// stackSpace) {
//  assert(functionMap->find(functionName) != functionMap->end() &&
//         "Function not declared!");
//  (*functionStackSpaceMap)[functionName] = stackSpace;
//}

void Gas::setFunctionStackSpace(Label::ptr_t functionLabel,
                                unsigned stackSpace) {
  assert(functionLabel->isFunctionEntry() && "Not a function label!");
  //  assert(functionMap->find(functionLabel->getName()) != functionMap->end()
  //  &&
  //           "Function not declared!");
  (*functionStackSpaceMap)[functionLabel] = stackSpace;
}

std::shared_ptr<function_stack_space_map_type> Gas::getFunctionStackSpaceMap() {
  return this->functionStackSpaceMap;
}

std::shared_ptr<variable_stack_offset_map_type>
Gas::getVariableStackOffsetMap() {
  return this->variableStackOffsetMap;
}

unsigned Gas::lookupFunctionArgSize(Label::ptr_t functionLabel) {
  auto found = functionArgSizeMap->find(functionLabel);

  if (found != functionArgSizeMap->end()) {
    return found->second;
  } else {
    return 0;
  }
}

void Gas::convertTac(Tac &tac) {
  this->analyzeTac(tac);

  for (auto triple : tac.codeLines) {
    auto op = triple->getOperator();

    switch (op.getName()) {
      case OperatorName::NOP:
        /*Ignore*/
        break;

      case OperatorName::ADD:
        break;

      case OperatorName::SUB:
        break;

      case OperatorName::MUL:
        break;

      case OperatorName::DIV:
        break;

      case OperatorName::ASSIGN:
        break;

      case OperatorName::LABEL:
        convertLabel(triple);
        break;

      case OperatorName::JUMP:
        break;

      case OperatorName::JUMPFALSE:
        break;

      case OperatorName::EQ:
      case OperatorName::NE:
      case OperatorName::LE:
      case OperatorName::GE:
      case OperatorName::LT:
      case OperatorName::GT:
        break;

      case OperatorName::MINUS:
        break;

      case OperatorName::NOT:
        break;

      case OperatorName::PUSH:
        break;

      case OperatorName::POP:
        break;

      case OperatorName::CALL:
        convertCall(triple);
        break;

      case OperatorName::RET:
        convertReturn(triple);
        break;
    }
  }
}

void Gas::convertLabel(Triple::ptr_t triple) {
  auto labelTriple = std::static_pointer_cast<Label>(triple);

  auto label = std::make_shared<Mnemonic>(labelTriple->getName());
  asmInstructions.push_back(label);

  if (labelTriple->isFunctionEntry()) {
    auto ebp = std::make_shared<Operand>(Register::EBP);
    auto esp = std::make_shared<Operand>(Register::ESP);

    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::PUSH, ebp));
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, ebp, esp));
  }
}

void Gas::convertCall(Triple::ptr_t triple) {
  if (triple->containsArg1()) {
    auto operand = triple->getArg1();
    if (typeid(*operand.get()) == typeid(Label)) {
      auto label = std::static_pointer_cast<Label>(operand);
      auto asmLabel = std::make_shared<Operand>(label->getName());

      asmInstructions.push_back(
          std::make_shared<Mnemonic>(Instruction::CALL, asmLabel));

      // Cleanup stack
      unsigned argSize = lookupFunctionArgSize(label);

      if (argSize > 0) {
        auto esp = std::make_shared<Operand>(Register::ESP);
        auto stackspaceOp = std::make_shared<Operand>(argSize);

        asmInstructions.push_back(
            std::make_shared<Mnemonic>(Instruction::ADD, esp, stackspaceOp));
      }
    }
  }
}

void Gas::convertReturn(Triple::ptr_t triple) {}

std::string Gas::toString() const {
  std::ostringstream stream;

  for (auto mnemonic : asmInstructions) {
    stream << mnemonic << "\n";
  }

  return stream.str();
}
}
}
