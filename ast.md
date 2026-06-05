# AST Structure

## 1. Overview

The Abstract Syntax Tree (AST) is the intermediate representation produced by the parser. Each node type corresponds to a language construct. The AST is designed as a tree of `ASTNode*` pointers, where each node stores its source location and children.

## 2. Base Structure

```cpp
struct Location {
    int line;
    int column;
};

enum class NodeKind {
    // --- Top Level ---
    Program,
    FunctionDecl,
    VariableDecl,
    StructDecl,
    TemplateDecl,

    // --- Types ---
    PrimitiveType,
    PointerType,
    StructType,
    NamedType,

    // --- Statements ---
    CompoundStmt,
    ExprStmt,
    IfStmt,
    WhileStmt,
    DoWhileStmt,
    ForStmt,
    SwitchStmt,
    CaseClause,
    DefaultClause,
    BreakStmt,
    ContinueStmt,
    ReturnStmt,

    // --- Expressions ---
    BinaryOp,
    UnaryOp,
    Assignment,
    TernaryOp,
    Call,
    Subscript,
    MemberAccess,
    ArrowAccess,
    PostInc,
    PostDec,
    PreInc,
    PreDec,
    Cast,

    // --- Primary ---
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    CharLiteral,
    StringLiteral,
    ParenthesizedExpr,

    // --- Advanced ---
    LambdaExpr,
    MallocExpr,
    FreeStmt,

    // --- Other ---
    Parameter,
    Capture,
    TemplateParam,
};
```

## 3. Node Hierarchy

```cpp
struct ASTNode {
    NodeKind kind;
    Location loc;
};

// ──────────────────────────────────────────
// Top-Level Declarations
// ──────────────────────────────────────────

struct Program : ASTNode {
    vector<ASTNode*> decls;    // functions, variables, structs, etc.
};

struct FunctionDecl : ASTNode {
    ASTNode* return_type;           // type node
    string name;
    vector<ASTNode*> params;        // Parameter nodes
    ASTNode* body;                  // CompoundStmt
    bool is_template = false;
};

struct VariableDecl : ASTNode {
    ASTNode* type;                  // type node
    string name;
    vector<ASTNode*> array_sizes;   // size exprs for multi-dim arrays
    ASTNode* initializer;           // optional
};

struct StructDecl : ASTNode {
    string name;
    vector<ASTNode*> members;       // VariableDecl nodes
};

// ──────────────────────────────────────────
// Types
// ──────────────────────────────────────────

struct PrimitiveType : ASTNode {
    enum class Prim { VOID, INT, CHAR, FLOAT, DOUBLE, AUTO };
    Prim prim;
};

struct PointerType : ASTNode {
    ASTNode* base;                  // type being pointed to
};

struct StructType : ASTNode {
    string name;                    // name of the struct
};

struct NamedType : ASTNode {
    string name;
};

// ──────────────────────────────────────────
// Parameters
// ──────────────────────────────────────────

struct Parameter : ASTNode {
    ASTNode* type;
    string name;
    vector<ASTNode*> array_sizes;
};

// ──────────────────────────────────────────
// Statements
// ──────────────────────────────────────────

struct CompoundStmt : ASTNode {
    vector<ASTNode*> stmts;
};

struct ExprStmt : ASTNode {
    ASTNode* expr;                  // can be null (empty statement)
};

struct IfStmt : ASTNode {
    ASTNode* condition;
    ASTNode* then_branch;
    ASTNode* else_branch;           // optional
};

struct WhileStmt : ASTNode {
    ASTNode* condition;
    ASTNode* body;
};

struct DoWhileStmt : ASTNode {
    ASTNode* body;
    ASTNode* condition;
};

struct ForStmt : ASTNode {
    ASTNode* init;                  // ExprStmt or VariableDecl
    ASTNode* condition;             // optional
    ASTNode* increment;             // optional
    ASTNode* body;
};

struct SwitchStmt : ASTNode {
    ASTNode* expr;
    vector<ASTNode*> cases;         // CaseClause / DefaultClause nodes
};

struct CaseClause : ASTNode {
    ASTNode* value;                 // constant expression
    vector<ASTNode*> stmts;
};

struct DefaultClause : ASTNode {
    vector<ASTNode*> stmts;
};

struct BreakStmt : ASTNode {};
struct ContinueStmt : ASTNode {};

struct ReturnStmt : ASTNode {
    ASTNode* expr;                  // optional
};

// ──────────────────────────────────────────
// Expressions
// ──────────────────────────────────────────

struct BinaryOp : ASTNode {
    enum class Op {
        ADD, SUB, MUL, DIV, MOD,
        EQ, NE, LT, GT, LE, GE,
        LOG_AND, LOG_OR,
        COMMA
    };
    Op op;
    ASTNode* left;
    ASTNode* right;
};

struct UnaryOp : ASTNode {
    enum class Op { ADDR, DEREF, MINUS, LOG_NOT };
    Op op;
    ASTNode* operand;
};

struct Assignment : ASTNode {
    enum class Op {
        ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN
    };
    Op op;
    ASTNode* target;
    ASTNode* value;
};

struct TernaryOp : ASTNode {
    ASTNode* condition;
    ASTNode* then_expr;
    ASTNode* else_expr;
};

struct Call : ASTNode {
    ASTNode* callee;                // function expression
    vector<ASTNode*> args;
};

struct Subscript : ASTNode {
    ASTNode* base;                  // array expression
    ASTNode* index;
};

struct MemberAccess : ASTNode {
    ASTNode* object;
    string member;
};

struct ArrowAccess : ASTNode {
    ASTNode* pointer;
    string member;
};

struct PostInc : ASTNode {
    ASTNode* operand;
};

struct PostDec : ASTNode {
    ASTNode* operand;
};

struct PreInc : ASTNode {
    ASTNode* operand;
};

struct PreDec : ASTNode {
    ASTNode* operand;
};

struct Cast : ASTNode {
    ASTNode* target_type;
    ASTNode* expr;
};

// ──────────────────────────────────────────
// Primary Expressions
// ──────────────────────────────────────────

struct Identifier : ASTNode {
    string name;
};

struct IntegerLiteral : ASTNode {
    long long value;
};

struct FloatLiteral : ASTNode {
    double value;
};

struct CharLiteral : ASTNode {
    char value;
};

struct StringLiteral : ASTNode {
    string value;
};

struct ParenthesizedExpr : ASTNode {
    ASTNode* expr;
};

// ──────────────────────────────────────────
// Advanced Features
// ──────────────────────────────────────────

struct LambdaExpr : ASTNode {
    vector<ASTNode*> captures;      // Capture nodes
    vector<ASTNode*> params;        // Parameter nodes
    ASTNode* return_type;
    ASTNode* body;                  // CompoundStmt
};

struct Capture : ASTNode {
    enum class Mode { BY_VALUE, BY_REF };
    Mode mode;
    string name;                    // optional (for default capture)
};

struct MallocExpr : ASTNode {
    ASTNode* size;
};

struct FreeStmt : ASTNode {
    ASTNode* ptr;
};

// ──────────────────────────────────────────
// Templates
// ──────────────────────────────────────────

struct TemplateDecl : ASTNode {
    vector<ASTNode*> params;        // TemplateParam nodes
    ASTNode* declaration;           // FunctionDecl or StructDecl
};

struct TemplateParam : ASTNode {
    string name;
};
```

## 4. Tree Relationships

```
Program
├── FunctionDecl
│   ├── PrimitiveType / PointerType / StructType
│   ├── Parameter (×N)
│   │   ├── type
│   │   └── name
│   └── CompoundStmt
│       └── Statement (×N)
│
├── VariableDecl
│   ├── type
│   ├── array_sizes[×N]
│   └── initializer (optional)
│
├── StructDecl
│   ├── name
│   └── VariableDecl (×N) — members
│
├── TemplateDecl
│   ├── TemplateParam (×N)
│   └── FunctionDecl / StructDecl

Statements
├── IfStmt ── condition, then_branch, [else_branch]
├── WhileStmt ── condition, body
├── DoWhileStmt ── body, condition
├── ForStmt ── init, condition, increment, body
├── SwitchStmt ── expr, CaseClause/DefaultClause[]
├── ReturnStmt ── [expr]
├── BreakStmt / ContinueStmt
├── ExprStmt ── expr
└── CompoundStmt ── stmt[]

Expressions
├── BinaryOp ── left OP right
├── UnaryOp ── OP operand
├── Assignment ── target OP value
├── TernaryOp ── cond ? then : else
├── Call ── callee(args[])
├── Subscript ── base[index]
├── MemberAccess ── object.member
├── ArrowAccess ── pointer->member
├── Cast ── (type)expr
├── PostInc/PostDec/PreInc/PreDec
├── LambdaExpr ── captures[], params[], return_type, body
├── MallocExpr ── size
└── Primary ── Identifier / Literal / String / (expr)
```

## 5. Memory Ownership

The AST uses **exclusive ownership** via raw pointers. The `Program` root node is responsible for deallocating the entire tree. A recursive `freeNode(ASTNode*)` function traverses and deletes all children based on `NodeKind`.

## 6. Visitor Pattern (Suggestion)

```cpp
class ASTVisitor {
public:
    virtual void visit(Program*) = 0;
    virtual void visit(FunctionDecl*) = 0;
    virtual void visit(VariableDecl*) = 0;
    virtual void visit(StructDecl*) = 0;
    virtual void visit(TemplateDecl*) = 0;
    virtual void visit(TemplateParam*) = 0;
    virtual void visit(PrimitiveType*) = 0;
    virtual void visit(PointerType*) = 0;
    virtual void visit(StructType*) = 0;
    virtual void visit(NamedType*) = 0;
    virtual void visit(Parameter*) = 0;
    virtual void visit(CompoundStmt*) = 0;
    virtual void visit(ExprStmt*) = 0;
    virtual void visit(IfStmt*) = 0;
    virtual void visit(WhileStmt*) = 0;
    virtual void visit(DoWhileStmt*) = 0;
    virtual void visit(ForStmt*) = 0;
    virtual void visit(SwitchStmt*) = 0;
    virtual void visit(CaseClause*) = 0;
    virtual void visit(DefaultClause*) = 0;
    virtual void visit(BreakStmt*) = 0;
    virtual void visit(ContinueStmt*) = 0;
    virtual void visit(ReturnStmt*) = 0;
    virtual void visit(BinaryOp*) = 0;
    virtual void visit(UnaryOp*) = 0;
    virtual void visit(Assignment*) = 0;
    virtual void visit(TernaryOp*) = 0;
    virtual void visit(Call*) = 0;
    virtual void visit(Subscript*) = 0;
    virtual void visit(MemberAccess*) = 0;
    virtual void visit(ArrowAccess*) = 0;
    virtual void visit(PostInc*) = 0;
    virtual void visit(PostDec*) = 0;
    virtual void visit(PreInc*) = 0;
    virtual void visit(PreDec*) = 0;
    virtual void visit(Cast*) = 0;
    virtual void visit(Identifier*) = 0;
    virtual void visit(IntegerLiteral*) = 0;
    virtual void visit(FloatLiteral*) = 0;
    virtual void visit(CharLiteral*) = 0;
    virtual void visit(StringLiteral*) = 0;
    virtual void visit(ParenthesizedExpr*) = 0;
    virtual void visit(LambdaExpr*) = 0;
    virtual void visit(Capture*) = 0;
    virtual void visit(MallocExpr*) = 0;
    virtual void visit(FreeStmt*) = 0;
};
```

This visitor pattern decouples traversal from analysis (type checking, code generation, printing).
