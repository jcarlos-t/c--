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

#include <functional>

// ============================================================
// Visitor
// ============================================================

class Visitor {
public:
    virtual ~Visitor() = default;

    // --- Expresiones ---
    virtual void visit(BinaryOpNode* e) = 0;
    virtual void visit(UnaryOpNode* e) = 0;
    virtual void visit(AssignmentNode* e) = 0;
    virtual void visit(FcallNode* e) = 0;
    virtual void visit(MallocNode* e) = 0;
    virtual void visit(IndexNode* e) = 0;          // arr[i] o ptr[i]
    virtual void visit(MemberAccessNode* e) = 0;   // obj.miembro
    virtual void visit(ArrowAccessNode* e) = 0;    // ptr->miembro
    virtual void visit(SizeOfNode* e) = 0;
    virtual void visit(LambdaExprNode* e) = 0;
    virtual void visit(CaptureNode* e) = 0;      // [x] en lambda
    virtual void visit(IdentifierNode* e) = 0;
    virtual void visit(IntegerLiteralNode* e) = 0;
    virtual void visit(FloatLiteralNode* e) = 0;
    virtual void visit(BoolLiteralNode* e) = 0;
    virtual void visit(CharLiteralNode* e) = 0;
    virtual void visit(StringLiteralNode* e) = 0;
    virtual void visit(PrintfNode* e) = 0;
    // Nodos de *tipo* en el AST 
    virtual void visit(PrimitiveTypeNode* e) = 0;  // int, char, void, ...
    virtual void visit(PointerTypeNode* e) = 0;    // T*
    virtual void visit(StructTypeNode* e) = 0;     // struct Nombre
    virtual void visit(NamedTypeNode* e) = 0;      // parámetro de template (ej. T)
    virtual void visit(TemplateTypeNode* e) = 0;   // Box<int>

    // --- Statements ---
    virtual void visit(Body* s) = 0;              // bloque { ... }
    virtual void visit(ExprStmtNode* s) = 0;       // expresión como statement
    virtual void visit(IfStmt* s) = 0;
    virtual void visit(WhileStmt* s) = 0;
    virtual void visit(DoWhileStmt* s) = 0;
    virtual void visit(ForStmt* s) = 0;
    virtual void visit(SwitchStmt* s) = 0;
    virtual void visit(CaseClause* s) = 0;         // case N: ...
    virtual void visit(BreakStmt* s) = 0;
    virtual void visit(ContinueStmt* s) = 0;
    virtual void visit(ReturnStmt* s) = 0;
    virtual void visit(FreeStmt* s) = 0;

    // --- Declaraciones ---
    virtual void visit(VarDecl* d) = 0;
    virtual void visit(FunDecl* d) = 0;
    virtual void visit(StructDecl* d) = 0;
    virtual void visit(TemplateDecl* d) = 0;     // template de struct o función
    virtual void visit(Program* p) = 0;          // raíz: globals, structs, funciones

    // --- Cálculo de dirección de l-values ---
    // Solo GenCodeVisitor los sobreescribe. Se usan para asignaciones y &:
    // en vez de cargar el valor de una variable, se calcula su dirección en memoria.
    // La implementación por defecto vacía permite que otros visitantes ignoren esto.
    virtual void computeAddress(UnaryOpNode* ) {}
    virtual void computeAddress(IdentifierNode* ) {}
    virtual void computeAddress(IndexNode* ) {}
    virtual void computeAddress(MemberAccessNode* ) {}
    virtual void computeAddress(ArrowAccessNode* ) {}
};

// ============================================================
// FuncInfo — Firma de función para el type checker
// ============================================================
// Resumen semántico de una función: no guarda el AST completo,
// solo los tipos necesarios para validar llamadas (FcallNode).
struct FuncInfo {
    ::Type* returnType;
    vector<::Type*> paramTypes;
};

// ============================================================
// TypeChecker — Análisis semántico
// ============================================================
// Recorre el AST, construye tablas de símbolos, valida reglas del lenguaje
// Esa información la consume GenCodeVisitor después.
class TypeChecker : public Visitor {
private:
    // env: nombre de variable → tipo semántico (Type*).
    // Soporta ámbitos anidados (función, bloque, for, lambda).
    Environment<::Type*> env;

    // varEnv: nombre → puntero al VarDecl original en el AST.
    // Permite enlazar IdentifierNode con su declaración (binding)
    // y leer offset/memSize calculados en visit(FunDecl).
    Environment<VarDecl*> varEnv;

    // Firmas de funciones normales registradas en visit(Program).
    unordered_map<string, FuncInfo> functions;

    // Structs ya resueltos: "Point" o instancias "Box<int>".
    unordered_map<string, StructType*> struct_types;

    // Plantillas declaradas (struct o función) antes de instanciar.
    unordered_map<string, TemplateDecl*> template_decls;

    // Cache de funciones template materializadas (ej. "max<int>" → FunDecl*).
    // Evita clonar la misma instancia varias veces.
    unordered_map<string, FunDecl*> instantiated_function_cache;

    // Tipo de retorno de la función/lambda que se está analizando ahora.
    // Se consulta en visit(ReturnStmt).
    ::Type* retornodefuncion;

    // Singletons de tipos primitivos (un solo objeto por tipo en todo el checker).
    ::Type* intType;
    ::Type* boolType;
    ::Type* voidType;
    ::Type* floatType;
    ::Type* doubleType;
    ::Type* charType;

    // Tipos creados dinámicamente durante el análisis (punteros, arreglos, errores).
    // El destructor de TypeChecker hace delete de todos.
    vector<::Type*> typeCache;

    // Profundidad de bucles/switch para validar break y continue.
    int loopDepth;
    int switchDepth;

    // Conjunto de variables que ya han sido inicializadas o asignadas.
    unordered_set<VarDecl*> initialized_vars;

    // Profundidad de funciones (0 = global, >=1 = dentro de función).
    int functionDepth = 0;

    // Flag para omitir chequeo de inicialización en contexto l-value.
    bool isLvalContext = false;

    // Acumulación de errores semánticos (no aborta en el primero).
    vector<string> errors;
    bool hasError;

    // Programa actual; se usa para añadir funciones instanciadas desde templates
    // a program->instantiated_functions y typechequearlas después.
    Program* program = nullptr;

    // Convierte Type* semántico → TypeNode* del AST (solo primitivos).
    // Necesario al instanciar funciones template: reemplazar NamedTypeNode("T")
    // por PrimitiveTypeNode(INT), etc.
    TypeNode* semantic_to_type_node(::Type* t);

    // Clona una FunDecl de template sustituyendo parámetros de tipo (T → int, ...).
    FunDecl* instantiate_function(TemplateDecl* tdecl, const vector<TypeNode*>& args);

    // Registra la firma de una función normal en el mapa functions.
    void add_function(FunDecl* fd);

    // Traduce nodos de tipo del AST (int, char*, struct X, Box<int>) a Type*.
    ::Type* type_from_ast(Exp* t);

    // Reglas de compatibilidad en asignaciones e inicializadores
    // (promoción int→float, truncamiento int→char, etc.).
    bool check_assign(::Type* target, ::Type* value);

    void error(const string& msg);
    void error(const string& msg, const Location& loc);

    // Materializa un struct template: Box<int> → StructType con miembros concretos.
    StructType* instantiate_template(const string& name, const vector<TypeNode*>& args);

    // Registra una variable en env (tipo) y varEnv (nodo VarDecl).
    void bind_var_decl(VarDecl* v);

    // Valida cantidad y tipos de argumentos en una llamada; reutilizado en FcallNode.
    bool checkFuncCall(const string& fname, FuncInfo& info, FcallNode* e);

    // Recorre el cuerpo de una función y junta todas las VarDecl locales
    // (incluso dentro de if/while/for) para calcular offsets antes de usarlas.
    void collectVars(Stm* stmt, vector<VarDecl*>& vars);

    // Asigna offset y memSize en el stack frame (bin packing en slots de 8 bytes).
    // Devuelve el tamaño total ocupado desde startOffset.
    int assignOffsets(vector<VarDecl*>& vars, int startOffset);

public:
    TypeChecker();
    ~TypeChecker();
    // Ejecuta el análisis; si hay errores imprime y termina el proceso (exit).
    void typecheck(Program* program);
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
// ConstantFolding — Evaluación de expresiones constantes en compile-time
// ============================================================
// Recorre el AST y, cuando una expresión solo depende de literales,
// la reemplaza por un único literal (marca isConstant / constantValue).
// Reduce trabajo en codegen e interpretación.
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
// GenCodeVisitor — Generador de código ensamblador x86-64 (sintaxis AT&T)
// ============================================================
// Emite instrucciones al ostream out. Usa resolvedType, offset, memSize
// y binding que dejó TypeChecker en los nodos del AST.
class GenCodeVisitor : public Visitor {
public:
    // Clasificación de expresiones que pueden aparecer a la izquierda de '=' o tras '&'.
    enum class LValKind { Invalid, Id, Index, Member, Deref };

    // Descripción de un l-value capturado durante el recorrido.
    // binding apunta al VarDecl cuando kind == Id o Index sobre variable.
    struct LVal {
        LValKind kind = LValKind::Invalid;
        string name;                    // nombre de variable (si aplica)
        VarDecl* binding = nullptr;     // enlace al VarDecl del TypeChecker
        Exp* index = nullptr;           // índice simple (legacy / 1D)
        vector<Exp*> indices;           // índices multidimensionales
        vector<string> members;         // lista de miembros para nested structs
        string structName;              // tipo del struct inicial para buscar offsets
        bool isArrow = false;           // true si fue ptr->miembro (vs obj.miembro)
    };

private:
    ostream &out;                       // destino del ensamblador (.s)

    // Si no es nullptr, visit() de expresiones l-value escribe aquí en vez de cargar valor.
    LVal *lvalTarget = nullptr;

    // Evalúa una expresión como l-value y devuelve su descripción (para asignaciones).
    LVal captureLVal(Exp *e);

    // Guarda el valor actual de %rax en el l-value descrito por lv.
    void storeTarget(const LVal &lv);

    // Registra offset/memoria de una VarDecl en el mapa interno memoria.
    void bind_var_decl(VarDecl* v);

    // Tamaño en bytes de un elemento del arreglo (base del ArrayType o memSize).
    int array_elem_size(VarDecl* vd) const;

    // Carga / guarda / dirección de una variable usando su binding (offset en %rbp).
    void loadBinding(VarDecl* vd);
    void storeBinding(VarDecl* vd);
    void leaBinding(VarDecl* vd);

    // Expresión de memoria AT&T: "-12(%rbp)" o "global(%rip)".
    string bindingMem(VarDecl* vd) const;

    // Arreglos: calcula dirección, carga o guarda con índices multidimensionales.
    void emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices);
    void emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices);
    void emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices, const string& valueReg);

    // Helpers según tamaño del operando: 1→b, 4→l, 8→q (convención x86-64).
    string instrSuffix(int size);
    string loadInstr(int size);   // ej. movzbq para char, movslq para int
    string storeInstr(int size);  // ej. movb, movl, movq

    // Offset negativo en stack frame por nombre de variable local/parámetro.
    unordered_map<string, int> memoria;

    // Variables globales: solo se marca presencia; la dirección es nombre(%rip).
    unordered_map<string, bool> memoriaGlobal;

    // Metadatos de structs para acceso a miembros y sizeof en codegen.
    unordered_map<string, int> structFieldCount;
    unordered_map<string, unordered_map<string, int>> structFieldOffset;
    unordered_map<string, unordered_map<string, int>> structMemberSizes;
    unordered_map<string, unordered_map<string, Type*>> structMemberTypes;

    // Literales string → índice de etiqueta .LstrN en .rodata
    unordered_map<string, int> stringLabels;

    // Próximo offset libre en el stack (negativo respecto a %rbp); avanza al declarar vars.
    int offset = -8;

    // Contador global para etiquetas únicas (.L0, .L1, ...) en saltos condicionales.
    int labelcont = 0;

    bool inFunction = false;          // true mientras se emite el cuerpo de una función
    string currentBreakLabel;         // etiqueta destino del break más interno
    string currentContinueLabel;      // etiqueta destino del continue más interno
    string funcName;                  // nombre de la función que se está generando
    string returnLabel;               // etiquapa común de salida de la función
    bool usedPow = false;             // true si el código fuente usa el operador **

public:
    explicit GenCodeVisitor(ostream &out) : out(out) {}

    // Punto de entrada: recorre el programa y escribe el .s completo.
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

    // Sobreescritura de la base Visitor: calculan dirección del l-value en %rax
    // activando lvalTarget antes de visitar el hijo correspondiente.
    void computeAddress(UnaryOpNode *e) override;
    void computeAddress(IdentifierNode *e) override;
    void computeAddress(IndexNode *e) override;
    void computeAddress(MemberAccessNode *e) override;
    void computeAddress(ArrowAccessNode *e) override;
};

#endif
