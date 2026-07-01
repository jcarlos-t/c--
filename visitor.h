#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include "environment.h"
#include "semantic_types.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <ostream>
#include <exception>

using namespace std;

#include <functional>

// ============================================================
// Visitor — Abstract base for all AST visitors
// ============================================================
class Visitor {
public:
    virtual ~Visitor() = default;

    // --- Expression visitors ---
    virtual void visit(BinaryOpNode* e) = 0;
    virtual void visit(UnaryOpNode* e) = 0;
    virtual void visit(AssignmentNode* e) = 0;
    virtual void visit(FcallNode* e) = 0;
    virtual void visit(MallocNode* e) = 0;
    virtual void visit(IndexNode* e) = 0;
    virtual void visit(MemberAccessNode* e) = 0;
    virtual void visit(ArrowAccessNode* e) = 0;
    virtual void visit(SizeOfNode* e) = 0;
    virtual void visit(LambdaExprNode* e) = 0;
    virtual void visit(CaptureNode* e) = 0;
    virtual void visit(IdentifierNode* e) = 0;
    virtual void visit(IntegerLiteralNode* e) = 0;
    virtual void visit(FloatLiteralNode* e) = 0;
    virtual void visit(BoolLiteralNode* e) = 0;
    virtual void visit(CharLiteralNode* e) = 0;
    virtual void visit(StringLiteralNode* e) = 0;
    virtual void visit(PrintfNode* e) = 0;
    virtual void visit(PrimitiveTypeNode* e) = 0;
    virtual void visit(PointerTypeNode* e) = 0;
    virtual void visit(StructTypeNode* e) = 0;
    virtual void visit(NamedTypeNode* e) = 0;
    virtual void visit(TemplateTypeNode* e) = 0;

    // --- Statement visitors ---
    virtual void visit(Body* s) = 0;
    virtual void visit(ExprStmtNode* s) = 0;
    virtual void visit(IfStmt* s) = 0;
    virtual void visit(WhileStmt* s) = 0;
    virtual void visit(DoWhileStmt* s) = 0;
    virtual void visit(ForStmt* s) = 0;
    virtual void visit(SwitchStmt* s) = 0;
    virtual void visit(CaseClause* s) = 0;
    virtual void visit(BreakStmt* s) = 0;
    virtual void visit(ContinueStmt* s) = 0;
    virtual void visit(ReturnStmt* s) = 0;
    virtual void visit(FreeStmt* s) = 0;

    // --- Declaration visitors ---
    virtual void visit(VarDecl* d) = 0;
    virtual void visit(FunDecl* d) = 0;
    virtual void visit(StructDecl* d) = 0;
    virtual void visit(TemplateDecl* d) = 0;
    virtual void visit(Program* p) = 0;

    // --- L-value address computation (default no-op) ---
    virtual void computeAddress(UnaryOpNode* e) {}
    virtual void computeAddress(IdentifierNode* e) {}
    virtual void computeAddress(IndexNode* e) {}
    virtual void computeAddress(MemberAccessNode* e) {}
    virtual void computeAddress(ArrowAccessNode* e) {}
};

// ============================================================
// FuncInfo — Firma de función para type checking
// ============================================================
struct FuncInfo {
    ::Type* returnType;
    vector<::Type*> paramTypes;
};

// ============================================================
// TypeChecker — Semantic Analysis
// ============================================================
class TypeChecker : public Visitor {
private:
    Environment<::Type*> env;
    Environment<VarDecl*> varEnv;

    unordered_map<string, FuncInfo> functions;
    unordered_map<string, StructType*> struct_types;
    unordered_map<string, TemplateDecl*> template_decls;
    unordered_map<string, FunDecl*> instantiated_function_cache;

    ::Type* retornodefuncion;
    ::Type* intType;
    ::Type* boolType;
    ::Type* voidType;
    ::Type* floatType;
    ::Type* doubleType;
    ::Type* charType;

    vector<::Type*> typeCache;
    int loopDepth;
    int switchDepth;
    vector<string> errors;
    bool hasError;
    int currentOffset = 0;
    Program* program = nullptr;

    TypeNode* semantic_to_type_node(::Type* t);
    FunDecl* instantiate_function(TemplateDecl* tdecl, const vector<TypeNode*>& args);

    void add_function(FunDecl* fd);
    ::Type* type_from_ast(Exp* t);
    bool check_assign(::Type* target, ::Type* value);
    void error(const string& msg);
    void error(const string& msg, const Location& loc);
    StructType* instantiate_template(const string& name, const vector<TypeNode*>& args);
    void bind_var_decl(VarDecl* v);
    bool checkFuncCall(const string& fname, FuncInfo& info, FcallNode* e);

    void collectVars(Stm* stmt, vector<VarDecl*>& vars);
    int assignOffsets(vector<VarDecl*>& vars, int startOffset);

public:
    TypeChecker();
    ~TypeChecker();
    void typecheck(Program* program);
    bool check(Program* program);

    void visit(Program* p) override;
    void visit(FunDecl* f) override;
    void visit(VarDecl* v) override;
    void visit(StructDecl* s) override;
    void visit(TemplateDecl* d) override;
    void visit(Body* b) override;
    void visit(ExprStmtNode* s) override;
    void visit(IfStmt* s) override;
    void visit(WhileStmt* s) override;
    void visit(DoWhileStmt* s) override;
    void visit(ForStmt* s) override;
    void visit(SwitchStmt* s) override;
    void visit(CaseClause* s) override;
    void visit(BreakStmt* s) override;
    void visit(ContinueStmt* s) override;
    void visit(ReturnStmt* s) override;
    void visit(FreeStmt* s) override;

    void visit(BinaryOpNode* e) override;
    void visit(UnaryOpNode* e) override;
    void visit(AssignmentNode* e) override;
    void visit(FcallNode* e) override;
    void visit(MallocNode* e) override;
    void visit(IndexNode* e) override;
    void visit(MemberAccessNode* e) override;
    void visit(ArrowAccessNode* e) override;
    void visit(SizeOfNode* e) override;
    void visit(LambdaExprNode* e) override;
    void visit(CaptureNode* e) override;
    void visit(IdentifierNode* e) override;
    void visit(IntegerLiteralNode* e) override;
    void visit(FloatLiteralNode* e) override;
    void visit(BoolLiteralNode* e) override;
    void visit(CharLiteralNode* e) override;
    void visit(StringLiteralNode* e) override;
    void visit(PrintfNode* e) override;
    void visit(PrimitiveTypeNode* e) override;
    void visit(PointerTypeNode* e) override;
    void visit(StructTypeNode* e) override;
    void visit(NamedTypeNode* e) override;
    void visit(TemplateTypeNode* e) override;
};

// ============================================================
// ConstantFolding — Constant expression evaluation
// ============================================================
class ConstantFolding : public Visitor {
public:
    void visit(BinaryOpNode* e) override;
    void visit(UnaryOpNode* e) override;
    void visit(AssignmentNode* e) override;
    void visit(FcallNode* e) override;
    void visit(MallocNode* e) override;
    void visit(IndexNode* e) override;
    void visit(MemberAccessNode* e) override;
    void visit(ArrowAccessNode* e) override;
    void visit(SizeOfNode* e) override;
    void visit(LambdaExprNode* e) override;
    void visit(CaptureNode* e) override;
    void visit(IdentifierNode* e) override;
    void visit(IntegerLiteralNode* e) override;
    void visit(FloatLiteralNode* e) override;
    void visit(BoolLiteralNode* e) override;
    void visit(CharLiteralNode* e) override;
    void visit(StringLiteralNode* e) override;
    void visit(PrintfNode* e) override;
    void visit(PrimitiveTypeNode* e) override;
    void visit(PointerTypeNode* e) override;
    void visit(StructTypeNode* e) override;
    void visit(NamedTypeNode* e) override;
    void visit(TemplateTypeNode* e) override;

    void visit(Body* s) override;
    void visit(ExprStmtNode* s) override;
    void visit(IfStmt* s) override;
    void visit(WhileStmt* s) override;
    void visit(DoWhileStmt* s) override;
    void visit(ForStmt* s) override;
    void visit(SwitchStmt* s) override;
    void visit(CaseClause* s) override;
    void visit(BreakStmt* s) override;
    void visit(ContinueStmt* s) override;
    void visit(ReturnStmt* s) override;
    void visit(FreeStmt* s) override;

    void visit(VarDecl* d) override;
    void visit(FunDecl* d) override;
    void visit(StructDecl* d) override;
    void visit(TemplateDecl* d) override;
    void visit(Program* p) override;
};

// ============================================================
// GenCodeVisitor — x86-64 AT&T Code Generator
// ============================================================
class GenCodeVisitor : public Visitor {
public:
    enum class LValKind { Invalid, Id, Index, Member, Deref };
    struct LVal {
        LValKind kind = LValKind::Invalid;
        string name;
        VarDecl* binding = nullptr;
        Exp* index = nullptr;
        vector<Exp*> indices;
        string member;
        string structName;
        bool isArrow = false;
    };

private:
    ostream &out;
    LVal *lvalTarget = nullptr;
    LVal captureLVal(Exp *e);
    void storeTarget(const LVal &lv);

    void bind_var_decl(VarDecl* v);
    int array_elem_size(VarDecl* vd) const;
    void loadBinding(VarDecl* vd);
    void storeBinding(VarDecl* vd);
    void leaBinding(VarDecl* vd);
    string bindingMem(VarDecl* vd) const;

    void emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices);
    void emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices);
    void emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices, const string& valueReg);

    string instrSuffix(int size);
    string loadInstr(int size);
    string storeInstr(int size);

    unordered_map<string, int> memoria;
    unordered_map<string, bool> memoriaGlobal;
    unordered_map<string, int> structFieldCount;
    unordered_map<string, unordered_map<string, int>> structFieldOffset;
    unordered_map<string, unordered_map<string, int>> structMemberSizes;
    unordered_map<string, int> stringLabels;

    int offset = -8;
    int labelcont = 0;
    bool inFunction = false;
    string currentBreakLabel;
    string currentContinueLabel;
    string funcName;
    string returnLabel;

public:
    explicit GenCodeVisitor(ostream &out) : out(out) {}

    void generate(Program *p);

    void visit(BinaryOpNode *e) override;
    void visit(UnaryOpNode *e) override;
    void visit(AssignmentNode *e) override;
    void visit(FcallNode *e) override;
    void visit(IndexNode *e) override;
    void visit(MemberAccessNode *e) override;
    void visit(ArrowAccessNode *e) override;
    void visit(MallocNode *e) override;
    void visit(SizeOfNode *e) override;
    void visit(IdentifierNode *e) override;
    void visit(IntegerLiteralNode *e) override;
    void visit(FloatLiteralNode *e) override;
    void visit(BoolLiteralNode *e) override;
    void visit(CharLiteralNode *e) override;
    void visit(StringLiteralNode *e) override;
    void visit(PrintfNode *e) override;
    void visit(PrimitiveTypeNode *e) override;
    void visit(PointerTypeNode *e) override;
    void visit(StructTypeNode *e) override;
    void visit(NamedTypeNode *e) override;
    void visit(TemplateTypeNode *e) override;
    void visit(CaptureNode *e) override;
    void visit(LambdaExprNode *e) override;

    void visit(Body *s) override;
    void visit(ExprStmtNode *s) override;
    void visit(IfStmt *s) override;
    void visit(WhileStmt *s) override;
    void visit(DoWhileStmt *s) override;
    void visit(ForStmt *s) override;
    void visit(SwitchStmt *s) override;
    void visit(CaseClause *s) override;
    void visit(BreakStmt *s) override;
    void visit(ContinueStmt *s) override;
    void visit(ReturnStmt *s) override;
    void visit(FreeStmt *s) override;

    void visit(VarDecl *d) override;
    void visit(FunDecl *d) override;
    void visit(StructDecl *d) override;
    void visit(Program *p) override;
    void visit(TemplateDecl *d) override;

    void computeAddress(UnaryOpNode *e) override;
    void computeAddress(IdentifierNode *e) override;
    void computeAddress(IndexNode *e) override;
    void computeAddress(MemberAccessNode *e) override;
    void computeAddress(ArrowAccessNode *e) override;
};

#endif
