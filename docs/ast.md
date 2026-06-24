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
    Body,
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
    ASTNode* body;                  // Body
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

struct Body : ASTNode {
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

struct Fcall : ASTNode {
    ASTNode* callee;                // function expression
    vector<ASTNode*> args;
};

struct Index : ASTNode {
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
    ASTNode* body;                  // Body
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
│   └── Body
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
└── Body ── stmt[]

Expressions
├── BinaryOp ── left OP right
├── UnaryOp ── OP operand
├── Assignment ── target OP value
├── TernaryOp ── cond ? then : else
├── Fcall ── callee(args[])
├── Index ── base[index]
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

## 6. Visitor Patterns

El AST soporta actualmente **tres** jerarquías de visitors mediante double dispatch:

| Jerarquía | Clase Base | Retorno Exp | Retorno Stm | Propósito |
|-----------|-----------|-------------|-------------|-----------|
| `Visitor` | `Visitor` | `double` | `int` | Interpretación (`EVALVisitor`) y pretty-print (`PrintVisitor`) |
| `TypeVisitor` | `TypeVisitor` | `Type*` | `void` | Type checking (`TypeChecker`) |
| `CodeGenVisitor` | `CodeGenVisitor` | `void` | `void` | Code generation (x86-64) |

Cada nodo tiene tres métodos `accept`:

```cpp
// Ejemplo: BinaryOpNode
double accept(Visitor* visitor);      → visitor.cpp
Type* accept(TypeVisitor* visitor);   → TypeChecker.cpp
void accept(CodeGenVisitor* visitor); → ast.cpp (stub, double dispatch)
```

### 6.1 CodeGenVisitor (abstracto)

`CodeGenVisitor` está definido en `CodeGenVisitor.h` e incluye:

```cpp
class CodeGenVisitor {
public:
    // --- Expressions ---
    virtual void visit(BinaryOpNode* e) = 0;
    virtual void visit(UnaryOpNode* e) = 0;
    virtual void visit(AssignmentNode* e) = 0;
    virtual void visit(TernaryOpNode* e) = 0;
    virtual void visit(FcallNode* e) = 0;
    virtual void visit(IndexNode* e) = 0;
    virtual void visit(MemberAccessNode* e) = 0;
    virtual void visit(ArrowAccessNode* e) = 0;
    virtual void visit(MallocNode* e) = 0;
    virtual void visit(CastNode* e) = 0;
    virtual void visit(SizeOfNode* e) = 0;
    virtual void visit(IdentifierNode* e) = 0;
    virtual void visit(IntegerLiteralNode* e) = 0;
    virtual void visit(FloatLiteralNode* e) = 0;
    virtual void visit(BoolLiteralNode* e) = 0;
    virtual void visit(CharLiteralNode* e) = 0;
    virtual void visit(StringLiteralNode* e) = 0;
    virtual void visit(ParenthesizedExprNode* e) = 0;
    virtual void visit(PrimitiveTypeNode* e) = 0;
    virtual void visit(PointerTypeNode* e) = 0;
    virtual void visit(StructTypeNode* e) = 0;
    virtual void visit(NamedTypeNode* e) = 0;
    virtual void visit(CaptureNode* e) = 0;
    virtual void visit(LambdaExprNode* e) = 0;

    // --- Statements ---
    virtual void visit(Body* s) = 0;
    virtual void visit(ExprStmtNode* s) = 0;
    virtual void visit(DeclStmt* s) = 0;
    virtual void visit(IfStmt* s) = 0;
    virtual void visit(WhileStmt* s) = 0;
    virtual void visit(DoWhileStmt* s) = 0;
    virtual void visit(ForStmt* s) = 0;
    virtual void visit(SwitchStmt* s) = 0;
    virtual void visit(CaseClause* s) = 0;
    virtual void visit(DefaultClause* s) = 0;
    virtual void visit(BreakStmt* s) = 0;
    virtual void visit(ContinueStmt* s) = 0;
    virtual void visit(ReturnStmt* s) = 0;
    virtual void visit(FreeStmt* s) = 0;

    // --- Declarations ---
    virtual void visit(VarDecl* d) = 0;
    virtual void visit(FunDecl* d) = 0;
    virtual void visit(StructDecl* d) = 0;
    virtual void visit(Program* p) = 0;
    virtual void visit(TemplateDecl* d) = 0;

    // --- Lvalue address computation ---
    virtual void computeAddress(UnaryOpNode* e) = 0;
    virtual void computeAddress(IdentifierNode* e) = 0;
    virtual void computeAddress(IndexNode* e) = 0;
    virtual void computeAddress(MemberAccessNode* e) = 0;
    virtual void computeAddress(ArrowAccessNode* e) = 0;
};
```

### 6.2 Lvalue handling (`computeAddress`)

Para generación de código, la asignación requiere distinguir entre:

- **Rvalue** (`visit`) — evaluar una expresión, dejar el valor en `%rax`
- **Lvalue** (`computeAddress`) — calcular la dirección efectiva de un lvalue y dejarla en `%rbx` para almacenar

El método `computeAddress` está declarado en `Exp` con una implementación default que lanza error ("not an lvalue"). Los siguientes nodos lo overriddean:

| Nodo | Descripción |
|------|-------------|
| `UnaryOpNode` (solo DEREF) | Dirección del apuntado (`*ptr`) |
| `IdentifierNode` | Dirección de variable (`x`) |
| `IndexNode` | Dirección de elemento de array (`arr[i]`) |
| `MemberAccessNode` | Dirección de miembro de struct (`s.m`) |
| `ArrowAccessNode` | Dirección de miembro vía puntero (`p->m`) |

El patrón de uso en asignación simple (`=`):

```
visit(RHS)           → valor en %rax
target->computeAddress(this) → dirección en %rbx
emit "movq %rax, (%rbx)"     → almacenar
```

Para asignación compuesta (`+=`, etc.), primero se calcula la dirección, luego se carga el valor actual, se aplica la operación, y se almacena el resultado.
