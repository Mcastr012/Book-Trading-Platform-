#include <iostream>
#include <cstdlib>
#include <map>
#include <vector>
#include <string>
#include "execute.h"
#include "lexer.h"

using namespace std;

LexicalAnalyzer tokenizer;
map<string, int> varLocations;

struct InstructionNode* parseProgram();
void handleVarSection();
void processIdList();
struct InstructionNode* getBodyCode();
struct InstructionNode* buildStmtList();
struct InstructionNode* handleStmt();
struct InstructionNode* createAssignNode();
void processExpression(int& first, ArithmeticOperatorType& oper, int& second);
int getPrimaryValue();
ArithmeticOperatorType getOperator();
struct InstructionNode* makeOutputNode();
struct InstructionNode* makeInputNode();
struct InstructionNode* buildWhileLoop();
struct InstructionNode* handleIfStmt();
void checkCondition(ConditionalOperatorType& cond, int& left, int& right);
ConditionalOperatorType getRelationalOp();
struct InstructionNode* processSwitch();
struct InstructionNode* createForLoop();
void readInputValues();
void processNumbers();

Token verifyToken(TokenType expected);
int findVarLocation(string name);
int storeConstant(int val);
struct InstructionNode* makeNoop();
void linkNodes(struct InstructionNode* start, struct InstructionNode* newNode);

struct InstructionNode* parse_Generate_Intermediate_Representation() {
    return parseProgram();
}

struct InstructionNode* parseProgram() {
    handleVarSection();
    struct InstructionNode* mainCode = getBodyCode();
    readInputValues();
    return mainCode;
}

void handleVarSection() {
    processIdList();
}

void processIdList() {
    Token t = verifyToken(ID);
    varLocations[t.lexeme] = next_available;
    mem[next_available] = 0;
    next_available++;
    
    Token nextToken = tokenizer.GetToken();
    if (nextToken.token_type == COMMA) {
        processIdList();
    } else if (nextToken.token_type != SEMICOLON) {
        cerr << "Unexpected token found" << endl;
        exit(1);
    }
}

struct InstructionNode* getBodyCode() {
    verifyToken(LBRACE);
    struct InstructionNode* statements = buildStmtList();
    verifyToken(RBRACE);
    return statements;
}

struct InstructionNode* buildStmtList() {
    struct InstructionNode* firstNode = handleStmt();
    
    Token peek = tokenizer.peek(1);
    if (peek.token_type == ID || peek.token_type == WHILE || peek.token_type == IF || 
        peek.token_type == SWITCH || peek.token_type == FOR || 
        peek.token_type == OUTPUT || peek.token_type == INPUT) {
        struct InstructionNode* restNodes = buildStmtList();
        
        struct InstructionNode* current = firstNode;
        while (current->next != NULL) {
            current = current->next;
        }
        
        current->next = restNodes;
    }
    
    return firstNode;
}

struct InstructionNode* handleStmt() {
    Token lookahead = tokenizer.peek(1);
    
    switch(lookahead.token_type) {
        case ID: return createAssignNode();
        case WHILE: return buildWhileLoop();
        case IF: return handleIfStmt();
        case SWITCH: return processSwitch();
        case FOR: return createForLoop();
        case OUTPUT: return makeOutputNode();
        case INPUT: return makeInputNode();
        default:
            cerr << "Invalid statement at line" << endl;
            exit(1);
    }
}

struct InstructionNode* createAssignNode() {
    struct InstructionNode* node = new InstructionNode;
    node->type = ASSIGN;
    node->next = NULL;
    
    Token var = verifyToken(ID);
    node->assign_inst.lhs_loc = findVarLocation(var.lexeme);
    
    verifyToken(EQUAL);
    
    Token next = tokenizer.peek(1);
    if ((next.token_type == ID || next.token_type == NUM) && 
        (tokenizer.peek(2).token_type == SEMICOLON)) {
        node->assign_inst.op1_loc = getPrimaryValue();
        node->assign_inst.op = OPERATOR_NONE;
    } else {
        int left, right;
        ArithmeticOperatorType op;
        processExpression(left, op, right);
        node->assign_inst.op1_loc = left;
        node->assign_inst.op2_loc = right;
        node->assign_inst.op = op;
    }
    
    verifyToken(SEMICOLON);
    return node;
}

void processExpression(int& first, ArithmeticOperatorType& oper, int& second) {
    first = getPrimaryValue();
    oper = getOperator();
    second = getPrimaryValue();
}

int getPrimaryValue() {
    Token t = tokenizer.GetToken();
    
    if (t.token_type == ID) {
        return findVarLocation(t.lexeme);
    } else if (t.token_type == NUM) {
        int value = stoi(t.lexeme);
        mem[next_available] = value;
        int loc = next_available;
        next_available++;
        return loc;
    } else {
        cerr << "Expected identifier or number" << endl;
        exit(1);
    }
}

ArithmeticOperatorType getOperator() {
    Token t = tokenizer.GetToken();
    
    switch(t.token_type) {
        case PLUS: return OPERATOR_PLUS;
        case MINUS: return OPERATOR_MINUS;
        case MULT: return OPERATOR_MULT;
        case DIV: return OPERATOR_DIV;
        default:
            cerr << "Bad operator" << endl;
            exit(1);
    }
}

struct InstructionNode* makeOutputNode() {
    struct InstructionNode* node = new InstructionNode;
    node->type = OUT;
    node->next = NULL;
    
    verifyToken(OUTPUT);
    Token var = verifyToken(ID);
    node->output_inst.var_loc = findVarLocation(var.lexeme);
    verifyToken(SEMICOLON);
    
    return node;
}

struct InstructionNode* makeInputNode() {
    struct InstructionNode* node = new InstructionNode;
    node->type = IN;
    node->next = NULL;
    
    verifyToken(INPUT);
    Token var = verifyToken(ID);
    node->input_inst.var_loc = findVarLocation(var.lexeme);
    verifyToken(SEMICOLON);
    
    return node;
}

struct InstructionNode* buildWhileLoop() {
    struct InstructionNode* condNode = new InstructionNode;
    condNode->type = CJMP;
    condNode->next = NULL;
    
    verifyToken(WHILE);
    
    ConditionalOperatorType condOp;
    int left, right;
    checkCondition(condOp, left, right);
    
    condNode->cjmp_inst.condition_op = condOp;
    condNode->cjmp_inst.op1_loc = left;
    condNode->cjmp_inst.op2_loc = right;
    
    struct InstructionNode* bodyCode = getBodyCode();
    condNode->next = bodyCode;
    
    struct InstructionNode* jumpNode = new InstructionNode;
    jumpNode->type = JMP;
    jumpNode->jmp_inst.target = condNode;
    jumpNode->next = NULL;
    
    linkNodes(bodyCode, jumpNode);
    
    struct InstructionNode* noop = makeNoop();
    
    condNode->cjmp_inst.target = noop;
    
    jumpNode->next = noop;
    
    return condNode;
}

struct InstructionNode* handleIfStmt() {
    struct InstructionNode* condNode = new InstructionNode;
    condNode->type = CJMP;
    condNode->next = NULL;
    
    verifyToken(IF);
    
    ConditionalOperatorType condOp;
    int left, right;
    checkCondition(condOp, left, right);
    
    condNode->cjmp_inst.condition_op = condOp;
    condNode->cjmp_inst.op1_loc = left;
    condNode->cjmp_inst.op2_loc = right;
    
    struct InstructionNode* bodyCode = getBodyCode();
    condNode->next = bodyCode;
    
    struct InstructionNode* noop = makeNoop();
    
    condNode->cjmp_inst.target = noop;
    
    linkNodes(bodyCode, noop);
    
    return condNode;
}

void checkCondition(ConditionalOperatorType& cond, int& left, int& right) {
    left = getPrimaryValue();
    cond = getRelationalOp();
    right = getPrimaryValue();
}

ConditionalOperatorType getRelationalOp() {
    Token t = tokenizer.GetToken();
    
    if (t.token_type == GREATER) return CONDITION_GREATER;
    if (t.token_type == LESS) return CONDITION_LESS;
    if (t.token_type == NOTEQUAL) return CONDITION_NOTEQUAL;
    
    cerr << "Invalid comparison" << endl;
    exit(1);
}

struct InstructionNode* processSwitch() {
    verifyToken(SWITCH);
    Token t = verifyToken(ID);
    int switchVar = findVarLocation(t.lexeme);
    verifyToken(LBRACE);

    vector<int> caseValues;
    vector<struct InstructionNode*> caseBodies;
    struct InstructionNode* defaultCase = NULL;

    while (tokenizer.peek(1).token_type == CASE) {
        verifyToken(CASE);
        Token num = verifyToken(NUM);
        caseValues.push_back(storeConstant(stoi(num.lexeme)));
        verifyToken(COLON);
        caseBodies.push_back(getBodyCode());
    }

    if (tokenizer.peek(1).token_type == DEFAULT) {
        verifyToken(DEFAULT);
        verifyToken(COLON);
        defaultCase = getBodyCode();
    }
    verifyToken(RBRACE);

    struct InstructionNode* endNode = makeNoop();

    if (caseBodies.empty() && !defaultCase) {
        return endNode;
    }

    if (defaultCase) {
        linkNodes(defaultCase, endNode);
    }

    struct InstructionNode* nextNode = defaultCase ? defaultCase : endNode;

    for (int i = caseBodies.size() - 1; i >= 0; i--) {
        struct InstructionNode* jump = new InstructionNode;
        jump->type = JMP;
        jump->jmp_inst.target = endNode;
        jump->next = NULL;
        linkNodes(caseBodies[i], jump);
        
        struct InstructionNode* condJump = new InstructionNode;
        condJump->type = CJMP;
        condJump->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
        condJump->cjmp_inst.op1_loc = switchVar;
        condJump->cjmp_inst.op2_loc = caseValues[i];
        condJump->cjmp_inst.target = caseBodies[i];
        condJump->next = nextNode;
        
        nextNode = condJump;
    }

    return nextNode;
}

struct InstructionNode* createForLoop() {
    verifyToken(FOR);
    verifyToken(LPAREN);
    
    struct InstructionNode* init = createAssignNode();
    
    ConditionalOperatorType cond;
    int left, right;
    checkCondition(cond, left, right);
    
    verifyToken(SEMICOLON);
    
    struct InstructionNode* update = createAssignNode();
    
    verifyToken(RPAREN);
    
    struct InstructionNode* condNode = new InstructionNode;
    condNode->type = CJMP;
    condNode->cjmp_inst.condition_op = cond;
    condNode->cjmp_inst.op1_loc = left;
    condNode->cjmp_inst.op2_loc = right;
    
    struct InstructionNode* body = getBodyCode();
    
    struct InstructionNode* noop = makeNoop();
    
    init->next = condNode;
    
    condNode->next = body;
    
    linkNodes(body, update);
    
    struct InstructionNode* jump = new InstructionNode;
    jump->type = JMP;
    jump->jmp_inst.target = condNode;
    jump->next = noop;
    
    update->next = jump;
    
    condNode->cjmp_inst.target = noop;
    
    return init;
}

void readInputValues() {
    while (tokenizer.peek(1).token_type == NUM) {
        Token num = tokenizer.GetToken();
        inputs.push_back(stoi(num.lexeme));
    }
}

Token verifyToken(TokenType expected) {
    Token t = tokenizer.GetToken();
    
    if (t.token_type != expected) {
        cerr << "Unexpected token type" << endl;
        exit(1);
    }
    
    return t;
}

int findVarLocation(string name) {
    if (varLocations.find(name) == varLocations.end()) {
        cerr << "Unknown variable" << endl;
        exit(1);
    }
    
    return varLocations[name];
}

int storeConstant(int val) {
    mem[next_available] = val;
    int loc = next_available;
    next_available++;
    return loc;
}

struct InstructionNode* makeNoop() {
    struct InstructionNode* node = new InstructionNode;
    node->type = NOOP;
    node->next = NULL;
    return node;
}

void linkNodes(struct InstructionNode* start, struct InstructionNode* newNode) {
    if (!start) return;
    
    struct InstructionNode* current = start;
    while (current->next != NULL) {
        current = current->next;
    }
    
    current->next = newNode;
}