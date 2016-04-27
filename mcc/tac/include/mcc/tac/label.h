/*
 * File:   triple.h
 *
 * Created on March 18, 2016, 11:02 PM
 */

#ifndef MCC_TAC_LABEL_H
#define MCC_TAC_LABEL_H

#include <string>

#include "mcc/tac/triple.h"

#include <memory>

namespace mcc {
namespace tac {

class Label : public Triple {
 public:
  typedef std::shared_ptr<Label> ptr_t;

  Label();
  virtual bool isLeaf() const override final;
};
}
}

#endif /* MCC_TAC_LABEL_H */
