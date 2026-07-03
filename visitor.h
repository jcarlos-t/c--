#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include "environment.h"
#include "semantic_types.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <ostream>
#include <exception>

using namespace std;


// Visitor — base abstracta para recorrer el AST

class Visitor {
public:
    virtual ~Visitor() = default;

    // --- Expresiones ---
    virtual void visit(BinaryOpNode* e) = 0;
    virtual void visit(UnaryOpNode* e) = 0;
    virtual void visit(AssignmentNode* e) = 0;
    virtual void visit(FcallNode* e) = 0;
    virtual void visit(MallocNode* e) = 0;
    virtual void visit(IndexNode* e) = 0;
    virtual void visit(MemberAccessNode* e) = 0;
    virtual void visit(ArrowAccessNode* e) = 0;
    virtual void visit(SizeOfNode* e) = 0;
    virtual void visit(IdNode* e) = 0;
    virtual void visit(NumberLiteralNode* e) = 0;
    virtual void visit(FloatLiteralNode* e) = 0;
    virtual void visit(BoolLiteralNode* e) = 0;
    virtual void visit(CharLiteralNode* e) = 0;
    virtual void visit(StringLiteralNode* e) = 0;
    virtual void visit(PrintfNode* e) = 0;
    // Nodos de tipo
    virtual void visit(PrimitiveTypeNode* e) = 0;
    virtual void visit(PointerTypeNode* e) = 0;
    virtual void visit(StructTypeNode* e) = 0;

    // --- Statements ---
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

    // --- Declaraciones ---
    virtual void visit(VarDecl* d) = 0;
    virtual void visit(FunDecl* d) = 0;
    virtual void visit(StructDecl* d) = 0;
    virtual void visit(Program* p) = 0;

    // Para & y asignaciones: calcula dirección en vez de cargar valor
    virtual void computeAddress(UnaryOpNode* ) {}
    virtual void computeAddress(IdNode* ) {}
    virtual void computeAddress(IndexNode* ) {}
    virtual void computeAddress(MemberAccessNode* ) {}
    virtual void computeAddress(ArrowAccessNode* ) {}
};

// FuncInfo — firma de función para validar llamadas
struct FuncInfo {
    ::Type* returnType;
    vector<::Type*> paramTypes;
};

// TypeChecker — análisis semántico, construye tablas de símbolos
class TypeChecker : public Visitor {
private:
    Environment<::Type*> env;              // variable → tipo
    Environment<VarDecl*> varEnv;          // variable → declaración
    unordered_map<string, FuncInfo> functions;     // firmas de funciones
    unordered_map<string, StructType*> struct_types; // structs resueltos
    ::Type* retornodefuncion;              // tipo de retorno actual

    ::Type* intType;    // singletons de tipos primitivos
    ::Type* boolType;
    ::Type* voidType;
    ::Type* floatType;
    ::Type* doubleType;
    ::Type* charType;
    ::Type* longType;

    vector<::Type*> typeCache;             // tipos dinámicos (punteros, arreglos)

    int loopDepth;                         // profundidad de bucles
    int switchDepth;                       // profundidad de switch
    unordered_set<VarDecl*> initialized_vars; // variables inicializadas
    int functionDepth = 0;                 // profundidad de funciones
    bool isLvalContext = false;            // omitir chequeo de inicialización en l-value

    vector<string> errors;                 // acumulación de errores
    bool hasError;

    void add_function(FunDecl* fd);        // registra firma de función
    ::Type* type_from_ast(TypeNode* t);    // TypeNode del AST → Type*
    bool check_assign(::Type* target, ::Type* value); // compatibilidad de tipos
    void error(const string& msg);
    void error(const string& msg, const Location& loc);
    void bind_var_decl(VarDecl* v);        // registra variable en envs
    bool checkFuncCall(const string& fname, FuncInfo& info, FcallNode* e);
    void collectVars(Stm* stmt, vector<VarDecl*>& vars); // junta todas las VarDecl locales
    int assignOffsets(vector<VarDecl*>& vars, int startOffset); // bin packing en stack

public:
    TypeChecker();
    ~TypeChecker();
    void typecheck(Program* program);      // entry point, exit(1) si hay errores
    void visit(Program* p) override;
    void visit(FunDecl* f) override;
    void visit(VarDecl* v) override;
    void visit(StructDecl* s) override;
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
    void visit(IdNode* e) override;
    void visit(NumberLiteralNode* e) override;
    void visit(FloatLiteralNode* e) override;
    void visit(BoolLiteralNode* e) override;
    void visit(CharLiteralNode* e) override;
    void visit(StringLiteralNode* e) override;
    void visit(PrintfNode* e) override;
    void visit(PrimitiveTypeNode* e) override;
    void visit(PointerTypeNode* e) override;
    void visit(StructTypeNode* e) override;
};

// ConstantFolding — evalúa expresiones constantes en compile-time
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
    void visit(IdNode* e) override;
    void visit(NumberLiteralNode* e) override;
    void visit(FloatLiteralNode* e) override;
    void visit(BoolLiteralNode* e) override;
    void visit(CharLiteralNode* e) override;
    void visit(StringLiteralNode* e) override;
    void visit(PrintfNode* e) override;
    void visit(PrimitiveTypeNode* e) override;
    void visit(PointerTypeNode* e) override;
    void visit(StructTypeNode* e) override;

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
    void visit(Program* p) override;
};

// GenCodeVisitor — genera ensamblador x86-64 (AT&T)
class GenCodeVisitor : public Visitor {
public:
    enum class LValKind { Invalid, Id, Index, Member, Deref };

    struct LVal {
        LValKind kind = LValKind::Invalid;
        string name;                    // nombre de variable
        VarDecl* binding = nullptr;     // enlace al VarDecl
        vector<Exp*> indices;           // índices multidimensionales
        vector<string> members;         // lista de miembros anidados
        vector<bool> memberArrow;       // true si el miembro se accedió vía ->
        string structName;              // tipo del struct contenedor
        bool isArrow = false;           // ptr->miembro vs obj.miembro
        int storeSize = 8;              // bytes a almacenar (para Deref)
    };

private:
    ostream &out;                       // salida del .s

    LVal *lvalTarget = nullptr;         // captura l-values para asignaciones
    LVal captureLVal(Exp *e);           // evalúa expresión como l-value
    void storeTarget(const LVal &lv);   // guarda %rax en el l-value
    bool directStoreForConstant(Exp* value, VarDecl* vd); // store directo si const
    void bind_var_decl(VarDecl* v);     // registra offset de variable

    int array_elem_size(VarDecl* vd) const;

    void loadBinding(VarDecl* vd);      // carga valor de variable a %rax
    void storeBinding(VarDecl* vd);     // guarda %rax en variable
    void leaBinding(VarDecl* vd);       // dirección de variable a %rax
    string bindingMem(VarDecl* vd) const; // ej. "-12(%rbp)"

    void emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices);
    void emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices);
    void emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices, const string& valueReg);

    string instrSuffix(int size);       // 1→b, 4→l, 8→q
    string loadInstr(int size, bool isUnsigned = false); // movsbq/movzbq, movslq, movq
    string storeInstr(int size);        // movb, movl, movq

    unordered_map<string, int> memoria;            // variable → offset en stack
    unordered_map<string, bool> memoriaGlobal;      // variables globales

    unordered_map<string, int> structFieldCount;   // metadatos de structs
    unordered_map<string, unordered_map<string, int>> structFieldOffset;
    unordered_map<string, unordered_map<string, int>> structMemberSizes;
    unordered_map<string, unordered_map<string, Type*>> structMemberTypes;

    unordered_map<string, int> stringLabels;       // string literal → .LstrN
    int offset = -8;                               // próximo offset en stack
    int labelcont = 0;                             // contador de etiquetas

    bool inFunction = false;
    string currentBreakLabel;
    string currentContinueLabel;
    string funcName;                    // nombre de la función actual
    string returnLabel;                 // etiqueta de retorno
    bool usedPow = false;               // true si se usa **

public:
    explicit GenCodeVisitor(ostream &out) : out(out) {}

    void generate(Program *p);          // entry point

    void visit(BinaryOpNode *e) override;
    void visit(UnaryOpNode *e) override;
    void visit(AssignmentNode *e) override;
    void visit(FcallNode *e) override;
    void visit(IndexNode *e) override;
    void visit(MemberAccessNode *e) override;
    void visit(ArrowAccessNode *e) override;
    void visit(MallocNode *e) override;
    void visit(SizeOfNode *e) override;
    void visit(IdNode *e) override;
    void visit(NumberLiteralNode *e) override;
    void visit(FloatLiteralNode *e) override;
    void visit(BoolLiteralNode *e) override;
    void visit(CharLiteralNode *e) override;
    void visit(StringLiteralNode *e) override;
    void visit(PrintfNode *e) override;
    void visit(PrimitiveTypeNode *e) override;
    void visit(PointerTypeNode *e) override;
    void visit(StructTypeNode *e) override;

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

    // calculan dirección del l-value en %rax
    void computeAddress(UnaryOpNode *e) override;
    void computeAddress(IdNode *e) override;
    void computeAddress(IndexNode *e) override;
    void computeAddress(MemberAccessNode *e) override;
    void computeAddress(ArrowAccessNode *e) override;
};

#endif
