#include <gtest/gtest.h>

#include "mcc/gas/gas.h"

#include "mcc/tac/float_literal.h"
#include "mcc/tac/helper/ast_converters.h"
#include "mcc/tac/int_literal.h"
#include "parser.h"
#include "test_utils.h"

#include <iostream>
#include <regex>

using namespace mcc::gas;

namespace mcc {
namespace gas {

TEST(Gas, FunctionStackSpace) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
         void bar(int arg1);

         void foo(int arg1) {
            if(arg1 < 0) return;
            bar(arg1 - 1);
         }

         void bar(int arg1) {
            if(arg1 < 0) return;
            foo(arg1 - 1);
            bar(arg1 - 1);
         }

          int main() {
             foo(10);
             return 0;
          }
        )");

  Tac tac = Tac(tree);
  Gas gas = Gas(tac);

  function_stack_space_map_type expected;

  auto l1 = std::make_shared<Label>("foo", scope);
  auto l2 = std::make_shared<Label>("bar", scope);
  auto l3 = std::make_shared<Label>("main", scope);

  expected[l1] = 4;
  expected[l2] = 8;
  expected[l3] = 0;

  for (auto const e : expected) {
    auto result = gas.getRegisterManager()->lookupFunctionStackSpace(e.first);

    EXPECT_EQ(e.second, result);
  }
}

TEST(Gas, VariableStackOffset) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
         void bar(int arg1);

         void foo(int arg1, int arg2) {
            if(arg1 < 0) return;
            bar(arg1 - 1);
         }

         void bar(int arg1) {
            if(arg1 < 0) return;
            foo(arg1 - 1, arg1);
            bar(arg1 - 1);
         }

          int main() {
             foo(10, 10);
             return 0;
          }
        )");

  Tac tac = Tac(tree);
  Gas gas = Gas(tac);

  auto regMan = gas.getRegisterManager();

  Label::ptr_t currentFunction;
  int varCounter = 0;
  signed expectedOffsets[] = {8, 12, -4, -4, -8, 8, -4, -4, -8, -8, -12, -4};
  for (auto codeLine : tac.codeLines) {
    if (helper::isType<Label>(codeLine)) {
      auto label = std::static_pointer_cast<Label>(codeLine);
      if (label->isFunctionEntry()) {
        currentFunction = label;
      }
    }

    if (codeLine->containsTargetVar()) {
      auto targetVar = codeLine->getTargetVariable();
      auto targetVarOffset =
          regMan->lookupVariableStackOffset(currentFunction, targetVar);

      EXPECT_EQ(expectedOffsets[varCounter++], targetVarOffset);
    }
  }
}

/* Gas Conversion */
TEST(Gas, GasConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
            void print_int(int out);
            int read_int();

            int fibonacci(int n) {
                if ( n == 0 )
                    return 0;
                else if ( n == 1 )
                    return 1;
                else
                    return (fibonacci(n-1) + fibonacci(n-2));
            } 

            int main(void) {
                int i = read_int();
                int fib = fibonacci(i); 
                print_int(fib);

                return 0;
            }
        )");

  auto tac = Tac(tree);
  Gas gas = Gas(tac);

  // get labels from tac
  auto labels = std::vector<std::string>();
  for (auto triple : tac.codeLines) {
    switch (triple->getOperator().getName()) {
      case OperatorName::LABEL: {
        auto label = std::static_pointer_cast<Label>(triple);
        if (!label->isFunctionEntry()) {
          labels.push_back(label->getValue());
        }
      } break;
      case OperatorName::JUMP: {
        auto label = std::static_pointer_cast<Label>(triple->getArg1());
        if (!label->isFunctionEntry()) {
          labels.push_back(label->getValue());
        }
      } break;
      case OperatorName::JUMPFALSE: {
        auto label = std::static_pointer_cast<Label>(triple->getArg2());
        if (!label->isFunctionEntry()) {
          labels.push_back(label->getValue());
        }
      } break;
      default:
        // ignore
        break;
    }
  }

  auto curLabel = labels.begin();

  auto expected = R"(.intel_syntax noprefix
.global main

fibonacci:
	push ebp
	mov ebp, esp
	sub esp, 20
	push ebx
	push edi
	push esi
	mov edx, DWORD PTR [ebp + 8]
	cmp edx, 0
	jne )" + *curLabel++ +
                  R"(
	mov eax, 0
	pop esi
	pop edi
	pop ebx
	add esp, 20
	mov esp, ebp
	pop ebp
	ret
	jmp )" + *curLabel++ +
                  R"(

)" + *curLabel++ +
                  R"(:
	cmp edx, 1
	jne )" + *curLabel++ +
                  R"(
	mov eax, 1
	pop esi
	pop edi
	pop ebx
	add esp, 20
	mov esp, ebp
	pop ebp
	ret
	jmp )" + *curLabel++ +
                  R"(

)" + *curLabel++ +
                  R"(:
	mov eax, edx
	sub eax, 1
	mov ecx, eax
	push ecx
	push edx
	push ecx
	call fibonacci
	add esp, 4
	pop edx
	pop ecx
	mov esi, eax
	mov ebx, edx
	sub ebx, 2
	mov ecx, ebx
	push ecx
	push edx
	push ecx
	call fibonacci
	add esp, 4
	pop edx
	pop ecx
	mov ecx, eax
	mov eax, esi
	add eax, ecx
	mov edx, eax
	mov eax, edx
	pop esi
	pop edi
	pop ebx
	add esp, 20
	mov esp, ebp
	pop ebp
	ret

)" + *curLabel++ +
                  R"(:

)" + *curLabel++ +
                  R"(:

main:
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push edi
	push esi
	push ecx
	push edx
	call read_int
	pop edx
	pop ecx
	mov ecx, eax
	push ecx
	push edx
	push ecx
	call fibonacci
	add esp, 4
	pop edx
	pop ecx
	mov ecx, eax
	push ecx
	push edx
	push ecx
	call print_int
	add esp, 4
	pop edx
	pop ecx
	mov eax, 0
	pop esi
	pop edi
	pop ebx
	add esp, 8
	mov esp, ebp
	pop ebp
	ret


.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasAddIntegerConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10 + 15;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	mov eax, 10
	add eax, 15
	mov ecx, eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret


.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasSubIntegerConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10 - 15;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	mov eax, 10
	sub eax, 15
	mov ecx, eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret


.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasMulIntegerConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10 * 15;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	mov eax, 10
	imul eax, 15
	mov ecx, eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret


.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasAddFloatConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10.0 + 15.0;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	fld DWORD PTR .FC0
	fld DWORD PTR .FC1
	faddp st(1), st
	fstp DWORD PTR [ebp - 4]
	mov eax, DWORD PTR [ebp - 4]
	mov DWORD PTR [ebp - 4], eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret

.FC0: .float 10.000000
.FC1: .float 15.000000

.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasSubFloatConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10.0 - 15.0;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	fld DWORD PTR .FC0
	fld DWORD PTR .FC1
	fsubp st(1), st
	fstp DWORD PTR [ebp - 4]
	mov eax, DWORD PTR [ebp - 4]
	mov DWORD PTR [ebp - 4], eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret

.FC0: .float 10.000000
.FC1: .float 15.000000

.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasMulFloatConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10.0 * 15.0;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	fld DWORD PTR .FC0
	fld DWORD PTR .FC1
	fmulp st(1), st
	fstp DWORD PTR [ebp - 4]
	mov eax, DWORD PTR [ebp - 4]
	mov DWORD PTR [ebp - 4], eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret

.FC0: .float 10.000000
.FC1: .float 15.000000

.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}

TEST(Gas, GasDivFloatConversion) {
  Scope::ptr_t scope = std::make_shared<Scope>(0, 0);

  auto tree = parser::parse(
      R"(
        void main() {
          10.0 / 15.0;
        }
        )");

  Tac tac = Tac(tree);

  Gas gas = Gas(tac);
  auto expected = R"(.intel_syntax noprefix
.global main

main:
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push edi
	push esi
	fld DWORD PTR .FC0
	fld DWORD PTR .FC1
	fdivp st(1), st
	fstp DWORD PTR [ebp - 4]
	mov eax, DWORD PTR [ebp - 4]
	mov DWORD PTR [ebp - 4], eax
	pop esi
	pop edi
	pop ebx
	add esp, 4
	mov esp, ebp
	pop ebp
	ret

.FC0: .float 10.000000
.FC1: .float 15.000000

.att_syntax noprefix
)";

  EXPECT_EQ(expected, gas.toString());
}
}
}
