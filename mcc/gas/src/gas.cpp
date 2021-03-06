#include "mcc/gas/gas.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <memory>
#include <ostream>
#include <typeinfo>

#include "mcc/gas/x86_instruction_set.h"
#include "mcc/tac/array.h"
#include "mcc/tac/array_access.h"
#include "mcc/tac/float_literal.h"
#include "mcc/tac/helper/ast_converters.h"
#include "mcc/tac/int_literal.h"
#include "mcc/tac/operator.h"
#include "mcc/tac/triple.h"
#include "mcc/tac/type.h"
#include "mcc/tac/variable.h"

#include "mcc/gas/helper/tac_converters.h"

namespace mcc {
namespace gas {

Gas::Gas(Tac& tac) {
  this->constantFloatsMap = std::make_shared<constant_floats_map_type>();
  this->registerManager = std::make_shared<RegisterManager>(tac);

  helper::convertTac(this, tac);
}

Operand::ptr_t Gas::loadOperand(Label::ptr_t functionLabel,
                                mcc::tac::Operand::ptr_t op,
                                Operand::ptr_t reg) {
  Operand::ptr_t returnOp;
  bool regSet = reg ? true : false;
  if (tac::helper::isType<Variable>(op)) {
    auto variableOp = std::static_pointer_cast<Variable>(op);
    returnOp = this->registerManager->getLocationForVariable(functionLabel,
                                                             variableOp);
  } else if (tac::helper::isType<Triple>(op)) {
    auto triple = std::static_pointer_cast<Triple>(op);
    auto variableOp = triple->getTargetVariable();
    returnOp = this->registerManager->getLocationForVariable(functionLabel,
                                                             variableOp);
  } else if (tac::helper::isType<Array>(op)) {
    auto arrOp = std::static_pointer_cast<Array>(op);
    returnOp = this->registerManager->getLocationForArray(functionLabel, arrOp);
  } else if (tac::helper::isType<ArrayAccess>(op)) {
    auto arrAcc = std::static_pointer_cast<ArrayAccess>(op);
    if (!regSet) {
      reg = this->registerManager->getTmpRegister();
    }

    auto arrOffsetOp = this->loadOperand(functionLabel, arrAcc->getPos());

    auto arrTypeSize = this->registerManager->getSize(arrAcc->getType());
    auto arrTypeSizeOp = std::make_shared<Operand>(std::to_string(arrTypeSize));

    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, reg, arrOffsetOp));
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::IMUL, reg, arrTypeSizeOp));

    auto arrStartOp = this->loadOperand(functionLabel, arrAcc->getArray());
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::SUB, reg, arrStartOp));
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::NEG, reg));

    auto arrAccOp = std::make_shared<Operand>(reg, 0);

    returnOp = arrAccOp;
  } else {
    // constant values
    if (op->getType() == Type::FLOAT) {
      auto floatConstant = createFloatConstant(op->getValue());

      returnOp = std::make_shared<Operand>(floatConstant);
    } else {
      returnOp = std::make_shared<Operand>(op->getValue());
    }
  }

  if (regSet) {
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, reg, returnOp));
    return reg;
  }

  return returnOp;
}

Operand::ptr_t Gas::loadOperandToRegister(Label::ptr_t functionLabel,
                                          mcc::tac::Operand::ptr_t op,
                                          Operand::ptr_t reg) {
  return loadOperand(functionLabel, op, reg);
}

Operand::ptr_t Gas::storeOperandFromRegister(Label::ptr_t functionLabel,
                                             mcc::tac::Operand::ptr_t op,
                                             Operand::ptr_t reg) {
  auto operand = loadOperand(functionLabel, op);
  auto tmpReg = reg;
  if ((operand->isAddress() && reg->isAddress()) ||
      (operand->isAddress() && reg->isFloatConstant())) {
    tmpReg = this->registerManager->getTmpRegister();
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, tmpReg, reg));
  }

  asmInstructions.push_back(
      std::make_shared<Mnemonic>(Instruction::MOV, operand, tmpReg));

  return operand;
}

void Gas::storeVariableFromFloatRegister(Label::ptr_t functionLabel,
                                         Variable::ptr_t var) {
  auto stackVar = loadOperand(functionLabel, var);
  auto asmVar =
      this->registerManager->getLocationForVariable(functionLabel, var);

  if (asmVar->isAddress()) {
    auto tmp = this->registerManager->getTmpRegister();
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::FSTP, stackVar));
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, tmp, stackVar));
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, asmVar, tmp));
  } else {
    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::FSTP, stackVar));

    asmInstructions.push_back(
        std::make_shared<Mnemonic>(Instruction::MOV, asmVar, stackVar));
  }
}

void Gas::storeRegisters(std::initializer_list<Register> list) {
  this->storeRegisters(list, asmInstructions.size());
}

void Gas::storeRegisters(std::initializer_list<Register> list, unsigned pos) {
  auto it = asmInstructions.begin() + pos;
  std::vector<Mnemonic::ptr_t> ops;

  for (auto reg : list) {
    auto regOp = std::make_shared<Operand>(reg);
    ops.push_back(std::make_shared<Mnemonic>(Instruction::PUSH, regOp));
  }

  asmInstructions.insert(it, ops.begin(), ops.end());
}

void Gas::restoreRegisters(std::initializer_list<Register> list) {
  this->restoreRegisters(list, asmInstructions.size());
}

void Gas::restoreRegisters(std::initializer_list<Register> list, unsigned pos) {
  auto it = asmInstructions.begin() + pos;
  std::vector<Mnemonic::ptr_t> ops;

  for (auto reg : list) {
    auto regOp = std::make_shared<Operand>(reg);
    ops.push_back(std::make_shared<Mnemonic>(Instruction::POP, regOp));
  }

  asmInstructions.insert(it, ops.begin(), ops.end());
}

void Gas::pushOperandToFloatRegister(Label::ptr_t functionLabel,
                                     mcc::tac::Operand::ptr_t op) {
  assert(op->getType() == Type::FLOAT && "Variable is not of type float!");

  Operand::ptr_t asmVar;
  if (tac::helper::isType<Variable>(op)) {
    auto var = std::static_pointer_cast<Variable>(op);
    asmVar = loadOperand(functionLabel, var);
  } else if (tac::helper::isType<Triple>(op)) {
    auto triple = std::static_pointer_cast<Triple>(op);
    auto var = triple->getTargetVariable();
    asmVar = loadOperand(functionLabel, var);
  } else {
    // constant values
    auto floatConstant = createFloatConstant(op->getValue());
    asmVar = std::make_shared<Operand>(floatConstant);
  }

  asmInstructions.push_back(
      std::make_shared<Mnemonic>(Instruction::FLD, asmVar));
}

std::pair<std::string, std::string> Gas::createFloatConstant(
    std::string value) {
  auto constName = ".FC" + std::to_string(constantFloatsMap->size());
  auto floatConstant = std::make_pair(constName, value);
  constantFloatsMap->insert(floatConstant);

  return floatConstant;
}

void Gas::addMnemonic(Mnemonic::ptr_t mnemonic) {
  asmInstructions.push_back(mnemonic);
}

std::shared_ptr<RegisterManager> Gas::getRegisterManager() {
  return registerManager;
}

std::vector<Mnemonic::ptr_t>& Gas::getAsmInstructions() {
  return asmInstructions;
}

std::string Gas::toString() const {
  std::ostringstream stream;

  stream << ".intel_syntax noprefix" << std::endl;
  stream << ".global main" << std::endl;

  for (auto mnemonic : asmInstructions) {
    stream << mnemonic << std::endl;
  }

  stream << std::endl;
  for (auto floatConstant : *constantFloatsMap.get()) {
    auto varName = floatConstant.first;
    auto value = floatConstant.second;
    stream << varName << ": .float " << value << std::endl;
  }

  stream << std::endl << ".att_syntax noprefix" << std::endl;

  return stream.str();
}

std::ostream& operator<<(std::ostream& os, const mcc::gas::Gas& gas) {
  os << gas.toString();

  return os;
}
}
}
