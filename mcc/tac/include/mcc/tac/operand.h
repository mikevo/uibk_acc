/*
 * File:   operand.h
 *
 * Created on March 19, 2016, 1:44 PM
 */

#ifndef MCC_TAC_OPERAND_H
#define MCC_TAC_OPERAND_H

#include <memory>
#include <string>
#include <set>

#include "mcc/tac/operator.h"
#include "mcc/tac/type.h"

namespace mcc {
namespace tac {

class Operand {
 public:
  typedef std::shared_ptr<Operand> ptr_t;

  Operand();
  Operand(Type t);
  virtual void updateResultType(Operator op);
  virtual Type getType() const;
  virtual void setType(Type t);
  virtual std::string getValue() const = 0;
  virtual bool isLeaf() const = 0;
  virtual std::size_t length() const;

 private:
  Type t;
};
}
}

#endif /* MCC_TAC_OPERAND_H */
