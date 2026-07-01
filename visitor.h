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
// Abstract Visitor — Visitor Pattern base for EVALVisitor and
// ConstantFolding. Each expression returns double, each
// statement/declaration returns int.
// ============================================================
class Visitor {
public:
    virtual ~Visitor() = default;

    // --- Expression visitors (retornan double) ---
    // Cada expresión produce un valor numérico al evaluarse
    virtual double visit(BinaryOpNode* e) = 0;       // a + b, a * b, a && b, etc.
    virtual double visit(UnaryOpNode* e) = 0;         // -x, *p, &x, !b, ++x, x--
    virtual double visit(AssignmentNode* e) = 0;      // x = expr
    virtual double visit(FcallNode* e) = 0;           // f(args), obj.method(args)
    virtual double visit(MallocNode* e) = 0;          // malloc(expr)
    virtual double visit(IndexNode* e) = 0;           // arr[i], ptr[i]
    virtual double visit(MemberAccessNode* e) = 0;    // obj.member
    virtual double visit(ArrowAccessNode* e) = 0;     // ptr->member
    virtual double visit(SizeOfNode* e) = 0;          // sizeof(type)
    virtual double visit(LambdaExprNode* e) = 0;      // [x](int a) -> int { ... }
    virtual double visit(CaptureNode* e) = 0;         // variable capturada en lambda
    virtual double visit(IdentifierNode* e) = 0;      // nombre de variable
    virtual double visit(IntegerLiteralNode* e) = 0;  // 42, 0, -1
    virtual double visit(FloatLiteralNode* e) = 0;    // 3.14, 2.0
    virtual double visit(BoolLiteralNode* e) = 0;     // true, false
    virtual double visit(CharLiteralNode* e) = 0;     // 'a', '\n'
    virtual double visit(StringLiteralNode* e) = 0;   // "hola"
    virtual double visit(PrintfNode* e) = 0;          // printf("fmt", args)
    virtual double visit(PrimitiveTypeNode* e) = 0;   // int, char, float, etc.
    virtual double visit(PointerTypeNode* e) = 0;     // int*, char*, etc.
    virtual double visit(StructTypeNode* e) = 0;      // struct Persona
    virtual double visit(NamedTypeNode* e) = 0;       // typename T (template)
    virtual double visit(TemplateTypeNode* e) = 0;    // Vector<int>, Par<float,int>

    // --- Statement visitors (retornan int, código de control) ---
    virtual int visit(Body* s) = 0;           // { stmt; stmt; ... }
    virtual int visit(ExprStmtNode* s) = 0;   // expr;
    virtual int visit(IfStmt* s) = 0;         // if (cond) then [else]
    virtual int visit(WhileStmt* s) = 0;      // while (cond) body
    virtual int visit(DoWhileStmt* s) = 0;    // do body while (cond);
    virtual int visit(ForStmt* s) = 0;        // for (init; cond; inc) body
    virtual int visit(SwitchStmt* s) = 0;     // switch (expr) { cases }
    virtual int visit(CaseClause* s) = 0;     // case val: stmts
    virtual int visit(BreakStmt* s) = 0;      // break;
    virtual int visit(ContinueStmt* s) = 0;   // continue;
    virtual int visit(ReturnStmt* s) = 0;     // return [expr];
    virtual int visit(FreeStmt* s) = 0;       // free(ptr);

    // --- Declaration visitors (retornan int) ---
    virtual int visit(VarDecl* d) = 0;        // int x = 5;
    virtual int visit(FunDecl* d) = 0;        // int main() { ... }
    virtual int visit(StructDecl* d) = 0;     // struct A { ... };
    virtual int visit(TemplateDecl* d) = 0;   // template<typename T> ...
    virtual int visit(Program* p) = 0;        // programa completo
};

// ============================================================
// ConstantFolding — Visitor para optimización en tiempo de compilación
// ============================================================
// Recorre el AST y evalúa las subexpresiones que son constantes
// (literales, operaciones aritméticas entre literales, etc.)
// reemplazándolas por su resultado numérico. Esto permite
// optimizar antes de la generación de código.
//
// Ejemplo:
//   int x = 2 + 3 * 4;  →  int x = 14;
//
// Implementa la misma interfaz Visitor pero retorna double (valor
// de la expresión) o int (control de statement).
class ConstantFolding : public Visitor {
public:
    // Expresiones
    double visit(BinaryOpNode* e) override;
    double visit(UnaryOpNode* e) override;
    double visit(AssignmentNode* e) override;
    double visit(FcallNode* e) override;
    double visit(MallocNode* e) override;
    double visit(IndexNode* e) override;
    double visit(MemberAccessNode* e) override;
    double visit(ArrowAccessNode* e) override;
    double visit(SizeOfNode* e) override;
    double visit(LambdaExprNode* e) override;
    double visit(CaptureNode* e) override;
    double visit(IdentifierNode* e) override;
    double visit(IntegerLiteralNode* e) override;
    double visit(FloatLiteralNode* e) override;
    double visit(BoolLiteralNode* e) override;
    double visit(CharLiteralNode* e) override;
    double visit(StringLiteralNode* e) override;
    double visit(PrintfNode* e) override;
    double visit(PrimitiveTypeNode* e) override;
    double visit(PointerTypeNode* e) override;
    double visit(StructTypeNode* e) override;
    double visit(NamedTypeNode* e) override;
    double visit(TemplateTypeNode* e) override;

    // Statements
    int visit(Body* s) override;
    int visit(ExprStmtNode* s) override;
    int visit(IfStmt* s) override;
    int visit(WhileStmt* s) override;
    int visit(DoWhileStmt* s) override;
    int visit(ForStmt* s) override;
    int visit(SwitchStmt* s) override;
    int visit(CaseClause* s) override;
    int visit(BreakStmt* s) override;
    int visit(ContinueStmt* s) override;
    int visit(ReturnStmt* s) override;
    int visit(FreeStmt* s) override;

    // Declarations
    int visit(VarDecl* d) override;
    int visit(FunDecl* d) override;
    int visit(StructDecl* d) override;
    int visit(TemplateDecl* d) override;
    int visit(Program* p) override;
};

// ============================================================
// Abstract TypeVisitor — Semantic Type Checking
// ============================================================
// Define la interfaz para el análisis semántico de tipos.
//   - Statements: retornan void (solo verifican, no evalúan)
//   - Expressions: retornan Type* (el tipo semántico resultante)
//
// Este visitor recorre el AST y construye el tipo de cada
// expresión, verificando que las operaciones sean válidas
// (suma solo entre aritméticos, asignaciones compatibles, etc.)
class TypeVisitor {
public:
    virtual ~TypeVisitor() = default;

    // --- Declaration and statement visitors (retornan void) ---
    virtual void visit(Program* p) = 0;
    virtual void visit(FunDecl* f) = 0;
    virtual void visit(VarDecl* v) = 0;
    virtual void visit(StructDecl* s) = 0;
    virtual void visit(TemplateDecl* d) = 0;
    virtual void visit(Body* b) = 0;
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

    // --- Expression visitors (retornan Type*) ---
    // Cada expresión devuelve su tipo semántico resuelto
    virtual ::Type* visit(BinaryOpNode* e) = 0;       // tipo del resultado aritmético/lógico
    virtual ::Type* visit(UnaryOpNode* e) = 0;         // -x (int), *p (base), &x (pointer)
    virtual ::Type* visit(AssignmentNode* e) = 0;      // tipo del target (lado izquierdo)
    virtual ::Type* visit(FcallNode* e) = 0;           // tipo de retorno de la función
    virtual ::Type* visit(MallocNode* e) = 0;          // void*
    virtual ::Type* visit(IndexNode* e) = 0;           // tipo base del arreglo
    virtual ::Type* visit(MemberAccessNode* e) = 0;    // tipo del miembro
    virtual ::Type* visit(ArrowAccessNode* e) = 0;     // tipo del miembro (vía puntero)
    virtual ::Type* visit(SizeOfNode* e) = 0;          // int
    virtual ::Type* visit(LambdaExprNode* e) = 0;      // void* (opaco)
    virtual ::Type* visit(CaptureNode* e) = 0;         // int (marcador)
    virtual ::Type* visit(IdentifierNode* e) = 0;      // tipo de la variable
    virtual ::Type* visit(IntegerLiteralNode* e) = 0;  // int
    virtual ::Type* visit(FloatLiteralNode* e) = 0;    // float
    virtual ::Type* visit(BoolLiteralNode* e) = 0;     // bool
    virtual ::Type* visit(CharLiteralNode* e) = 0;     // char
    virtual ::Type* visit(StringLiteralNode* e) = 0;   // int (dirección)
    virtual ::Type* visit(PrintfNode* e) = 0;          // void
    virtual ::Type* visit(PrimitiveTypeNode* e) = 0;   // el tipo primitivo mismo
    virtual ::Type* visit(PointerTypeNode* e) = 0;     // PointerType resultante
    virtual ::Type* visit(StructTypeNode* e) = 0;      // StructType correspondiente
    virtual ::Type* visit(NamedTypeNode* e) = 0;       // typename T (template)
    virtual ::Type* visit(TemplateTypeNode* e) = 0;    // tipo template instanciado
};

// ============================================================
// FuncInfo — Firma de función para type checking
// ============================================================
// Guarda el tipo de retorno y los tipos de cada parámetro
// de una función. Se usa en TypeChecker para verificar
// llamadas a función.
struct FuncInfo {
    ::Type* returnType;            // tipo de retorno
    vector<::Type*> paramTypes;    // tipos de cada parámetro
};

// ============================================================
// TypeChecker — Semantic Analysis Concrete Visitor
// ============================================================
// Recorre el AST y realiza verificación de tipos:
//   - Resuelve tipos de declaraciones (variables, funciones, structs)
//   - Infiere tipos con `auto`
//   - Verifica compatibilidad en asignaciones y operaciones
//   - Promueve tipos automáticamente (int→float→double)
//   - Instancia templates (funciones y structs)
//   - Asigna offsets con bin packing para el layout del stack frame
//
// Flujo:
//   1. typecheck(program) → visit(Program)
//   2. Registra todas las funciones → add_function()
//   3. Procesa globales, structs, templates, funciones
//   4. En visit(FunDecl): resuelve tipos de params, calcula offsets
//      con bin packing, asigna frame size, typecheckea el body
//   5. En visit(BinaryOpNode/UnaryOpNode/etc.): verifica operandos,
//      aplica promociones, retorna el tipo resultado
class TypeChecker : public TypeVisitor {
private:
    // --- Entornos de variables ---
    Environment<::Type*> env;            // nombre de variable → tipo semántico
    Environment<VarDecl*> varEnv;        // nombre de variable → nodo VarDecl (para binding)

    // --- Funciones declaradas ---
    unordered_map<string, FuncInfo> functions;           // nombre → firma
    unordered_map<string, StructType*> struct_types;     // nombre → StructType
    unordered_map<string, TemplateDecl*> template_decls; // nombre → declaración template
    unordered_map<string, FunDecl*> instantiated_function_cache; // key mangled → FunDecl instanciado

    // --- Tipos singleton (reutilizados, no creados en typeCache) ---
    ::Type* retornodefuncion;   // tipo de retorno de la función actual
    ::Type* intType;            // Type(INT)
    ::Type* boolType;           // Type(BOOL)
    ::Type* voidType;           // Type(VOID)
    ::Type* floatType;          // Type(FLOAT)
    ::Type* doubleType;         // Type(DOUBLE)
    ::Type* charType;           // Type(CHAR)

    // --- Cache y estado ---
    vector<::Type*> typeCache;  // tipos dinámicos creados (PointerType, ArrayType) para cleanup
    int loopDepth;              // profundidad de anidamiento de loops (para break/continue)
    int switchDepth;            // profundidad de switch (para break)
    vector<string> errors;      // errores semánticos acumulados
    bool hasError;              // flag de error
    int currentOffset = 0;      // offset actual (reservado, no usado actualmente)
    Program* program = nullptr; // programa actual (para registrar funciones instanciadas)

    // --- Funciones auxiliares ---
    TypeNode* semantic_to_type_node(::Type* t);    // Type* → TypeNode (para sustitución en templates)
    FunDecl* instantiate_function(TemplateDecl* tdecl, const vector<TypeNode*>& args);

    void add_function(FunDecl* fd);           // registra función en el mapa functions
    ::Type* type_from_ast(Exp* t);           // convierte TypeNode del AST → Type* semántico
    bool check_assign(::Type* target, ::Type* value);  // verifica compatibilidad de asignación
    void error(const string& msg);           // registra error semántico
    void error(const string& msg, const Location& loc);  // registra error con ubicación
    StructType* instantiate_template(const string& name, const vector<TypeNode*>& args);  // instancia struct template
    void bind_var_decl(VarDecl* v);           // asocia variable en environments
    bool checkFuncCall(const string& fname, FuncInfo& info, FcallNode* e);  // verifica argumentos de llamada

    // --- Recolectar variables y asignar offsets ---
    void collectVars(Stm* stmt, vector<VarDecl*>& vars);           // recolecta recursivamente vars
    int assignOffsets(vector<VarDecl*>& vars, int startOffset);    // asigna offsets (retorna offset final)

public:
    TypeChecker();
    ~TypeChecker();
    void typecheck(Program* program);  // entry point principal (imprime error y sale)
    bool check(Program* program);      // entry point alternativo (retorna bool)

    // --- Declaraciones y statements (retornan void) ---
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

    // --- Expresiones (retornan Type*) ---
    ::Type* visit(BinaryOpNode* e) override;
    ::Type* visit(UnaryOpNode* e) override;
    ::Type* visit(AssignmentNode* e) override;
    ::Type* visit(FcallNode* e) override;
    ::Type* visit(MallocNode* e) override;
    ::Type* visit(IndexNode* e) override;
    ::Type* visit(MemberAccessNode* e) override;
    ::Type* visit(ArrowAccessNode* e) override;
    ::Type* visit(SizeOfNode* e) override;
    ::Type* visit(LambdaExprNode* e) override;
    ::Type* visit(CaptureNode* e) override;
    ::Type* visit(IdentifierNode* e) override;
    ::Type* visit(IntegerLiteralNode* e) override;
    ::Type* visit(FloatLiteralNode* e) override;
    ::Type* visit(BoolLiteralNode* e) override;
    ::Type* visit(CharLiteralNode* e) override;
    ::Type* visit(StringLiteralNode* e) override;
    ::Type* visit(PrintfNode* e) override;
    ::Type* visit(PrimitiveTypeNode* e) override;
    ::Type* visit(PointerTypeNode* e) override;
    ::Type* visit(StructTypeNode* e) override;
    ::Type* visit(NamedTypeNode* e) override;
    ::Type* visit(TemplateTypeNode* e) override;
};

// ============================================================
// Abstract CodeGenVisitor — Code Generation
// ============================================================
// Define la interfaz para generar código de máquina a partir
// del AST. Cada método visit() genera las instrucciones en
// ensamblador correspondientes al nodo.
//
// Incluye métodos adicionales computeAddress() para obtener
// la dirección efectiva de un l-value (útil para &, arrays, etc.)
class CodeGenVisitor {
public:
    virtual ~CodeGenVisitor() = default;

    // --- Expression visitors (generan código que deja resultado en %rax) ---
    virtual void visit(BinaryOpNode* e) = 0;
    virtual void visit(UnaryOpNode* e) = 0;
    virtual void visit(AssignmentNode* e) = 0;
    virtual void visit(FcallNode* e) = 0;
    virtual void visit(IndexNode* e) = 0;
    virtual void visit(MemberAccessNode* e) = 0;
    virtual void visit(ArrowAccessNode* e) = 0;
    virtual void visit(MallocNode* e) = 0;
    virtual void visit(SizeOfNode* e) = 0;
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
    virtual void visit(CaptureNode* e) = 0;
    virtual void visit(LambdaExprNode* e) = 0;

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
    virtual void visit(Program* p) = 0;
    virtual void visit(TemplateDecl* d) = 0;

    // --- L-value address computation ---
    // computeAddress genera código para calcular la dirección
    // efectiva de un l-value y la deja en %rax.
    // Se usa para &x, arrays multidimensionales, structs, etc.
    virtual void computeAddress(UnaryOpNode* e) = 0;       // *p → dirección de p
    virtual void computeAddress(IdentifierNode* e) = 0;    // x → dirección de x
    virtual void computeAddress(IndexNode* e) = 0;          // a[i] → &a[i]
    virtual void computeAddress(MemberAccessNode* e) = 0;   // s.m → &s.m
    virtual void computeAddress(ArrowAccessNode* e) = 0;    // p->m → &(p->m)
};

// ============================================================
// GenCodeVisitor — x86-64 AT&T Code Generator
// ============================================================
// Genera código ensamblador AT&T para x86-64 a partir del AST.
// Recorre el árbol y escribe instrucciones al stream de salida.
//
// Características:
//   - Llamadas a función con convención System V AMD64
//   - Stack frame con %rbp, espacio para locales
//   - Bin packing para variables locales (ahorra memoria en stack)
//   - Manejo de arreglos multidimensionales con cálculo de índices
//   - Captura por valor en lambdas
//   - Strings como datos en .data
//   - Structs con offsets calculados
//
// Flujo:
//   1. generate(program) → asm header + visit(Program)
//   2. visit(Program) genera .data, procesa decls globales
//   3. visit(FunDecl): prólogo, asigna memoria local, procesa body, epílogo
//   4. Cada expresión genera instrucciones, dejando resultado en %rax
//   5. L-values se manejan con LVal (para asignaciones, ++, etc.)
class GenCodeVisitor : public CodeGenVisitor {
public:
    // -----------------------------------------------------------
    // LVal — Representa un l-value (destino de asignación)
    // -----------------------------------------------------------
    // Durante la generación de código, una expresión puede ser
    // un l-value (identificador, acceso por índice, acceso a
    // miembro, dereferencia). Guardamos la información necesaria
    // para leer o escribir en esa ubicación de memoria.
    //
    // Ejemplos:
    //   x       → LVal{Id, binding=&x}
    //   a[i]    → LVal{Index, binding=&a, indices=[i]}
    //   s.m     → LVal{Member, binding=&s, member="m"}
    //   *p      → LVal{Deref, ...}
    enum class LValKind { Invalid, Id, Index, Member, Deref };
    struct LVal {
        LValKind kind = LValKind::Invalid;  // tipo de l-value
        string name;                        // nombre de variable (para Id)
        VarDecl* binding = nullptr;         // VarDecl vinculado
        Exp* index = nullptr;               // índice simple
        vector<Exp*> indices;               // múltiples índices (multidim arrays)
        string member;                      // nombre de miembro (para Member/Deref)
        string structName;                  // nombre del struct contenedor
        bool isArrow = false;               // true si es p->m
    };

private:
    ostream &out;                           // stream de salida (ensamblador)
    LVal *lvalTarget = nullptr;             // l-value actual (para asignación)
    LVal captureLVal(Exp *e);               // analiza expresión como l-value
    void storeTarget(const LVal &lv);       // escribe %eax/%rax en la dirección del l-value

    // --- Gestión de variables y memoria ---
    void bind_var_decl(VarDecl* v);         // registra variable con su offset
    int array_elem_size(VarDecl* vd) const; // tamaño del elemento base del arreglo
    void loadBinding(VarDecl* vd);          // carga valor de variable en %rax
    void storeBinding(VarDecl* vd);         // guarda %eax en variable
    void leaBinding(VarDecl* vd);           // carga dirección de variable en %rax
    string bindingMem(VarDecl* vd) const;   // obtiene string "offset(%rbp)" para la variable

    // --- Acceso a arreglos multidimensionales ---
    void emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices);  // calcula &a[i][j] en %rax
    void emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices);     // carga a[i][j] en %rax
    void emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices, const string& valueReg);  // guarda en a[i][j]

    // --- Helpers de instrucciones según tamaño ---
    string instrSuffix(int size);    // "", "b", "w", "l", "q" según size
    string loadInstr(int size);      // "movzbq" (1), "movslq" (4), "movq" (8)
    string storeInstr(int size);     // "movb" (1), "movl" (4), "movq" (8)

    // --- Mapas de estado ---
    unordered_map<string, int> memoria;                      // nombre de variable → offset en stack
    unordered_map<string, bool> memoriaGlobal;               // nombre → true si es global
    unordered_map<string, int> structFieldCount;              // nombre struct → número de campos
    unordered_map<string, unordered_map<string, int>> structFieldOffset;     // struct → miembro → offset
    unordered_map<string, unordered_map<string, int>> structMemberSizes;     // struct → miembro → tamaño
    unordered_map<string, int> stringLabels;                  // string literal → número de label

    // --- Estado de generación ---
    int offset = -8;                         // desplazamiento actual en stack frame
    int labelcont = 0;                       // contador de labels (para saltos)
    bool inFunction = false;                 // true si estamos dentro de una función
    string currentBreakLabel;                // label de salto para break
    string currentContinueLabel;             // label de salto para continue
    string funcName;                         // nombre de la función actual
    string returnLabel;                      // label de retorno

public:
    explicit GenCodeVisitor(ostream &out) : out(out) {}

    void generate(Program *p);     // entry point: genera código completo

    // --- Expressions ---
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

    // --- Statements ---
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

    // --- Declarations ---
    void visit(VarDecl *d) override;
    void visit(FunDecl *d) override;
    void visit(StructDecl *d) override;
    void visit(Program *p) override;
    void visit(TemplateDecl *d) override;

    // --- L-value address computation ---
    void computeAddress(UnaryOpNode *e) override;
    void computeAddress(IdentifierNode *e) override;
    void computeAddress(IndexNode *e) override;
    void computeAddress(MemberAccessNode *e) override;
    void computeAddress(ArrowAccessNode *e) override;
};

#endif
