#include "visitor.h"
#include <iostream>
#include <sstream>
#include <functional>

using namespace std;

// ============================================================
//  Funciones auxiliares de verificación de tipos (anónimas)
// ============================================================

namespace {

// is_integral_type: true si el tipo es entero (int o char)
//   Sirve para operaciones que solo aceptan enteros
//   y para promociones donde char → int automáticamente.
//
//   Ej: int → true    char → true
//       float → false double → false
//       bool → false   void → false
bool is_integral_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR;
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
    return t->ttype == Type::INT || t->ttype == Type::CHAR;
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
    retornodefuncion = nullptr;          // se setea al entrar a cada función
    loopDepth = 0;                       // sin loops activos
    switchDepth = 0;                     // sin switches activos
    hasError = false;
    currentOffset = 0;
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
//   Nombre<T1, T2> → TemplateTypeNode → instantiate_template()
//   typename T (NamedTypeNode) → error (debe ser resuelto por template)
//
//   Ej: type_from_ast(PrimitiveTypeNode(INT)) → intType
//       type_from_ast(PointerTypeNode(IntTypeNode)) → PointerType(IntType)
Type* TypeChecker::type_from_ast(Exp* t) {
    // --- Tipos primitivos: void, int, char, float, double, bool, auto ---
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(t)) {
        switch (pt->prim) {
            case PrimitiveTypeNode::VOID:   return voidType;
            case PrimitiveTypeNode::INT:    return intType;
            case PrimitiveTypeNode::CHAR:   return charType;
            case PrimitiveTypeNode::FLOAT:  return floatType;
            case PrimitiveTypeNode::DOUBLE: return doubleType;
            case PrimitiveTypeNode::BOOL:   return boolType;
            case PrimitiveTypeNode::AUTO:   return intType; // default, se infiere después
        }
    }
    // --- Punteros: T* busca el tipo base recursivamente ---
    if (auto* pt = dynamic_cast<PointerTypeNode*>(t)) {
        Type* base = type_from_ast(pt->base);
        PointerType* ptr = new PointerType(base);
        typeCache.push_back(ptr);
        return ptr;
    }
    // --- Structs: busca el StructType por nombre ---
    if (auto* st = dynamic_cast<StructTypeNode*>(t)) {
        auto it = struct_types.find(st->name);
        if (it != struct_types.end()) return it->second;
        error("struct '" + st->name + "' no declarado.");
        return intType;
    }
    // --- NamedType (typename param de template) ---
    if (auto* nt = dynamic_cast<NamedTypeNode*>(t)) {
        error("tipo no reconocido: '" + nt->name + "'.");
        return intType;
    }
    // --- TemplateType (instancia concreta de template struct) ---
    if (auto* tt = dynamic_cast<TemplateTypeNode*>(t)) {
        return instantiate_template(tt->name, tt->type_args);
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
// semantic_to_type_node — convertir Type* semántico a TypeNode
// ============================================================
// Hace la operación inversa de type_from_ast: dado un Type*,
// crea el nodo AST correspondiente. Se usa durante la
// instanciación de templates para sustituir tipos.
//
//   Ej: Type*(INT) → PrimitiveTypeNode(INT)
//       Type*(FLOAT) → PrimitiveTypeNode(FLOAT)
TypeNode* TypeChecker::semantic_to_type_node(::Type* t) {
    if (!t) return new PrimitiveTypeNode(PrimitiveTypeNode::INT);
    switch (t->ttype) {
        case Type::INT: return new PrimitiveTypeNode(PrimitiveTypeNode::INT);
        case Type::FLOAT: return new PrimitiveTypeNode(PrimitiveTypeNode::FLOAT);
        case Type::DOUBLE: return new PrimitiveTypeNode(PrimitiveTypeNode::DOUBLE);
        case Type::CHAR: return new PrimitiveTypeNode(PrimitiveTypeNode::CHAR);
        case Type::BOOL: return new PrimitiveTypeNode(PrimitiveTypeNode::BOOL);
        default: return new PrimitiveTypeNode(PrimitiveTypeNode::INT);
    }
}

// ============================================================
// mangleTemplateName — generar nombre único para instancia de template
// ============================================================
// Convierte un nombre de template con sus argumentos en un
// string único. Ejemplo:
//
//   mangleTemplateName("Vector", {IntTypeNode}) → "Vector<int>"
//   mangleTemplateName("Par", {IntTypeNode, FloatTypeNode}) → "Par<int,float>"
//
// Se usa como key para cachear instancias de templates y como
// nombre concreto del struct/función generado.
static string mangleTemplateName(const string& base, const vector<TypeNode*>& args) {
    string result = base + "<";
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) result += ",";
        if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(args[i])) {
            switch (pt->prim) {
                case PrimitiveTypeNode::INT: result += "int"; break;
                case PrimitiveTypeNode::FLOAT: result += "float"; break;
                case PrimitiveTypeNode::CHAR: result += "char"; break;
                case PrimitiveTypeNode::DOUBLE: result += "double"; break;
                case PrimitiveTypeNode::BOOL: result += "bool"; break;
                default: result += "?"; break;
            }
        } else if (auto* st = dynamic_cast<StructTypeNode*>(args[i])) {
            result += st->name;
        } else {
            result += "?";
        }
    }
    result += ">";
    return result;
}

// ============================================================
// instantiate_function — crear instancia concreta de función template
// ============================================================
// Dado un TemplateDecl de función (ej: template<typename T> T id(T x))
// y una lista de argumentos concretos (ej: [int]), genera un FunDecl
// con los tipos sustituidos.
//
// Flujo:
//   1. Genera key con mangleTemplateName
//   2. Si ya fue instanciada, retorna la versión cacheada
//   3. Crea mapa de sustitución: T → int
//   4. Sustituye tipos en retorno y parámetros
//   5. Crea nuevo FunDecl, lo cachea y lo registra en program
//
//   Ej: template<typename T> T id(T x) → instantiate con int
//       → FunDecl{return:int, params:[int x]}
FunDecl* TypeChecker::instantiate_function(TemplateDecl* tdecl, const vector<TypeNode*>& args) {
    FunDecl* orig = tdecl->func;
    string key = mangleTemplateName(orig->name, args);

    // Cache: si ya instanciamos esta combinación, reusar
    auto cached = instantiated_function_cache.find(key);
    if (cached != instantiated_function_cache.end())
        return cached->second;

    // Construir mapa de sustitución: T → tipo_concreto
    unordered_map<string, Type*> subs;
    for (size_t i = 0; i < tdecl->params.size() && i < args.size(); i++)
        subs[tdecl->params[i]] = type_from_ast(args[i]);

    // Función lambda que sustituye tipos en el AST
    function<Exp*(Exp*)> subst_node = [&](Exp* node) -> Exp* {
        if (auto* nt = dynamic_cast<NamedTypeNode*>(node)) {
            auto it = subs.find(nt->name);
            if (it != subs.end()) return semantic_to_type_node(it->second);
        }
        if (auto* pt = dynamic_cast<PointerTypeNode*>(node)) {
            return new PointerTypeNode(dynamic_cast<TypeNode*>(subst_node(pt->base)));
        }
        return node;
    };

    // Crear nuevo FunDecl con tipos sustituidos
    FunDecl* fd = new FunDecl(subst_node(orig->return_type), orig->name, orig->body);
    for (auto p : orig->params)
        fd->params.push_back(new VarDecl(subst_node(p->type), p->name));

    instantiated_function_cache[key] = fd;
    if (program) program->instantiated_functions.push_back(fd);
    return fd;
}

// ============================================================
// instantiate_template — instanciar struct template
// ============================================================
// Similar a instantiate_function pero para structs.
// Crea un StructType concreto reemplazando los parámetros
// de template por los tipos dados.
//
//   Ej: template<typename T> struct Par { T x; T y; };
//       instantiate_template("Par", {int})
//       → StructType("Par<int>") con miembros {x: int, y: int}
//
// Flujo:
//   1. Genera nombre concreto: "Par<int>"
//   2. Si ya existe en struct_types, retornar
//   3. Busca TemplateDecl por nombre
//   4. Sustituye tipos en cada miembro
//   5. Crea StructType y lo registra
StructType* TypeChecker::instantiate_template(const string& name, const vector<TypeNode*>& args) {
    string concrete_name = mangleTemplateName(name, args);

    // Cache
    auto cit = struct_types.find(concrete_name);
    if (cit != struct_types.end()) return cit->second;

    // Buscar declaración del template
    auto it = template_decls.find(name);
    if (it == template_decls.end() || it->second->is_function) {
        error("template '" + name + "' no declarado.");
        StructType* err = new StructType("error");
        typeCache.push_back(err);
        return err;
    }

    TemplateDecl* tdecl = it->second;
    StructDecl* orig = tdecl->struct_decl;

    // Mapa de sustitución: T → tipo
    unordered_map<string, Type*> subs;
    for (size_t i = 0; i < tdecl->params.size() && i < args.size(); i++) {
        subs[tdecl->params[i]] = type_from_ast(args[i]);
    }

    // Función para sustituir tipos recursivamente
    function<Type*(Exp*)> substitute = [&](Exp* ast_type) -> Type* {
        if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(ast_type)) {
            return type_from_ast(pt);
        }
        if (auto* idt = dynamic_cast<NamedTypeNode*>(ast_type)) {
            auto sit = subs.find(idt->name);
            if (sit != subs.end()) return sit->second;
            error("tipo de template '" + idt->name + "' no reconocido.");
            return intType;
        }
        if (auto* ptr = dynamic_cast<PointerTypeNode*>(ast_type)) {
            Type* base = substitute(ptr->base);
            PointerType* result = new PointerType(base);
            typeCache.push_back(result);
            return result;
        }
        return type_from_ast(ast_type);
    };

    // Crear StructType concreto
    StructType* st = new StructType(concrete_name);
    for (auto member : orig->members) {
        Type* mt = substitute(member->type);
        // Wrap en ArrayType si tiene dimensiones
        for (size_t d = 0; d < member->array_sizes.size(); d++) {
            ArrayType* at = new ArrayType(mt, -1);
            typeCache.push_back(at);
            mt = at;
        }
        st->members[member->name] = mt;
    }
    struct_types[concrete_name] = st;
    return st;
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
    if (target->match(value)) return true;
    if (target->ttype == Type::POINTER && value->ttype == Type::POINTER) return true;
    // Truncamiento / promoción entre char e int
    if (target->ttype == Type::CHAR && value->ttype == Type::INT) return true;
    if (target->ttype == Type::INT && value->ttype == Type::CHAR) return true;
    // Promoción: int/char → float o double
    if (target->ttype == Type::FLOAT && is_integral_type(value)) return true;
    if (target->ttype == Type::DOUBLE && is_integral_type(value)) return true;
    // Promoción: float → double
    if (target->ttype == Type::DOUBLE && value->ttype == Type::FLOAT) return true;
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
//
//   Ej: TypeChecker tc; tc.typecheck(program);
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
// check — entry point alternativo (retorna bool sin salir)
// ============================================================
// Similar a typecheck pero no llama a exit(). Retorna true si
// no hubo errores, false en caso contrario. Usado para pruebas.
bool TypeChecker::check(Program* program) {
    hasError = false;
    errors.clear();
    if (program) program->accept(this);
    if (hasError) {
        for (auto& e : errors) cerr << "Error semántico: " << e << endl;
    }
    return !hasError;
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
//   5. Procesar templates
//   6. Procesar funciones (typecheck interno)
//   7. Procesar funciones instanciadas de templates
//   8. Cerrar scope global
void TypeChecker::visit(Program* p) {
    program = p;
    for (auto f : p->functions) add_function(f);
    env.add_level();
    varEnv.add_level();
    for (auto g : p->globals) g->accept(this);
    for (auto s : p->structs) s->accept(this);
    for (auto t : p->templates) t->accept(this);
    for (auto f : p->functions) f->accept(this);
    for (auto f : p->instantiated_functions) f->accept(this);
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
            // Wrap en ArrayType si tiene dimensiones de arreglo
            // Ej: int a[5][3] → ArrayType(ArrayType(IntType, 3), 5)
            for (auto s : v->array_sizes) {
                int dim = -1;
                if (auto* il = dynamic_cast<IntegerLiteralNode*>(s))
                    dim = (int)il->value;
                ArrayType* at = new ArrayType(t, dim);
                typeCache.push_back(at);
                t = at;
            }
            // Inferencia auto: usar placeholder, se resolverá después
            if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(v->type)) {
                if (pt->prim == PrimitiveTypeNode::AUTO) {
                    if (!v->initializer) {
                        error("variable 'auto' necesita inicializador para inferir tipo.");
                        t = intType;
                    } else {
                        // Placeholder: será reemplazado en visit(VarDecl)
                        PointerType* placeholder = new PointerType(voidType);
                        typeCache.push_back(placeholder);
                        t = placeholder;
                    }
                }
            }
            v->resolvedType = t;
            v->memSize = t->size();
        }
    }

    // Asignar offsets a variables locales (después de parámetros)
    int totalSize = assignOffsets(localVars, paramSize);

    // Alinear a 16 bytes (convención System V)
    f->frameSize = (totalSize + 15) & ~15;

    // Convertir offsets a negativos (en x86-64 el stack crece hacia abajo)
    // Los offsets positivos son relativos a %rbp, los negativos son (%rbp - offset)
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

    // ---- Verificar que funciones no-void retornen en todos los caminos ----
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
// assignOffsets — asignar offsets en el stack frame
// -----------------------------------------------------------
// Asigna un slot de 8 bytes a cada variable <= 8 bytes, y el
// tamaño real a variables grandes (arrays, structs > 8 bytes).
// Esto evita la complejidad del bin packing sin penalización
// práctica en x86-64.
//
// Retorna el offset final (startOffset + bytes ocupados).
int TypeChecker::assignOffsets(vector<VarDecl*>& vars, int startOffset) {
    int nextOffset = startOffset;
    for (auto v : vars) {
        int slotSize = v->memSize > 8 ? v->memSize : 8;
        v->offset = nextOffset;
        nextOffset += slotSize;
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
//       auto y = x; → infiere y como int
//       int a[5];   → resolvedType = ArrayType(IntType, 5)
void TypeChecker::visit(VarDecl* v) {
    if (v->resolvedType != nullptr) {
        // Variable ya procesada en FunDecl (resolvedType pre-asignado)
        if (env.check_current(v->name)) {
            error("variable '" + v->name + "' ya declarada en este ámbito.");
            return;
        }
        bool reinferred = false;
        // Si es auto, inferir tipo del inicializador ahora
        if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(v->type)) {
            if (pt->prim == PrimitiveTypeNode::AUTO && v->initializer) {
                v->initializer->accept(this); v->resolvedType = v->initializer->resolvedType;
                v->memSize = v->resolvedType->size();
                reinferred = true;
            }
        }
        // Verificar compatibilidad del inicializador con el tipo declarado
        if (!reinferred && v->initializer) {
            v->initializer->accept(this); Type* initType = v->initializer->resolvedType;
            if (!check_assign(v->resolvedType, initType)) {
                error("tipos incompatibles en inicialización de '" + v->name + "'.");
            }
        }
        bind_var_decl(v);
        return;
    }

    // Variable no procesada (global o local sin pre-asignación)
    Type* t = type_from_ast(v->type);

    if (t->match(voidType)) {
        error("no se puede declarar variable de tipo void.");
        return;
    }

    // Wrap en ArrayType si tiene dimensiones
    for (auto s : v->array_sizes) {
        int dim = -1;
        if (auto* il = dynamic_cast<IntegerLiteralNode*>(s))
            dim = (int)il->value;
        ArrayType* at = new ArrayType(t, dim);
        typeCache.push_back(at);
        t = at;
    }

    // Inferencia auto: usar tipo del inicializador
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(v->type)) {
        if (pt->prim == PrimitiveTypeNode::AUTO) {
            if (!v->initializer) {
                error("variable 'auto' necesita inicializador para inferir tipo.");
                t = intType;
            } else {
                v->initializer->accept(this); t = v->initializer->resolvedType;
            }
        }
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
    int offset = 0;

    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        st->members[m->name] = mt;
        s->memberOffsets[m->name] = offset;
        s->memberSizes[m->name] = mt->size();
        offset += mt->size();
    }

    s->totalSize = offset;
    struct_types[s->name] = st;
}

// -----------------------------------------------------------
// visit(Body) — typecheck de bloque { ... }
// -----------------------------------------------------------
// Abre un nuevo ámbito (scope) para las variables declaradas
// dentro del bloque.
void TypeChecker::visit(Body* b) {
    env.add_level();
    varEnv.add_level();
    for (auto s : b->stmts) s->accept(this);
    varEnv.remove_level();
    env.remove_level();
}

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
    if (!t->match(boolType)) {
        error("condición de if debe ser bool.");
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
    if (!t->match(boolType)) {
        error("condición de while debe ser bool.");
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
    if (!t->match(boolType)) {
        error("condición de do-while debe ser bool.");
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
        if (!t->match(boolType)) {
            error("condición de for debe ser bool.");
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

void TypeChecker::visit(CaseClause* s) {
    s->value->accept(this); Type* t = s->value->resolvedType;
    if (!is_switch_index_type(t)) {
        error("valor de case debe ser int o char.");
    }
    for (auto st : s->body) st->accept(this);
}

// -----------------------------------------------------------
// visit(TemplateDecl) — registrar template
// -----------------------------------------------------------
// Almacena la declaración en template_decls para instanciación
// posterior. Puede ser template de función o de struct.
void TypeChecker::visit(TemplateDecl* d) {
    if (d->is_function) {
        template_decls[d->func->name] = d;
    } else {
        template_decls[d->struct_decl->name] = d;
    }
}

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
            if (is_arithmetic_type(left) && is_arithmetic_type(right)) {
                // Promoción automática: si alguno es double, resultado double
                if (left->ttype == Type::DOUBLE || right->ttype == Type::DOUBLE)
                    resultType = doubleType;
                // Si alguno es float, resultado float
                else if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
                    resultType = floatType;
                else
                    resultType = intType; // char promueve a int
            } else {
                error("operación aritmética requiere int, char, float o double.");
                resultType = intType;
            }
            break;

        // Comparaciones: operandos aritméticos, resultado bool
        case BinaryOp::EQ: case BinaryOp::NE:
        case BinaryOp::LT: case BinaryOp::GT:
        case BinaryOp::LE: case BinaryOp::GE:
            if (is_arithmetic_type(left) && is_arithmetic_type(right))
                resultType = boolType;
            else {
                error("comparación requiere int, char, float o double.");
                resultType = boolType;
            }
            break;

        // Lógicos: operandos bool, resultado bool
        case BinaryOp::LOG_AND: case BinaryOp::LOG_OR:
            if (left->match(boolType) && right->match(boolType))
                resultType = boolType;
            else {
                error("operación lógica requiere bool.");
                resultType = boolType;
            }
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
    e->operand->accept(this); Type* t = e->operand->resolvedType;
    Type* resultType;
    switch (e->op) {
        case UnaryOp::MINUS:
            if (is_arithmetic_type(t)) {
                resultType = is_integral_type(t) ? intType : t;
            } else {
                error("operación unaria requiere int, char, float o double.");
                resultType = t;
            }
            break;
        case UnaryOp::PRE_INC: case UnaryOp::PRE_DEC:
        case UnaryOp::POST_INC: case UnaryOp::POST_DEC:
            if (is_arithmetic_type(t)) {
                resultType = t;
            } else {
                error("operación unaria requiere int, char, float o double.");
                resultType = t;
            }
            break;
        case UnaryOp::LOG_NOT:
            if (t->match(boolType)) {
                resultType = boolType;
            } else {
                error("! requiere bool.");
                resultType = boolType;
            }
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
    e->target->accept(this); Type* targetType = e->target->resolvedType;
    e->value->accept(this); Type* valueType = e->value->resolvedType;
    if (!check_assign(targetType, valueType)) {
        error("tipos incompatibles en asignación (se esperaba " +
              targetType->str() + ").");
    }
    e->resolvedType = targetType;
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
//   1. Llamada a función template: f<T>(args) → instancia y verifica
//   2. Llamada a variable (lambda/ptr): f(args) → acepta args, retorna int
//   3. Llamada a función normal: f(args) → busca firma y verifica
//
//   Ej: suma(1, 2) → busca función "suma", verifica argumentos
//       Vector<int>::crear(10) → instancia template, verifica
//       ptr_lambda(5) → trata como llamada indirecta
void TypeChecker::visit(FcallNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->callee)) {
        string fname = id->name;

        // --- Caso 1: Llamada a función template ---
        if (!e->template_args.empty()) {
            auto tit = template_decls.find(fname);
            if (tit == template_decls.end() || !tit->second->is_function) {
                error("template de función '" + fname + "' no declarado.");
                e->resolvedType = intType; return;
            }
            TemplateDecl* tdecl = tit->second;
            instantiate_function(tdecl, e->template_args);
            FunDecl* tfunc = tdecl->func;
            // Construir mapa de sustitución
            unordered_map<string, Type*> subs;
            for (size_t i = 0; i < tdecl->params.size() && i < e->template_args.size(); i++)
                subs[tdecl->params[i]] = type_from_ast(e->template_args[i]);

            // Resolver tipo de retorno concreto
            Type* concrete_ret;
            if (auto* nt = dynamic_cast<NamedTypeNode*>(tfunc->return_type)) {
                auto sit = subs.find(nt->name);
                concrete_ret = (sit != subs.end()) ? sit->second : intType;
            } else {
                concrete_ret = type_from_ast(tfunc->return_type);
            }
            // Resolver tipos de parámetros concretos
            vector<Type*> concrete_params;
            for (auto p : tfunc->params) {
                Type* pt;
                if (auto* nt = dynamic_cast<NamedTypeNode*>(p->type)) {
                    auto sit = subs.find(nt->name);
                    pt = (sit != subs.end()) ? sit->second : intType;
                } else {
                    pt = type_from_ast(p->type);
                }
                concrete_params.push_back(pt);
            }
            // Registrar firma concreta y verificar llamada
            FuncInfo info;
            info.returnType = concrete_ret;
            info.paramTypes = concrete_params;
            functions[fname] = info;

            checkFuncCall(fname, info, e);
            e->resolvedType = info.returnType; return;
        }

        // --- Caso 2: Llamada a variable (lambda o puntero a función) ---
        Type* localType = env.lookup(fname);
        if (localType) {
            // Acepta argumentos pero no verifica tipos (no tenemos firma)
            for (auto* arg : e->args) arg->accept(this);
            e->resolvedType = intType; return;
        }

        // --- Caso 3: Llamada a función normal ---
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
// visit(IdentifierNode) — typecheck de identificador
// -----------------------------------------------------------
// Busca la variable en el environment. Si existe, asigna
// el binding (VarDecl*) al nodo y retorna su tipo.
// Esto es crucial porque GenCodeVisitor usa el binding
// para conocer el offset de la variable en el stack.
//
//   Ej: x → busca "x" en varEnv, binding = &VarDecl{x, ...}
//       resolvedType = IntType
void TypeChecker::visit(IdentifierNode* e) {
    VarDecl* vd = nullptr;
    if (!varEnv.lookup(e->name, vd)) {
        error("variable '" + e->name + "' no declarada.");
        e->resolvedType = intType;
        return;
    }
    e->binding = vd;
    e->resolvedType = vd->resolvedType;
}

// -----------------------------------------------------------
// Visits de literales — cada literal tiene un tipo fijo
// -----------------------------------------------------------
void TypeChecker::visit(IntegerLiteralNode* e) {
    e->resolvedType = intType;
}
void TypeChecker::visit(FloatLiteralNode* e) {
    e->resolvedType = floatType;
}
void TypeChecker::visit(BoolLiteralNode* e) {
    e->resolvedType = boolType;
}
void TypeChecker::visit(CharLiteralNode* e) {
    e->resolvedType = charType;
}
void TypeChecker::visit(StringLiteralNode* e) {
    e->resolvedType = intType;
}

// -----------------------------------------------------------
// Visits de nodos de tipo — delegan a type_from_ast
// -----------------------------------------------------------
void TypeChecker::visit(PrimitiveTypeNode* e) { e->resolvedType = type_from_ast(e); }
void TypeChecker::visit(PointerTypeNode* e) { e->resolvedType = type_from_ast(e); }
void TypeChecker::visit(StructTypeNode* e) { e->resolvedType = type_from_ast(e); }
void TypeChecker::visit(NamedTypeNode* e) { e->resolvedType = type_from_ast(e); }

void TypeChecker::visit(TemplateTypeNode* e) { e->resolvedType = type_from_ast(e); }

// -----------------------------------------------------------
// visit(SizeOfNode) — typecheck de sizeof
// -----------------------------------------------------------
// sizeof siempre retorna int (el tamaño en bytes).
// El operando es un tipo, se verifica que exista.
//
//   Ej: sizeof(int) → 4
//       sizeof(char) → 1
//       sizeof(struct Persona) → tamaño de la struct
void TypeChecker::visit(SizeOfNode* e) {
    e->target_type->accept(this);
    e->resolvedType = intType;
}

// -----------------------------------------------------------
// visit(LambdaExprNode) — typecheck de expresión lambda
// -----------------------------------------------------------
// Procesa una expresión lambda:
//   1. Abre nuevo ámbito para parámetros y capturas
//   2. Resuelve tipos de parámetros
//   3. Agrega capturas por valor al environment
//   4. Typecheckea el cuerpo con el tipo de retorno de la lambda
//   5. Retorna void* (tipo opaco, la lambda es un puntero a función)
//
//   Ej: [x](int a) -> int { return a + x; }
//       → parámetro a: int
//       → captura x: se agrega al ámbito interno
//       → retorno: int
//       → tipo de la expresión: void*
void TypeChecker::visit(LambdaExprNode* e) {
    env.add_level();
    varEnv.add_level();
    for (auto p : e->params) p->accept(this);
    for (auto cap : e->captures) {
        VarDecl* capVd = nullptr;
        if (!varEnv.lookup(cap->name, capVd)) {
            error("variable '" + cap->name + "' no declarada.");
        } else {
            env.add_var(cap->name, capVd->resolvedType);
            varEnv.add_var(cap->name, capVd);
        }
    }
    Type* savedRet = retornodefuncion;
    if (e->return_type)
        retornodefuncion = type_from_ast(e->return_type);
    else
        retornodefuncion = voidType;
    if (e->body) e->body->accept(this);
    retornodefuncion = savedRet;
    varEnv.remove_level();
    env.remove_level();

    // Retorna void* (puntero opaco)
    PointerType* lambdaPtr = new PointerType(voidType);
    typeCache.push_back(lambdaPtr);
    e->resolvedType = lambdaPtr;
}

// -----------------------------------------------------------
// visit(CaptureNode) — typecheck de captura en lambda
// -----------------------------------------------------------
// Cada variable capturada en [x, y] se verifica. El modo
// BY_REF (captura por referencia, [&x]) no está soportado.
//
//   Ej: [x] → ok, captura por valor
//       [&x] → error: "captura por referencia no soportada"
void TypeChecker::visit(CaptureNode* e) {
    if (e->mode == CaptureNode::BY_REF)
        error("captura por referencia no soportada, use captura por valor [x].");
    e->resolvedType = intType;
}
