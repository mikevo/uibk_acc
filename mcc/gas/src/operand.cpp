#include "mcc/gas/operand.h"
#include <cassert>

namespace mcc {
namespace gas {

Operand::Operand(Register reg) : mType(OperandType::REGISTER), mRegister(reg) {}
Operand::Operand(Register reg, int offset)
    : mType(OperandType::ADDRESS), mRegister(reg), mAddrOffset(offset) {}
Operand::Operand(int constant)
    : mType(OperandType::CONSTANT), mConstValue(constant) {}

std::ostream& operator<<(std::ostream& os, const mcc::gas::Operand& op) {
  switch (op.mType) {
    case OperandType::CONSTANT:
      return os << std::to_string(op.mConstValue);

    case OperandType::REGISTER:
      return os << RegisterName[op.mRegister];

    case OperandType::ADDRESS:
      os << "DWORD PTR [" << RegisterName[op.mRegister];
      if (op.mAddrOffset > 0) {
        return os << " + " << op.mAddrOffset << "]";
      } else if (op.mAddrOffset < 0) {
        return os << " - " << op.mAddrOffset << "]";
      } else {
        return os << "]";
      }
    default:
      assert(false && "Unknown assembly operand type");
      return os;
  }
}
}
}