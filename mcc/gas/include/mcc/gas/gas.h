/*
 * File:   gas.h
 *
 * Created on May 12, 2016, 11:55 PM
 */

#ifndef GAS_H
#define GAS_H

#include <memory>
#include <string>
#include <vector>

#include "mcc/tac/tac.h"
#include "mnemonic.h"

using namespace mcc::tac;

namespace mcc {
namespace gas {

// typedef std::map<std::string, Label::ptr_t> function_map_type;
typedef std::map<Label::ptr_t, unsigned, Label::less>
    function_stack_space_map_type;
typedef std::map<Variable::ptr_t, unsigned> variable_stack_offset_map_type;
typedef std::map<Label::ptr_t, unsigned, Label::less> function_arg_size_type;

class Gas {
 public:
  Gas(Tac tac);
  std::shared_ptr<function_stack_space_map_type> getFunctionStackSpaceMap();
  std::shared_ptr<variable_stack_offset_map_type> getVariableStackOffsetMap();
  std::string toString() const;

  friend std::ostream& operator<<(std::ostream& os, const mcc::gas::Gas& gas);

 private:
  //  std::shared_ptr<function_map_type> functionMap;
  std::shared_ptr<function_stack_space_map_type> functionStackSpaceMap;
  std::shared_ptr<variable_stack_offset_map_type> variableStackOffsetMap;
  std::shared_ptr<function_arg_size_type> functionArgSizeMap;
  OperatorName lastOperator;

  std::vector<Mnemonic::ptr_t> asmInstructions;

  void convertTac(Tac& tac);
  void analyzeTac(Tac& tac);

  //  void setFunctionStackSpace(std::string functionName, unsigned stackSpace);
  void setFunctionStackSpace(Label::ptr_t functionLabel, unsigned stackSpace);

  unsigned lookupFunctionArgSize(Label::ptr_t functionLabel);
  unsigned lookupFunctionStackSize(Label::ptr_t functionLabel);
  unsigned lookupVariableStackOffset(Variable::ptr_t var);

  void convertLabel(Triple::ptr_t triple, Label::ptr_t currentFunction);
  void convertReturn(Triple::ptr_t triple, Label::ptr_t currentFunction);
  void convertCall(Triple::ptr_t triple);
  void convertPush(Triple::ptr_t triple);
  void convertAssign(Triple::ptr_t triple);
  void convertAddSubMul(Triple::ptr_t triple);
  void convertDiv(Triple::ptr_t triple);
  void convertLogicOperator(Triple::ptr_t triple);
  void convertJump(Triple::ptr_t triple);
  void convertJumpFalse(Triple::ptr_t triple);
};
}
}

#endif /* GAS_H */
