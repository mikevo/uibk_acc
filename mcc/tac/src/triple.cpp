#include "mcc/tac/triple.h"

#include <cassert>
#include <typeinfo>

#include "mcc/tac/variable.h"

namespace mcc {
  namespace tac {
    unsigned Triple::nextId = 0;

    Triple::Triple(std::shared_ptr<Operand> arg) :
    Triple(Operator(OperatorName::NOP), arg) {
      // TODO: check if it is a terminal
    }

    Triple::Triple(Operator op, std::shared_ptr<Operand> arg) :
    Triple(op, arg, NULL) {
      // TODO: check if it is an unary operator;
    }

    Triple::Triple(Operator op, std::shared_ptr<Operand> arg1,
            std::shared_ptr<Operand> arg2) : 
    Operand(arg1->getType()),
    arg1(arg1),
    arg2(arg2),
    op(op),
    basicBlockId(0),
    id(0) {
      this->updateResultType(op);

      if (arg1 == NULL) {
        assert(false && "Missing argument");
      }

      if (arg2 != NULL) {
        assert((arg1->getType() == arg2->getType()) && "Type miss match");
      }

      if (typeid (*arg1.get()) == typeid (Variable)) {
        this->name = arg1.get()->getValue();
      } else if (name.empty()) {
        id = ++nextId;
        this->name = "$t" + std::to_string(id);
      } else {
        this->name = name;
      }
    }
    
    bool Triple::isLeaf() const {
      return false;
    }

    unsigned Triple::getId() const {
      return id;
    }

    std::string Triple::getName() const {
      return name;
    }
    
    void Triple::setName(const std::string name) {
      this->id = 0;
      this->name = name;
    }

    std::string Triple::getValue() const {
      return getName();
    }

    std::string Triple::toString() const {
      std::string output;

      if (name != arg1.get()->getValue()) {
        output.append(name);
        auto assignOp = Operator(OperatorName::ASSIGN);
        output.append(assignOp.toString());
      }

      if (op.getType() != OperatorType::BINARY) {
        output.append(op.toString());
        output.append(arg1.get()->getValue());
      } else if (arg2 == NULL) {
        output.append(arg1.get()->getValue());
      }

      if (arg2 != NULL) {
        output.append(arg1.get()->getValue());
        output.append(op.toString());
        output.append(arg2.get()->getValue());
      }

      return output;
    }
  }
}