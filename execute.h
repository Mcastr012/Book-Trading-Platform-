/*
 * Copyright (C) Rida Bazzi, 2017
 *
 * Do not share this file with anyone
 */
#ifndef _COMPILER_H_
#define _COMPILER_H_

#include <string>
#include <vector>

extern int mem[1000];
extern int next_available;

extern std::vector<int> inputs;
extern int next_input;

enum ArithmeticOperatorType
{
    OPERATOR_NONE = 123,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MULT,
    OPERATOR_DIV
};

enum ConditionalOperatorType
{
    CONDITION_GREATER = 345,
    CONDITION_LESS,
    CONDITION_NOTEQUAL
};

enum InstructionType
{
    NOOP = 1000,
    IN,
    OUT,
    ASSIGN,
    CJMP,
    JMP
};

struct InstructionNode
{
    InstructionType type;

    union
    {
        struct
        {
            int lhs_loc;
            int op1_loc;
            int op2_loc;

            /*
             * If op == OPERATOR_NONE then only opernd1 is meaningful.
             * Otherwise both opernds are meaningful
             */
            ArithmeticOperatorType op;
        } assign_inst;

        struct
        {
            int var_loc;
        } input_inst;

        struct
        {
            int var_loc;
        } output_inst;

        struct
        {
            ConditionalOperatorType condition_op;
            int op1_loc;
            int op2_loc;
            struct InstructionNode *target;
        } cjmp_inst;

        struct
        {
            struct InstructionNode *target;
        } jmp_inst;
    };

    struct InstructionNode *next; // next statement in the list or NULL
};

void debug(const char *format, ...);

//---------------------------------------------------------
// You should write the following function:

struct InstructionNode *parse_Generate_Intermediate_Representation();

/*
  NOTE:

  You need to write a function with the above signature. This function
  is supposed to parse the input program and generate an intermediate
  representation for it. The output of this function is passed to the
  execute_program function in main().

  Write your code in a separate file and include this header file in
  your code.
*/

#endif /* _COMPILER_H_ */
