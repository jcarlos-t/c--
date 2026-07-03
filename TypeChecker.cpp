#include "visitor.h"
#include <iostream>
#include <sstream>


using namespace std;

// ============================================================
// TypeChecker — Análisis semántico (verificación de tipos)
// ============================================================
// Implementa Visitor sobre el AST: valida reglas del lenguaje,
// resuelve tipos y anota nodos para GenCodeVisitor.
//
// Salidas principales (campos anotados en el AST):
//   - Exp::resolvedType   — tipo semántico de cada expresión
//   - VarDecl::offset, memSize, resolvedType — layout en stack
//   - IdNode::binding — enlace al VarDecl de la variable
//   - FunDecl::frameSize — tamaño total del stack frame
//   - StructDecl::memberOffsets, memberSizes, totalSize
//
// Tablas de estado (ver visitor.h):
//   env / varEnv, functions, struct_types, template_decls, typeCache
//
// Flujo en visit(Program):
//   1. Registrar firmas de funciones (add_function)
//   2. Globals → structs → templates (solo registro)
//   3. Typecheck de cada función (visit FunDecl)
//   4. Funciones instanciadas desde templates
//
// Errores: se acumulan en errors[]; typecheck() hace exit(1) al final.
//
// Índice del archivo:
//   1. Helpers anónimos (is_integral_type, is_arithmetic_type, ...)
//   2. Constructor, type_from_ast, bind_var_decl, templates
//   3. check_assign, error, add_function, typecheck/check
//   4. visit(Program), visit(FunDecl), collectVars, assignOffsets
//   5. visit(declaraciones y statements)
//   6. visit(expresiones): binarias, llamadas, índices, lambdas, ...

// ============================================================
//  Funciones auxiliares de verificación de tipos (anónimas)
// ============================================================
// En namespace anónimo para evitar colisión con std::is_integral
// y std::is_arithmetic de <type_traits>.

namespace {

// is_integral_type: true si el tipo es entero (int, char, long)
//   Sirve para operaciones que solo aceptan enteros
//   y para promociones donde char → int automáticamente.
//
//   Ej: int → true    char → true    long → true
//       float → false double → false
//       bool → false   void → false
bool is_integral_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR || t->ttype == Type::LONG
           || t->ttype == Type::BOOL;
}

// is_arithmetic_type: true si el tipo es aritmético
//   (int, char, float, double)
//   Se usa en operaciones binarias (+, -, *, /, %)
//   y unarias (-, ++, --) que requieren operandos aritméticos.
//
//   Ej: int → true   char → true   float → true   double → true
//       bool → false void → false  struct → false  int* → false
bool is_arithmetic_type(Type* t) {
    return is_integral_type(t) ||
           t->ttype == Type::FLOAT ||
           t->ttype == Type::DOUBLE;
}

// is_switch_index_type: tipos válidos para expresión de switch y case
//   Solo int y char están permitidos como índice de switch.
//
//   Ej: int → true   char → true
//       float → false  double → false
bool is_switch_index_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR || t->ttype == Type::BOOL;
}

} // namespace

// ============================================================
// Constructor / Destructor
// ============================================================
// Crea los tipos singleton que se reutilizan durante todo
// el typechecking. Estos tipos no se guardan en typeCache
// porque no se crean con new dinámico en cada uso.
//
// typeCache: almacena tipos creados dinámicamente (PointerType,
// ArrayType, StructType) para poder liberarlos en el destructor.

TypeChecker::TypeChecker() {
    intType = new Type(Type::INT);       // representa el tipo int
    boolType = new Type(Type::BOOL);     // representa bool
    voidType = new Type(Type::VOID);     // representa void
    floatType = new Type(Type::FLOAT);   // representa float
    doubleType = new Type(Type::DOUBLE); // representa double
    charType = new Type(Type::CHAR);     // representa char
    longType = new Type(Type::LONG);     // representa long long
    retornodefuncion = nullptr;          // se setea al entrar a cada función
    loopDepth = 0;                       // sin loops activos
    switchDepth = 0;                     // sin switches activos
    functionDepth = 0;
    hasError = false;
}

TypeChecker::~TypeChecker() {
    // Limpia tipos dinámicos creados durante el typecheck
    for (auto t : typeCache) delete t;
    for (auto& [k, v] : struct_types) delete v;
    delete intType;
    delete boolType;
    delete voidType;
    delete floatType;
    delete doubleType;
    delete charType;
    delete longType;
}

// ============================================================
// type_from_ast — convertir nodo tipo del AST a Type* semántico
// ============================================================
// Toma un nodo del AST que representa un tipo (TypeNode) y lo
// convierte al tipo semántico correspondiente (Type*).
//
// Casos:
//   int, char, float, double, bool, void → tipo primitivo singleton
//   int*, char**, etc. → PointerType(base)
//   struct Persona → StructType("Persona") buscado en struct_types
//
//   Ej: type_from_ast(PrimitiveTypeNode(INT)) → intType
//       type_from_ast(PointerTypeNode(IntTypeNode)) → PointerType(IntType)
Type* TypeChecker::type_from_ast(TypeNode* t) {
    Type* res = nullptr;
    // --- Tipos primitivos: void, int, char, float, double, bool, long ---
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(t)) {
        switch (pt->prim) {
            case PrimitiveTypeNode::VOID:   res = voidType; break;
            case PrimitiveTypeNode::INT:    res = intType; break;
            case PrimitiveTypeNode::CHAR:   res = charType; break;
            case PrimitiveTypeNode::FLOAT:  res = floatType; break;
            case PrimitiveTypeNode::DOUBLE: res = doubleType; break;
            case PrimitiveTypeNode::BOOL:   res = boolType; break;
            case PrimitiveTypeNode::LONG:   res = longType; break;
        }
        if (res && (pt->isUnsigned || pt->isConst)) {
            // Clonar tipo para aplicar modificadores (evitar modificar singletons)
            Type* clone = new Type(res->ttype);
            clone->isUnsigned = pt->isUnsigned;
            clone->isConst = pt->isConst;
            typeCache.push_back(clone);
            res = clone;
        }
        return res;
    }
    // --- Punteros: T* busca el tipo base recursivamente ---
    if (auto* pt = dynamic_cast<PointerTypeNode*>(t)) {
        Type* base = type_from_ast(pt->base);
        PointerType* ptr = new PointerType(base);
        ptr->isConst = pt->isConst;
        typeCache.push_back(ptr);
        return ptr;
    }
    // --- Structs: busca el StructType por nombre ---
    if (auto* st = dynamic_cast<StructTypeNode*>(t)) {
        auto it = struct_types.find(st->name);
        if (it != struct_types.end()) {
            res = it->second;
            if (st->isConst) {
                StructType* clone = new StructType(st->name);
                clone->members = ((StructType*)res)->members;
                clone->isConst = true;
                typeCache.push_back(clone);
                res = clone;
            }
            return res;
        }
        error("struct '" + st->name + "' no declarado.");
        return intType;
    }
    return intType; // fallback
}

// ============================================================
// bind_var_decl — registrar variable en environments
// ============================================================
// Después de resolver el tipo de una variable, este método
// la agrega al environment de tipos (env) y al environment
// de declaraciones (varEnv) para que pueda ser referenciada
// por identificadores y capturas de lambda.
//
//   Ej: int x = 5;
//       env["x"] = IntType
//       varEnv["x"] = &VarDecl{name:"x", resolvedType:IntType, ...}
void TypeChecker::bind_var_decl(VarDecl* v) {
    env.add_var(v->name, v->resolvedType);
    varEnv.add_var(v->name, v);
}

// ============================================================
// check_assign — verificar compatibilidad de tipos en asignación
// ============================================================
// Determina si un valor de tipo `value` puede asignarse a un
// destino de tipo `target`. Permite:
//   - Tipos idénticos (match)
//   - Puntero a puntero (compatible por coerción)
//   - char ↔ int (truncamiento/promoción)
//   - int/char → float/double (promoción aritmética)
//   - float → double (promoción)
//
//   Ej: check_assign(int, int) → true
//       check_assign(int, float) → false
//       check_assign(float, int) → true  (promoción)
//       check_assign(double, int) → true (promoción)
//       check_assign(char, int) → true
bool TypeChecker::check_assign(Type* target, Type* value) {
    // Check for exact match (ignoring isConst for assignability, since const is checked elsewhere)
    if (target->ttype == value->ttype && target->isUnsigned == value->isUnsigned) return true;
    // Cualquier puntero a cualquier puntero (coerción laxa; no compara base).
    if (target->ttype == Type::POINTER && value->ttype == Type::POINTER) return true;
    // Truncamiento / promoción entre char, int, long (ignoring unsigned for implicit conversions like C)
    if (target->ttype == Type::CHAR && (value->ttype == Type::INT || value->ttype == Type::LONG)) return true;
    if (target->ttype == Type::INT && (value->ttype == Type::CHAR || value->ttype == Type::LONG)) return true;
    if (target->ttype == Type::LONG && is_integral_type(value)) return true;
    // Allow assigning between same type even if unsigned differs (like C does implicitly)
    if (target->ttype == value->ttype && is_integral_type(target)) return true;
    // Promoción: int/char/long → float o double
    if ((target->ttype == Type::FLOAT || target->ttype == Type::DOUBLE) && is_integral_type(value)) return true;
    // Promoción: float → double
    if (target->ttype == Type::DOUBLE && value->ttype == Type::FLOAT) return true;
    // Narrowing: double → float (C compatible)
    if (target->ttype == Type::FLOAT && value->ttype == Type::DOUBLE) return true;
    // Truncación: float/double → int/char/long (C compatible)
    if (is_integral_type(target) && (value->ttype == Type::FLOAT || value->ttype == Type::DOUBLE)) return true;
    // Conversión implícita: int/char/long → bool
    if (target->ttype == Type::BOOL && is_integral_type(value)) return true;
    // Conversión implícita: bool → int/char/long
    if (is_integral_type(target) && value->ttype == Type::BOOL) return true;
    return false;
}

// ============================================================
// error — registrar error semántico (no aborta)
// ============================================================
// Acumula errores en lugar de abortar, permitiendo reportar
// múltiples errores en una sola pasada. Al final, typecheck()
// y check() verifican si hay errores y deciden qué hacer.
void TypeChecker::error(const string& msg) {
    errors.push_back(msg);
    hasError = true;
}

// error con Location: incluye línea y columna del error
void TypeChecker::error(const string& msg, const Location& loc) {
    ostringstream oss;
    if (loc.line > 0)
        oss << "line " << loc.line << ":" << loc.column << " - " << msg;
    else
        oss << msg;
    errors.push_back(oss.str());
    hasError = true;
}

// ============================================================
// add_function — registrar función en el mapa de funciones
// ============================================================
// Almacena la firma de una función (tipo de retorno + tipos de
// parámetros) para poder verificar llamadas posteriormente.
// Detecta redeclaraciones.
void TypeChecker::add_function(FunDecl* fd) {
    if (functions.find(fd->name) != functions.end()) {
        error("función '" + fd->name + "' ya declarada.");
        return;
    }
    Type* returnType = type_from_ast(fd->return_type);
    FuncInfo info;
    info.returnType = returnType;
    for (auto p : fd->params) {
        info.paramTypes.push_back(type_from_ast(p->type));
    }
    functions[fd->name] = info;
}

// ============================================================
// typecheck — entry point principal (imprime error y sale)
// ============================================================
// Inicia el análisis semántico del programa. Si encuentra
// errores, los imprime en stderr y termina con exit(1).

void TypeChecker::typecheck(Program* program) {
    hasError = false;
    errors.clear();
    if (program) program->accept(this);
    if (hasError) {
        for (auto& e : errors) cerr << "Error semántico: " << e << endl;
        exit(1);
    }
}

// ============================================================
//  Visits: declaraciones y statements
// ============================================================

// -----------------------------------------------------------
// visit(Program) — punto de entrada del recorrido
// -----------------------------------------------------------
// Procesa todas las declaraciones del programa en orden:
//   1. Registrar todas las funciones primero (add_function)
//   2. Abrir scope global
//   3. Procesar variables globales
//   4. Procesar declaraciones de struct
//   5. Procesar funciones (typecheck interno)
//   6. Cerrar scope global
void TypeChecker::visit(Program* p) {
    for (auto f : p->functions) add_function(f);
    env.add_level();
    varEnv.add_level();
    for (auto g : p->globals) g->accept(this);
    for (auto s : p->structs) s->accept(this);
    for (auto f : p->functions) f->accept(this);
    varEnv.remove_level();
    env.remove_level();
}

// -----------------------------------------------------------
// visit(FunDecl) — typecheck de una función
// -----------------------------------------------------------
// Flujo completo:
//   1. Abrir scope para parámetros y variables locales
//   2. Guardar tipo de retorno
//   3. Resolver tipos de parámetros, asignar offsets (8 bytes por variable)
//   4. Recolectar variables locales recursivamente (collectVars)
//   5. Resolver tipos de locales, inferir auto si es necesario
//   6. Asignar offsets a locales (después de parámetros)
//   7. Calcular frameSize (alineado a 16 bytes)
//   8. Convertir offsets a negativos (relativos a %rbp)
//   9. Vincular parámetros en environment
//   10. Typecheckear el body de la función
//   11. Verificar que funciones no-void tengan return en todos los caminos
//   12. Cerrar scope
//
//   Ej: int suma(int a, int b) { return a + b; }
//       → params: a@(-8), b@(-12), locals: vacío
//       → frameSize: 16
//       → body: verifica return int, verifica tipos de a + b
void TypeChecker::visit(FunDecl* f) {
    env.add_level();
    varEnv.add_level();
    functionDepth++;

    retornodefuncion = type_from_ast(f->return_type);

    // ---- Recolectar parámetros y resolver sus tipos ----
    vector<VarDecl*> params;
    for (auto p : f->params) {
        Type* t = type_from_ast(p->type);
        if (t->match(voidType))
            error("parámetro no puede ser de tipo void.");
        p->memSize = t->size();
        p->resolvedType = t;
        params.push_back(p);
        initialized_vars.insert(p);
    }

    // Asignar offsets a parámetros (8 bytes por variable)
    // Los parámetros van al inicio del stack frame
    int paramSize = assignOffsets(params, 0);

    // ---- Recolectar todas las variables locales del body ----
    vector<VarDecl*> localVars;
    collectVars(f->body, localVars);

    // Resolver tipos de variables locales (sin agregar al environment todavía)
    for (auto v : localVars) {
        if (v->resolvedType == nullptr) {
            Type* t = type_from_ast(v->type);
            if (t->match(voidType)) {
                error("no se puede declarar variable de tipo void.");
                t = intType;
            }
            // Wrap en ArrayType si tiene dimensiones de arreglo (orden inverso)
            // Ej: int a[5][3] → ArrayType(ArrayType(IntType, 3), 5)
            for (int idx = (int)v->array_sizes.size() - 1; idx >= 0; idx--) {
                int dim = -1;
                if (auto* il = dynamic_cast<NumberLiteralNode*>(v->array_sizes[idx]))
                    dim = (int)il->value;
                ArrayType* at = new ArrayType(t, dim);
                typeCache.push_back(at);
                t = at;
            }
            v->resolvedType = t;
            v->memSize = t->size();
        }
    }

    // Asignar offsets a variables locales (después de parámetros)
    int totalSize = assignOffsets(localVars, paramSize);

    // Alinear a 16 bytes (convención System V)
    f->frameSize = (totalSize + 15) & ~15;

    // Convertir offsets positivos (desde inicio del frame) a negativos respecto a %rbp.
    // GenCode usa -8(%rbp), -16(%rbp), etc. (stack crece hacia abajo).
    for (auto p : params) {
        p->offset = p->offset - f->frameSize;
    }
    for (auto v : localVars) {
        v->offset = v->offset - f->frameSize;
    }

    // Agregar parámetros al environment
    for (auto p : params) {
        bind_var_decl(p);
    }

    // Typecheckear el body
    f->body->accept(this);

    // Comprobación superficial de retorno: solo mira el último statement del body.
    // No analiza todos los caminos (if sin else al final, etc. pueden pasar o fallar).
    if (!retornodefuncion->match(voidType)) {
        bool endsWithReturn = false;
        if (!f->body->stmts.empty()) {
            Stm* last = f->body->stmts.back();
            endsWithReturn = (dynamic_cast<ReturnStmt*>(last) != nullptr);
            if (!endsWithReturn) {
                if (auto* ifst = dynamic_cast<IfStmt*>(last)) {
                    endsWithReturn = ifst->else_branch != nullptr;
                }
            }
        }
        if (!endsWithReturn)
            error("función no-void no garantiza retorno en todos los caminos.", f->loc);
    }

    varEnv.remove_level();
    env.remove_level();
    functionDepth--;
}

// -----------------------------------------------------------
// collectVars — recolectar todas las declaraciones de variables
// -----------------------------------------------------------
// Recorre recursivamente un statement (y sus sub-statements)
// para recolectar todas las declaraciones de variables.
// Se usa para calcular offsets de stack frame antes de
// procesar el body.
//
//   Ej: { int a; if (c) { int b; } int c; }
//       → vars = [a, b, c]
// Recorre if/while/for/switch pero no abre scopes: todas las VarDecl locales
// comparten el mismo frame (como en C tradicional con gcc -O0).
void TypeChecker::collectVars(Stm* stmt, vector<VarDecl*>& vars) {
    if (!stmt) return;

    if (auto* v = dynamic_cast<VarDecl*>(stmt)) {
        vars.push_back(v);
    }

    if (auto* body = dynamic_cast<Body*>(stmt)) {
        for (auto s : body->stmts) {
            collectVars(s, vars);
        }
    }

    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        collectVars(ifStmt->then_branch, vars);
        if (ifStmt->else_branch) collectVars(ifStmt->else_branch, vars);
    }

    if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        collectVars(whileStmt->body, vars);
    }

    if (auto* forStmt = dynamic_cast<ForStmt*>(stmt)) {
        if (forStmt->init) collectVars(forStmt->init, vars);
        collectVars(forStmt->body, vars);
    }

    if (auto* doWhileStmt = dynamic_cast<DoWhileStmt*>(stmt)) {
        collectVars(doWhileStmt->body, vars);
    }

    if (auto* switchStmt = dynamic_cast<SwitchStmt*>(stmt)) {
        for (auto cc : switchStmt->cases) {
            for (auto s : cc->body) {
                collectVars(s, vars);
            }
        }
        for (auto s : switchStmt->default_body) {
            collectVars(s, vars);
        }
    }
}

// -----------------------------------------------------------
// assignOffsets — asignar offsets en el stack frame (bin packing)
// -----------------------------------------------------------
// Empaqueta variables en bloques de 8 bytes según su tamaño:
//   - Variables >8B: tamaño real (arrays, structs grandes)
//   - Variables de 8B: 1 por bloque (double, long, punteros)
//   - Variables de 4B: 2 por bloque (int, float)
//   - Variables de 1B: 4 por bloque (char, bool) en slots de 2B
//
// Orden de llenado: más pesados primero para optimizar alineamiento.
//
// Retorna el offset final (startOffset + bytes ocupados).
int TypeChecker::assignOffsets(vector<VarDecl*>& vars, int startOffset) {
    vector<VarDecl*> large, size8, size4, size1;
    
    for (auto v : vars) {
        if (v->memSize > 8) {
            large.push_back(v);
        } else if (v->memSize == 8) {
            size8.push_back(v);
        } else if (v->memSize == 4) {
            size4.push_back(v);
        } else if (v->memSize == 1) {
            size1.push_back(v);
        }
    }
    
    int nextOffset = startOffset;
    
    for (auto v : large) {
        v->offset = nextOffset;
        nextOffset += v->memSize;
    }
    
    for (auto v : size8) {
        v->offset = nextOffset;
        nextOffset += 8;
    }
    
    for (size_t i = 0; i < size4.size(); i += 2) {
        size4[i]->offset = nextOffset;
        if (i + 1 < size4.size()) {
            size4[i + 1]->offset = nextOffset + 4;
        }
        nextOffset += 8;
    }
    
    for (size_t i = 0; i < size1.size(); i += 4) {
        int blockStart = nextOffset;
        for (size_t j = 0; j < 4 && i + j < size1.size(); j++) {
            size1[i + j]->offset = blockStart + (j * 2);
        }
        nextOffset += 8;
    }
    
    return nextOffset;
}

// -----------------------------------------------------------
// visit(VarDecl) — typecheck de declaración de variable
// -----------------------------------------------------------
// Si la variable ya fue procesada en FunDecl (resolvedType != null),
// solo verifica el inicializador y la bindea. Si no, resuelve
// el tipo desde el AST, infiere auto si aplica, y verifica
// compatibilidad del inicializador.
//
//   Ej: int x = 5;  → resolvedType = IntType, verifica 5 es int
//       int a[5];   → resolvedType = ArrayType(IntType, 5)
void TypeChecker::visit(VarDecl* v) {
    if (v->resolvedType != nullptr) {
        // Variable ya procesada en FunDecl (resolvedType pre-asignado)
        if (env.check_current(v->name)) {
            error("variable '" + v->name + "' ya declarada en este ámbito.");
            return;
        }
        // Verificar compatibilidad del inicializador con el tipo declarado
        if (v->initializer) {
            v->initializer->accept(this); Type* initType = v->initializer->resolvedType;
            if (!check_assign(v->resolvedType, initType)) {
                error("tipos incompatibles en inicialización de '" + v->name + "'.");
            }
        }
        bind_var_decl(v);
        // Validar lista de inicialización
        if (!v->init_list.empty()) {
            Type* t = v->resolvedType;
            if (t->ttype != Type::ARRAY) {
                error("lista de inicialización '{...}' solo válida para arreglos.");
            } else {
                ArrayType* at = (ArrayType*)t;
                if (at->length > 0 && (int)v->init_list.size() > at->length) {
                    error("demasiados elementos en inicialización del arreglo '" + v->name + "'.");
                }
                Type* elemType = at->base;
                for (auto initExpr : v->init_list) {
                    initExpr->accept(this);
                    if (!check_assign(elemType, initExpr->resolvedType)) {
                        error("tipo incompatible en elemento de inicialización del arreglo '" + v->name + "'.");
                    }
                }
            }
        }
        if (v->initializer || !v->init_list.empty() || v->resolvedType->ttype == Type::STRUCT ||
            v->resolvedType->ttype == Type::ARRAY) {
            initialized_vars.insert(v);
        }
        return;
    }

    // Variable no procesada (global o local sin pre-asignación)
    Type* t = type_from_ast(v->type);

    if (t->match(voidType)) {
        error("no se puede declarar variable de tipo void.");
        return;
    }

    // Wrap en ArrayType si tiene dimensiones (orden inverso: primera dimensión = outermost)
    for (int idx = (int)v->array_sizes.size() - 1; idx >= 0; idx--) {
        int dim = -1;
        if (auto* il = dynamic_cast<NumberLiteralNode*>(v->array_sizes[idx]))
            dim = (int)il->value;
        ArrayType* at = new ArrayType(t, dim);
        typeCache.push_back(at);
        t = at;
    }

    // Verificar redeclaración en el ámbito actual
    if (env.check_current(v->name)) {
        error("variable '" + v->name + "' ya declarada en este ámbito.");
        return;
    }

    // Guardar tipo y tamaño resueltos
    v->resolvedType = t;
    v->memSize = t->size();

    bind_var_decl(v);

    // Verificar compatibilidad del inicializador
    if (v->initializer) {
        v->initializer->accept(this); Type* initType = v->initializer->resolvedType;
        if (!check_assign(t, initType)) {
            error("tipos incompatibles en inicialización de '" + v->name + "'.");
        }
    }
    // Verificar lista de inicialización para arreglos
    if (!v->init_list.empty()) {
        if (t->ttype != Type::ARRAY) {
            error("lista de inicialización '{...}' solo válida para arreglos.");
        } else {
            ArrayType* at = (ArrayType*)t;
            if (at->length > 0 && (int)v->init_list.size() > at->length) {
                error("demasiados elementos en inicialización del arreglo '" + v->name + "'.");
            }
            Type* elemType = at->base;
            for (auto initExpr : v->init_list) {
                initExpr->accept(this);
                if (!check_assign(elemType, initExpr->resolvedType)) {
                    error("tipo incompatible en elemento de inicialización del arreglo '" + v->name + "'.");
                }
            }
        }
    }
    // Marcar como inicializada: variables con inicializador explícito,
    //       y variables de tipo struct/arreglo (se inicializan campo a campo)
    if (v->initializer || !v->init_list.empty() || t->ttype == Type::STRUCT || t->ttype == Type::ARRAY) {
        initialized_vars.insert(v);
    }
}

// -----------------------------------------------------------
// visit(StructDecl) — typecheck de declaración de struct
// -----------------------------------------------------------
// Crea un StructType con los miembros y sus tipos. Calcula
// los offsets de cada miembro secuencialmente.
//
//   Ej: struct Punto { int x; float y; };
//       → StructType("Punto")
//         members: {"x": IntType(offset=0), "y": FloatType(offset=4)}
//         totalSize: 8
void TypeChecker::visit(StructDecl* s) {
    StructType* st = new StructType(s->name);
    struct_types[s->name] = st;   // Registrar antes de procesar miembros (soporta auto-referencia)
    int offset = 0;

    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);

        // Wrap en ArrayType si tiene dimensiones (orden inverso: primera dimensión = outermost)
        for (int idx = (int)m->array_sizes.size() - 1; idx >= 0; idx--) {
            int dim = -1;
            if (auto* il = dynamic_cast<NumberLiteralNode*>(m->array_sizes[idx]))
                dim = (int)il->value;
            ArrayType* at = new ArrayType(mt, dim);
            typeCache.push_back(at);
            mt = at;
        }

        m->resolvedType = mt;
        st->members[m->name] = mt;
        s->memberOffsets[m->name] = offset;
        s->memberSizes[m->name] = mt->size();
        offset += mt->size();
    }

    s->totalSize = offset;
}

// -----------------------------------------------------------
// visit(Body) — typecheck de bloque { ... }
// -----------------------------------------------------------
// Abre un nuevo ámbito (scope) para las variables declaradas
// dentro del bloque. Bodies sintéticos (comma-decl) no crean scope.
void TypeChecker::visit(Body* b) {
    if (!b->synthetic) {
        env.add_level();
        varEnv.add_level();
    }
    for (auto s : b->stmts) s->accept(this);
    if (!b->synthetic) {
        varEnv.remove_level();
        env.remove_level();
    }
}

// Expresión como statement: solo verifica tipos de la expresión (descarta el valor).
void TypeChecker::visit(ExprStmtNode* s) {
    if (s->expr) s->expr->accept(this);
}

// -----------------------------------------------------------
// visit(IfStmt) — typecheck de if/else
// -----------------------------------------------------------
// Verifica que la condición sea bool, luego procesa las ramas.
//
//   Ej: if (x > 0) { ... } else { ... }
//       → condición x > 0 debe ser bool
void TypeChecker::visit(IfStmt* s) {
    s->condition->accept(this); Type* t = s->condition->resolvedType;
    if (t->match(voidType)) {
        error("condición de if no puede ser void.");
    }
    s->then_branch->accept(this);
    if (s->else_branch) s->else_branch->accept(this);
}

// -----------------------------------------------------------
// visit(WhileStmt) — typecheck de while
// -----------------------------------------------------------
// Incrementa loopDepth (para break/continue), verifica la
// condición y procesa el cuerpo.
void TypeChecker::visit(WhileStmt* s) {
    s->condition->accept(this); Type* t = s->condition->resolvedType;
    if (t->match(voidType)) {
        error("condición de while no puede ser void.");
    }
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
}

// -----------------------------------------------------------
// visit(DoWhileStmt) — typecheck de do-while
// -----------------------------------------------------------
// Procesa el cuerpo primero (con loopDepth), luego verifica
// la condición.
void TypeChecker::visit(DoWhileStmt* s) {
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
    s->condition->accept(this); Type* t = s->condition->resolvedType;
    if (t->match(voidType)) {
        error("condición de do-while no puede ser void.");
    }
}

// -----------------------------------------------------------
// visit(ForStmt) — typecheck de for
// -----------------------------------------------------------
// Abre un ámbito para la variable de inicialización, procesa
// init, condición, cuerpo e incremento.
//
//   Ej: for (int i = 0; i < 10; i++) { ... }
void TypeChecker::visit(ForStmt* s) {
    env.add_level();
    varEnv.add_level();
    if (s->init) s->init->accept(this);
    if (s->condition) {
        s->condition->accept(this); Type* t = s->condition->resolvedType;
        if (t->match(voidType)) {
            error("condición de for no puede ser void.");
        }
    }
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
    if (s->increment) s->increment->accept(this);
    varEnv.remove_level();
    env.remove_level();
}

// -----------------------------------------------------------
// visit(SwitchStmt) — typecheck de switch
// -----------------------------------------------------------
// Verifica que la expresión sea int o char, incrementa
// switchDepth (para break), procesa casos y default.
//
//   Ej: switch (x) { case 1: ...; break; default: ...; }
void TypeChecker::visit(SwitchStmt* s) {
    s->expr->accept(this); Type* t = s->expr->resolvedType;
    if (!is_switch_index_type(t)) {
        error("expresión de switch debe ser int o char.");
    }
    switchDepth++;
    for (auto cc : s->cases) cc->accept(this);
    for (auto st : s->default_body) st->accept(this);
    switchDepth--;
}

// Valida el literal del case y typecheckea el cuerpo. visit(SwitchStmt) ya validó
// que la expresión del switch sea int/char.
void TypeChecker::visit(CaseClause* s) {
    s->value->accept(this); Type* t = s->value->resolvedType;
    if (!is_switch_index_type(t)) {
        error("valor de case debe ser int o char.");
    }
    for (auto st : s->body) st->accept(this);
}

// free(ptr): solo verifica que la expresión sea typecheckeable (debería ser puntero).
void TypeChecker::visit(FreeStmt* s) {
    s->expr->accept(this);
}

// -----------------------------------------------------------
// visit(BreakStmt) — verificar break en contexto válido
// -----------------------------------------------------------
// break solo es válido dentro de un loop o switch.
void TypeChecker::visit(BreakStmt* s) {
    if (loopDepth == 0 && switchDepth == 0) {
        error("break fuera de ciclo o switch.", s->loc);
    }
}

// -----------------------------------------------------------
// visit(ContinueStmt) — verificar continue en contexto válido
// -----------------------------------------------------------
// continue solo es válido dentro de un loop.
void TypeChecker::visit(ContinueStmt* s) {
    if (loopDepth == 0) {
        error("continue fuera de ciclo.", s->loc);
    }
}

// -----------------------------------------------------------
// visit(ReturnStmt) — verificar return
// -----------------------------------------------------------
// Verifica que:
//   - Función void no retorne valor
//   - Función no-void retorne un valor
//   - El tipo del valor sea compatible con el tipo de retorno
//
//   Ej: int suma() { return 5; } → ok
//       void nada() { return; } → ok
//       int suma() { return; } → error
//       void nada() { return 5; } → error
void TypeChecker::visit(ReturnStmt* s) {
    if (retornodefuncion->match(voidType) && s->expr) {
        error("función void no debe retornar valor.", s->loc);
        return;
    }
    if (!retornodefuncion->match(voidType) && !s->expr) {
        error("función no-void debe retornar un valor.", s->loc);
        return;
    }
    if (s->expr) {
        s->expr->accept(this); Type* t = s->expr->resolvedType;
        if (!check_assign(retornodefuncion, t)) {
            error("tipo de retorno incompatible (esperaba " +
                  retornodefuncion->str() + ").", s->loc);
        }
    }
}

// ============================================================
//  Expression type checking
// ============================================================

// -----------------------------------------------------------
// visit(BinaryOpNode) — typecheck de operaciones binarias
// -----------------------------------------------------------
// Determina el tipo resultante de una operación binaria según
// los tipos de los operandos. Las reglas son:
//
//   Aritméticas (+, -, *, /, %, ^):
//     - Ambos operandos deben ser aritméticos (int, char, float, double)
//     - Promoción automática: si alguno es double → double
//       si no, si alguno es float → float
//       si no → int (char promueve a int)
//
//   Comparación (==, !=, <, >, <=, >=):
//     - Ambos operandos deben ser aritméticos
//     - Resultado siempre bool
//
//   Lógicos (&&, ||):
//     - Ambos operandos deben ser bool
//     - Resultado bool
//
//   Ej: 3 + 4 → int
//       3.0 + 4 → float (promoción)
//       3.0 + 4.0 → float
//       3 > 4 → bool
//       true && false → bool
void TypeChecker::visit(BinaryOpNode* e) {
    e->left->accept(this); Type* left = e->left->resolvedType;
    e->right->accept(this); Type* right = e->right->resolvedType;

    Type* resultType;
    switch (e->op) {
        // Operaciones aritméticas: permiten int, float o double, promueven al tipo más grande
        case BinaryOp::ADD: case BinaryOp::SUB:
        case BinaryOp::MUL: case BinaryOp::DIV: case BinaryOp::MOD:
        case BinaryOp::POW:
            // Aritmética de punteros: ptr + int o int + ptr o ptr - int o ptr - ptr
            if ((left->ttype == Type::POINTER && is_integral_type(right)) || 
                (is_integral_type(left) && right->ttype == Type::POINTER && e->op == BinaryOp::ADD)) {
                // ptr + int o int + ptr → mismo tipo de puntero
                resultType = left->ttype == Type::POINTER ? left : right;
            } else if (left->ttype == Type::POINTER && right->ttype == Type::POINTER && e->op == BinaryOp::SUB) {
                // ptr - ptr → int
                resultType = intType;
            } else if (left->ttype == Type::POINTER && is_integral_type(right) && e->op == BinaryOp::SUB) {
                // ptr - int → mismo tipo de puntero
                resultType = left;
            } else if (is_arithmetic_type(left) && is_arithmetic_type(right)) {
                // Promoción automática: si alguno es double, resultado double
                if (left->ttype == Type::DOUBLE || right->ttype == Type::DOUBLE)
                    resultType = doubleType;
                // Si alguno es float, resultado float
                else if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
                    resultType = floatType;
                else
                    resultType = intType; // char promueve a int
            } else {
                error("operación aritmética requiere int, char, float, double o aritmética de punteros.");
                resultType = intType;
            }
            break;

        // Comparaciones: operandos aritméticos o punteros, resultado bool
        case BinaryOp::EQ: case BinaryOp::NE:
        case BinaryOp::LT: case BinaryOp::GT:
        case BinaryOp::LE: case BinaryOp::GE:
            if (is_arithmetic_type(left) && is_arithmetic_type(right)) {
                resultType = intType;
            } else if (left->ttype == Type::POINTER && right->ttype == Type::POINTER) {
                resultType = intType;
            } else if ((left->ttype == Type::POINTER && is_integral_type(right)) ||
                       (is_integral_type(left) && right->ttype == Type::POINTER)) {
                // pointer == 0 / pointer != 0 (NULL checks)
                if (e->op == BinaryOp::EQ || e->op == BinaryOp::NE) {
                    resultType = intType;
                } else {
                    error("comparación aritmética entre puntero y entero.");
                    resultType = intType;
                }
            } else {
                error("comparación requiere tipo aritmético.");
                resultType = intType;
            }
            break;

        // Lógicos: operandos bool, resultado bool
        case BinaryOp::LOG_AND: case BinaryOp::LOG_OR:
            if (left->ttype == Type::VOID || right->ttype == Type::VOID) {
                error("operación lógica no puede ser void.");
            }
            resultType = intType;
            break;

        default:
            resultType = intType;
            break;
    }

    e->resolvedType = resultType;
}

// -----------------------------------------------------------
// visit(UnaryOpNode) — typecheck de operaciones unarias
// -----------------------------------------------------------
//   -MINUS: operando aritmético, resultado int (si char) o el mismo
//   PRE_INC/PRE_DEC/POST_INC/POST_DEC: operando aritmético, mismo tipo
//   LOG_NOT: operando bool, resultado bool
//   ADDR (&): devuelve PointerType(base)
//   DEREF (*): devuelve tipo base del puntero
//
//   Ej: -5 → int
//       &x → int* (PointerType(IntType))
//       *p → int (base del puntero)
//       !true → bool
void TypeChecker::visit(UnaryOpNode* e) {
    e->operand->accept(this); 
    Type* t = e->operand->resolvedType;
    Type* resultType;
    switch (e->op) {
        case UnaryOp::MINUS:
            if (is_arithmetic_type(t)) {
                // -'a' o -x con char/int → resultado int (promoción); -3.14 → double.
                resultType = is_integral_type(t) ? intType : t;
            } else {
                error("operación unaria requiere int, char, float o double.");
                resultType = t;
            }
            break;
        case UnaryOp::PRE_INC: case UnaryOp::PRE_DEC:
        case UnaryOp::POST_INC: case UnaryOp::POST_DEC:
            if (is_arithmetic_type(t)) {
                // ++/-- conservan el tipo del operando (char sigue siendo char).
                resultType = t;
            } else {
                error("operación unaria requiere int, char, float o double.");
                resultType = t;
            }
            break;
        case UnaryOp::LOG_NOT:
            if (t->ttype == Type::VOID) {
                error("! no puede aplicarse a void.");
            }
            resultType = intType;
            break;
        case UnaryOp::ADDR:
            // &x devuelve puntero al tipo de x
            // Ej: int x → &x es PointerType(IntType)
            {
                PointerType* ptr = new PointerType(t);
                typeCache.push_back(ptr);
                resultType = ptr;
            }
            break;
        case UnaryOp::DEREF:
            // *p devuelve el tipo base del puntero
            // Ej: int* p → *p es IntType
            if (t->ttype == Type::POINTER) {
                PointerType* pt = (PointerType*)t;
                resultType = pt->base;
            } else {
                error("operando de * debe ser puntero.");
                resultType = intType;
            }
            break;
        default:
            resultType = intType;
            break;
    }

    e->resolvedType = resultType;
}

// -----------------------------------------------------------
// visit(AssignmentNode) — typecheck de asignación
// -----------------------------------------------------------
// Verifica que el tipo del valor sea asignable al tipo del
// destino (usando check_assign). Retorna el tipo del destino.
//
//   Ej: x = 5 → verifica int ← int ok
//       x = 3.5 → verifica int ← float → error
//       f = 3.5 → verifica float ← float ok
void TypeChecker::visit(AssignmentNode* e) {
    // El target debe ser l-value con resolvedType (Id, Index, Member, etc.).
    isLvalContext = true;
    e->target->accept(this); Type* targetType = e->target->resolvedType;
    isLvalContext = false;

    if (targetType->isConst) {
        error("no se puede asignar a una variable declarada como const.");
    }

    e->value->accept(this); Type* valueType = e->value->resolvedType;
    if (!check_assign(targetType, valueType)) {
        error("tipos incompatibles en asignación (se esperaba " +
              targetType->str() + ").");
    }
    e->resolvedType = targetType;
    if (auto* id = dynamic_cast<IdNode*>(e->target)) {
        initialized_vars.insert(id->binding);
    }
}

// -----------------------------------------------------------
// visit(MallocNode) — typecheck de malloc
// -----------------------------------------------------------
// malloc siempre retorna void*.
//
//   Ej: int* p = malloc(8); → p es int*, malloc retorna void*
void TypeChecker::visit(MallocNode* e) {
    e->size->accept(this);
    PointerType* ptr = new PointerType(voidType);
    typeCache.push_back(ptr);
    e->resolvedType = ptr;
}

// -----------------------------------------------------------
// visit(PrintfNode) — typecheck de printf
// -----------------------------------------------------------
// Procesa todos los argumentos, retorna void.
void TypeChecker::visit(PrintfNode* e) {
    for (auto a : e->args) a->accept(this);
    e->resolvedType = voidType;
}

// -----------------------------------------------------------
// checkFuncCall — verificar argumentos de llamada a función
// -----------------------------------------------------------
// Helper que verifica:
//   - Cantidad correcta de argumentos
//   - Cada argumento es asignable al tipo del parámetro
//
// Retorna false si hay error de cantidad.
//
//   Ej: f(int a, float b) → f(1, 2.0) ok; f(1) error (cantidad)
bool TypeChecker::checkFuncCall(const string& fname, FuncInfo& info, FcallNode* e) {
    if (e->args.size() != info.paramTypes.size()) {
        error("número de argumentos incorrecto en llamada a '" + fname +
              "' (esperaba " + to_string(info.paramTypes.size()) +
              ", recibió " + to_string(e->args.size()) + ").");
        return false;
    }
    for (size_t i = 0; i < e->args.size(); i++) {
        e->args[i]->accept(this); Type* argType = e->args[i]->resolvedType;
        if (!check_assign(info.paramTypes[i], argType)) {
            error("tipo de argumento " + to_string(i+1) +
                  " incorrecto en llamada a '" + fname + "'.");
        }
    }
    return true;
}

// -----------------------------------------------------------
// visit(FcallNode) — typecheck de llamada a función
// -----------------------------------------------------------
// Maneja tres casos:
// Verifica cantidad y tipos de argumentos contra la firma registrada.
//
//   Ej: suma(1, 2) → busca función "suma", verifica argumentos
void TypeChecker::visit(FcallNode* e) {
    if (auto* id = dynamic_cast<IdNode*>(e->callee)) {
        string fname = id->name;

        // --- Llamada a función normal ---
        auto it = functions.find(fname);
        if (it == functions.end()) {
            error("función no declarada '" + fname + "'.");
            e->resolvedType = intType; return;
        }
        FuncInfo& info = it->second;
        checkFuncCall(fname, info, e);
        e->resolvedType = info.returnType; return;
    }
    error("llamada a función no reconocida.");
    e->resolvedType = intType;
}

// -----------------------------------------------------------
// visit(IndexNode) — typecheck de acceso por índice
// -----------------------------------------------------------
// Verifica que el índice sea int/char y que el operando base
// sea arreglo o puntero. Retorna el tipo base.
//
//   Ej: a[5] donde a es int[10] → retorna int
//       m[1][2] donde m es int[3][4] → retorna int
//       p[3] donde p es int* → retorna int (*(p+3))
//       a["hola"] → error (índice no int)
void TypeChecker::visit(IndexNode* e) {
    e->base->accept(this); Type* base = e->base->resolvedType;
    e->index->accept(this); Type* index = e->index->resolvedType;
    // El índice debe ser int o char
    if (!is_switch_index_type(index)) {
        error("índice de arreglo debe ser int o char.");
    }
    // Acceso a arreglo: devuelve tipo base
    // Ej: a[5] en int a[10] → int
    if (base->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)base;
        e->resolvedType = at->base;
        return;
    }
    // Acceso a puntero: devuelve tipo base (equivalente a *(p + i))
    if (base->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)base;
        e->resolvedType = pt->base;
        return;
    }
    error("acceso por índice requiere arreglo o puntero.");
    e->resolvedType = intType;
}

// -----------------------------------------------------------
// visit(MemberAccessNode) — typecheck de acceso a miembro (.)
// -----------------------------------------------------------
// Verifica que el objeto sea struct y que el miembro exista.
// Retorna el tipo del miembro.
//
//   Ej: p.x donde p es struct Punto { int x; float y; } → int
//       p.z → error (no existe miembro z)
void TypeChecker::visit(MemberAccessNode* e) {
    e->object->accept(this); Type* objType = e->object->resolvedType;
    if (objType->ttype != Type::STRUCT) {
        error("acceso a miembro '.' sobre tipo no struct (es " +
              objType->str() + ").");
        e->resolvedType = intType;
        return;
    }
    StructType* st = (StructType*)objType;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" +
              e->member + "'.");
        e->resolvedType = intType;
        return;
    }
    e->resolvedType = it->second;
}

// -----------------------------------------------------------
// visit(ArrowAccessNode) — typecheck de acceso por flecha (->)
// -----------------------------------------------------------
// Verifica que el operando sea puntero a struct y que el
// miembro exista. Retorna el tipo del miembro.
//
//   Ej: ptr->x donde ptr es struct Punto* → int
void TypeChecker::visit(ArrowAccessNode* e) {
    e->pointer->accept(this); Type* ptrType = e->pointer->resolvedType;
    if (ptrType->ttype != Type::POINTER) {
        error("acceso '->' requiere puntero (es " + ptrType->str() + ").");
        e->resolvedType = intType;
        return;
    }
    PointerType* pt = (PointerType*)ptrType;
    if (pt->base->ttype != Type::STRUCT) {
        error("acceso '->' requiere puntero a struct.");
        e->resolvedType = intType;
        return;
    }
    StructType* st = (StructType*)pt->base;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" +
              e->member + "'.");
        e->resolvedType = intType;
        return;
    }
    e->resolvedType = it->second;
}

// -----------------------------------------------------------
// visit(IdNode) — typecheck de identificador
// -----------------------------------------------------------
// Busca la variable en el environment. Si existe, asigna
// el binding (VarDecl*) al nodo y retorna su tipo.
// Esto es crucial porque GenCodeVisitor usa el binding
// para conocer el offset de la variable en el stack.
//
//   Ej: x → busca "x" en varEnv, binding = &VarDecl{x, ...}
//       resolvedType = IntType
void TypeChecker::visit(IdNode* e) {
    VarDecl* vd = nullptr;
    if (!varEnv.lookup(e->name, vd)) {
        error("variable '" + e->name + "' no declarada.");
        e->resolvedType = intType;
        return;
    }
    e->binding = vd;
    
    // Array-to-pointer decay: en contexto rvalue, el identificador de un arreglo
    // se convierte en puntero a su primer elemento.
    //   Ej: int a[4] → a (en rvalue) es int*
    if (!isLvalContext && vd->resolvedType->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)vd->resolvedType;
        PointerType* ptrType = new PointerType(at->base);
        typeCache.push_back(ptrType);
        e->resolvedType = ptrType;
    } else {
        e->resolvedType = vd->resolvedType;
    }
    
    if (!isLvalContext && functionDepth > 0 && initialized_vars.find(vd) == initialized_vars.end()) {
        error("variable '" + e->name + "' usada sin inicializar.", e->loc);
    }
}

// -----------------------------------------------------------
// Visits de literales — cada literal fija resolvedType en el nodo
// -----------------------------------------------------------
void TypeChecker::visit(NumberLiteralNode* e) {
    e->resolvedType = intType;
}
void TypeChecker::visit(FloatLiteralNode* e) {
    e->resolvedType = doubleType;
}
void TypeChecker::visit(BoolLiteralNode* e) {
    e->resolvedType = boolType;
}
void TypeChecker::visit(CharLiteralNode* e) {
    e->resolvedType = charType;  // en expresiones char promueve vía reglas aritméticas
}
void TypeChecker::visit(StringLiteralNode* e) {
    // Tratado como puntero a char (char*) para permitir asignación a char*
    PointerType* pt = new PointerType(charType);
    typeCache.push_back(pt);
    e->resolvedType = pt;
}

// Nodos de tipo en expresiones/contextos: delegan la resolución a type_from_ast.
void TypeChecker::visit(PrimitiveTypeNode*) { }
void TypeChecker::visit(PointerTypeNode*) { }
void TypeChecker::visit(StructTypeNode*) { }
// NamedTypeNode fuera de template → error dentro de type_from_ast.
// -----------------------------------------------------------
// visit(SizeOfNode) — typecheck de sizeof
// -----------------------------------------------------------
// Retorna el tamaño en bytes del tipo o de la expresión.
//   Ej: sizeof(int) -> 4
//       sizeof(x) -> tamaño de x
void TypeChecker::visit(SizeOfNode* e) {
    Type* t;
    if (e->kind == SizeOfNode::TYPE_ARG) {
        t = type_from_ast(e->type_arg);
    } else {
        e->expr_arg->accept(this);
        t = e->expr_arg->resolvedType;
    }
    e->resolvedType = intType;
    e->isConstant = true;
    e->constantValue = (double)t->size();
}


