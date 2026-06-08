#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include <unordered_map>
#include <string>
#include <vector>
#include "ast.h"
#include "environment.h"
#include "semantic_types.h"

using namespace std;

// Visitor abstracto para recorrido semántico del AST
class TypeVisitor {
public:
    virtual void visit(Program* p) = 0;
    virtual void visit(FunDecl* f) = 0;
    virtual void visit(VarDecl* v) = 0;
    virtual void visit(StructDecl* s) = 0;
    virtual void visit(CompoundStmt* b) = 0;

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

    virtual Type* visit(BinaryOpNode* e) = 0;
    virtual Type* visit(UnaryOpNode* e) = 0;
    virtual Type* visit(AssignmentNode* e) = 0;
    virtual Type* visit(TernaryOpNode* e) = 0;
    virtual Type* visit(CallNode* e) = 0;
    virtual Type* visit(SubscriptNode* e) = 0;
    virtual Type* visit(MemberAccessNode* e) = 0;
    virtual Type* visit(ArrowAccessNode* e) = 0;
    virtual Type* visit(CastNode* e) = 0;
    virtual Type* visit(IdentifierNode* e) = 0;
    virtual Type* visit(IntegerLiteralNode* e) = 0;
    virtual Type* visit(FloatLiteralNode* e) = 0;
    virtual Type* visit(BoolLiteralNode* e) = 0;
    virtual Type* visit(CharLiteralNode* e) = 0;
    virtual Type* visit(StringLiteralNode* e) = 0;
    virtual Type* visit(ParenthesizedExprNode* e) = 0;
    virtual Type* visit(PrimitiveTypeNode* e) = 0;
    virtual Type* visit(PointerTypeNode* e) = 0;
    virtual Type* visit(StructTypeNode* e) = 0;
    virtual Type* visit(NamedTypeNode* e) = 0;
};

// Información de firma de función para verificación de llamadas
struct FuncInfo {
    Type* returnType;
    vector<Type*> paramTypes;
};

class TypeChecker : public TypeVisitor {
private:
    Environment<Type*> env;           // Tabla de símbolos con scoping
    unordered_map<string, FuncInfo> functions;  // Funciones declaradas
    unordered_map<string, StructType*> struct_types; // Structs declarados
    Type* retornodefuncion;           // Tipo de retorno de la función actual

    // Tipos predefinidos compartidos
    Type* intType;
    Type* boolType;
    Type* voidType;
    Type* floatType;
    Type* charType;

    // Cache de tipos dinámicos creados (PointerType, ArrayType, etc.)
    vector<Type*> typeCache;

    // Contexto para break/continue
    int loopDepth;   // Nivel de anidamiento de ciclos (while/for/do-while)
    int switchDepth; // Nivel de anidamiento de switch

    // Colección de errores semánticos (en lugar de exit inmediato)
    vector<string> errors;
    bool hasError;

    void add_function(FunDecl* fd);
    Type* type_from_ast(Exp* t);
    bool check_assign(Type* target, Type* value);
    void error(const string& msg);

public:
    TypeChecker();
    ~TypeChecker();
    void typecheck(Program* program);

    // Visitas a declaraciones y statements
    void visit(Program* p) override;
    void visit(FunDecl* f) override;
    void visit(VarDecl* v) override;
    void visit(StructDecl* s) override;
    void visit(CompoundStmt* b) override;

    void visit(ExprStmtNode* s) override;
    void visit(DeclStmt* s) override;
    void visit(IfStmt* s) override;
    void visit(WhileStmt* s) override;
    void visit(DoWhileStmt* s) override;
    void visit(ForStmt* s) override;
    void visit(SwitchStmt* s) override;
    void visit(CaseClause* s) override;
    void visit(DefaultClause* s) override;
    void visit(BreakStmt* s) override;
    void visit(ContinueStmt* s) override;
    void visit(ReturnStmt* s) override;

    // Visitas a expresiones
    Type* visit(BinaryOpNode* e) override;
    Type* visit(UnaryOpNode* e) override;
    Type* visit(AssignmentNode* e) override;
    Type* visit(TernaryOpNode* e) override;
    Type* visit(CallNode* e) override;
    Type* visit(SubscriptNode* e) override;
    Type* visit(MemberAccessNode* e) override;
    Type* visit(ArrowAccessNode* e) override;
    Type* visit(CastNode* e) override;
    Type* visit(IdentifierNode* e) override;
    Type* visit(IntegerLiteralNode* e) override;
    Type* visit(FloatLiteralNode* e) override;
    Type* visit(BoolLiteralNode* e) override;
    Type* visit(CharLiteralNode* e) override;
    Type* visit(StringLiteralNode* e) override;
    Type* visit(ParenthesizedExprNode* e) override;
    Type* visit(PrimitiveTypeNode* e) override;
    Type* visit(PointerTypeNode* e) override;
    Type* visit(StructTypeNode* e) override;
    Type* visit(NamedTypeNode* e) override;
};

#endif
