#include "mcc/gas/gas.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>
#include <typeinfo>

#include "mcc/gas/x86_instruction_set.h"
#include "mcc/tac/float_literal.h"
#include "mcc/tac/float_literal.h"
#include "mcc/tac/helper/ast_converters.h"
#include "mcc/tac/int_literal.h"
#include "mcc/tac/operator.h"
#include "mcc/tac/triple.h"
#include "mcc/tac/variable.h"

namespace mcc {

namespace gas {

Gas::Gas(Tac tac) : functionMap(tac.getFunctionMap()) {
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
              auto found =
                  functionArgSizeMap->find(currentFunctionLabel->getName());

              if (found != functionArgSizeMap->end()) {
                found->second = found->second + codeLine->getArg1()->getSize();
              } else {
                (*functionArgSizeMap)[currentFunctionLabel->getName()] =
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

        void Gas::setFunctionStackSpace(std::string functionName,
                                        unsigned stackSpace) {
          assert(functionMap->find(functionName) != functionMap->end() &&
                 "Function not declared!");
          (*functionStackSpaceMap)[functionName] = stackSpace;
        }

        void Gas::setFunctionStackSpace(Label::ptr_t functionLabel,
                                        unsigned stackSpace) {
          assert(functionLabel->isFunctionEntry() && "Not a function label!");
          setFunctionStackSpace(functionLabel->getName(), stackSpace);
        }

        std::shared_ptr<function_stack_space_map_type>
        Gas::getFunctionStackSpaceMap() {
          return this->functionStackSpaceMap;
        }

        std::shared_ptr<variable_stack_offset_map_type>
        Gas::getVariableStackOffsetMap() {
          return this->variableStackOffsetMap;
        }

        unsigned Gas::lookupFunctionArgSize(std::string functionName) {
          auto found = functionArgSizeMap->find(functionName);

          if (found != functionArgSizeMap->end()) {
            return found->second;
          } else {
            return 0;
          }
        }

        unsigned Gas::lookupFunctionStackSize(std::string functionName) {
          auto found = functionStackSpaceMap->find(functionName);

          if (found != functionStackSpaceMap->end()) {
            return found->second;
          } else {
            return 0;
          }
        }

        unsigned Gas::lookupVariableStackOffset(Variable::ptr_t var,
                                                std::string functionName) {
          auto found = variableStackOffsetMap->find(var);

          if (found != variableStackOffsetMap->end()) {
            return found->second + lookupFunctionArgSize(functionName) + 4;
          } else {
            return 0;
          }
        }

        void Gas::convertTac(Tac &tac) {
          this->analyzeTac(tac);
          Label::ptr_t currentFunction = std::make_shared<Label>();

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
                convertLabel(triple, currentFunction);
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
                convertReturn(triple, currentFunction);
                break;
            }
            }
        }

        void Gas::convertLabel(Triple::ptr_t triple,
                               Label::ptr_t currentFunction) {
          auto labelTriple = std::static_pointer_cast<Label>(triple);

          auto label = std::make_shared<Mnemonic>(labelTriple->getName());
          asmInstructions.push_back(label);

          // Add function entry
          if (labelTriple->isFunctionEntry()) {
            *currentFunction.get() = *labelTriple.get();

            auto ebp = std::make_shared<Operand>(Register::EBP);
            auto esp = std::make_shared<Operand>(Register::ESP);

            asmInstructions.push_back(
                std::make_shared<Mnemonic>(Instruction::PUSH, ebp));
            asmInstructions.push_back(
                std::make_shared<Mnemonic>(Instruction::MOV, ebp, esp));

            // Make space on stack for variables
            unsigned stackSize =
                lookupFunctionStackSize(labelTriple->getName());

            if (stackSize > 0) {
              auto stackspaceOp = std::make_shared<Operand>(stackSize);
              asmInstructions.push_back(std::make_shared<Mnemonic>(
                  Instruction::SUB, esp, stackspaceOp));
            }
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
              unsigned argSize = lookupFunctionArgSize(label->getName());

              if (argSize > 0) {
                auto esp = std::make_shared<Operand>(Register::ESP);
                auto stackspaceOp = std::make_shared<Operand>(argSize);

                asmInstructions.push_back(std::make_shared<Mnemonic>(
                    Instruction::ADD, esp, stackspaceOp));
              }
            }
          }
        }

        void Gas::convertReturn(Triple::ptr_t triple,
                                Label::ptr_t currentFunction) {
          auto eax = std::make_shared<Operand>(Register::EAX);
          auto ebp = std::make_shared<Operand>(Register::EBP);
          auto esp = std::make_shared<Operand>(Register::ESP);

          if (triple->containsArg1()) {
            auto op = triple->getArg1();

            if (helper::isType<IntLiteral>(op)) {
              auto intOp = std::static_pointer_cast<IntLiteral>(op);
              auto asmInt = std::make_shared<Operand>(intOp->value);

              asmInstructions.push_back(
                  std::make_shared<Mnemonic>(Instruction::MOV, eax, asmInt));

            } else if (helper::isType<FloatLiteral>(op)) {
              /*TODO*/

            } else if (helper::isType<Variable>(op)) {
              auto variableOp = std::static_pointer_cast<Variable>(op);
              unsigned varOffset = lookupVariableStackOffset(
                  variableOp, currentFunction->getName());
              auto asmVar = std::make_shared<Operand>(Register::EBP, varOffset);

              asmInstructions.push_back(
                  std::make_shared<Mnemonic>(Instruction::MOV, eax, asmVar));
            }
          }

          unsigned stackSize =
              lookupFunctionStackSize(currentFunction->getName());

          // Cleanup stack
          if (stackSize > 0) {
            auto stackspaceOp = std::make_shared<Operand>(stackSize);
            asmInstructions.push_back(std::make_shared<Mnemonic>(
                Instruction::ADD, esp, stackspaceOp));
          }

          asmInstructions.push_back(
              std::make_shared<Mnemonic>(Instruction::POP, ebp));

          asmInstructions.push_back(
              std::make_shared<Mnemonic>(Instruction::RET));
        }

        std::string Gas::toString() const {
          std::ostringstream stream;

          for (auto mnemonic : asmInstructions) {
            stream << mnemonic << "\n";
          }

          return stream.str();
        }
        }
}
