#include "visitor.h"
#include <iostream>
#include <sstream>


using namespace std;

// ============================================================
// TypeChecker — Análisis semántico (verificación de tipos)
// ============================================================
// Recorre el AST usando Visitor, valida reglas del lenguaje y
// anota tipos, offsets y enlaces para la generación de código.
//
// Anotaciones principales en el AST:
//   - Exp::resolvedType   — tipo semántico de cada expresión
//   - VarDecl::offset, memSize, resolvedType — layout en stack
//   - IdNode::binding — enlace al VarDecl de la variable
//   - FunDecl::frameSize — tamaño total del stack frame
//   - StructDecl::memberOffsets, memberSizes, totalSize
//
// Tablas de estado (ver visitor.h): env / varEnv, functions,
// struct_types, template_decls, typeCache.
//
// Flujo en visit(Program):
//   1. Registrar firmas de funciones.
//   2. Procesar variables globales, structs y templates (registro).
//   3. Typecheck de cada función.
//   4. Typecheck de funciones instanciadas desde templates.
//
// Errores se acumulan en errors[]; typecheck() hace exit(1) al final.
// ============================================================

// ============================================================
// Helpers anónimos de verificación de tipos
// ============================================================

namespace {

// Indica si t es un tipo entero (int, char, long, bool).
// Se usa en operaciones enteras y promociones automáticas.
bool is_integral_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR || t->ttype == Type::LONG
           || t->ttype == Type::BOOL;
}

// Indica si t es un tipo aritmético (enteros + float + double).
// Se usa en operaciones binarias y unarias aritméticas.
bool is_arithmetic_type(Type* t) {
    return is_integral_type(t) ||
           t->ttype == Type::FLOAT ||
           t->ttype == Type::DOUBLE;
}

// Tipos permitidos como índice de switch y case: int, char, bool.
bool is_switch_index_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR || t->ttype == Type::BOOL;
}

} // namespace

// ============================================================
// Constructor / Destructor
// ============================================================
// Crea los tipos primitivos singleton reutilizados durante el
// typechecking. El destructor libera tipos dinámicos almacenados
// en typeCache y los tipos singleton.

TypeChecker::TypeChecker() {
    intType = new Type(Type::INT);       // int
    boolType = new Type(Type::BOOL);     // bool
    voidType = new Type(Type::VOID);     // void
    floatType = new Type(Type::FLOAT);   // float
    doubleType = new Type(Type::DOUBLE); // double
    charType = new Type(Type::CHAR);     // char
    longType = new Type(Type::LONG);     // long long
    uintType = new Type(Type::INT);      // unsigned int
    uintType->isUnsigned = true;
    ulongType = new Type(Type::LONG);    // unsigned long long
    ulongType->isUnsigned = true;
    retornodefuncion = nullptr;          // tipo de retorno de la función actual
    loopDepth = 0;                       // sin loops activos
    switchDepth = 0;                     // sin switches activos
    functionDepth = 0;
    hasError = false;
}

TypeChecker::~TypeChecker() {
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
// type_from_ast — convierte un TypeNode del AST a Type*
// ============================================================
// Resuelve tipos primitivos, punteros y structs. Los modificadores
// unsigned/const clonan el tipo para no alterar los singletons.

Type* TypeChecker::type_from_ast(TypeNode* t) {
    Type* res = nullptr;
    // Tipos primitivos.
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
            Type* clone = new Type(res->ttype);
            clone->isUnsigned = pt->isUnsigned;
            clone->isConst = pt->isConst;
            typeCache.push_back(clone);
            res = clone;
        }
        return res;
    }
    // Punteros: resuelve el tipo base recursivamente.
    if (auto* pt = dynamic_cast<PointerTypeNode*>(t)) {
        Type* base = type_from_ast(pt->base);
        PointerType* ptr = new PointerType(base);
        ptr->isConst = pt->isConst;
        typeCache.push_back(ptr);
        return ptr;
    }
    // Structs: busca el StructType por nombre.
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
// bind_var_decl — registra una variable en los environments
// ============================================================
// Agrega la variable al environment de tipos (env) y de
// declaraciones (varEnv) para referencias posteriores.

void TypeChecker::bind_var_decl(VarDecl* v) {
    env.add_var(v->name, v->resolvedType);
    varEnv.add_var(v->name, v);
}

// ============================================================
// check_assign — verifica compatibilidad en asignación
// ============================================================
// Determina si value puede asignarse a target. Permite tipos
// idénticos, punteros compatibles, y conversiones implícitas
// comunes de C (char↔int, enteros→float/double, etc.).

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
// error — registra un error semántico
// ============================================================
// Acumula errores en errors[] sin abortar. typecheck() los
// imprime al final y termina el proceso si hay alguno.

void TypeChecker::error(const string& msg) {
    errors.push_back(msg);
    hasError = true;
}

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
// add_function — registra la firma de una función
// ============================================================
// Almacena tipo de retorno y tipos de parámetros. Detecta
// redeclaraciones.

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
// typecheck — punto de entrada del análisis semántico
// ============================================================
// Inicia el recorrido del AST. Si hay errores, los imprime y
// termina con exit(1).

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
// Visits: declaraciones y statements
// ============================================================

// visit(Program) — punto de entrada del recorrido.
// Registra funciones, abre el scope global, procesa globales,
// structs, templates y funciones, y cierra el scope global.
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

// visit(FunDecl) — typecheck de una función.
// Flujo:
//   1. Abrir scope para parámetros y locales.
//   2. Resolver tipo de retorno.
//   3. Resolver tipos de parámetros y asignar offsets.
//   4. Recolectar variables locales del body.
//   5. Resolver tipos de locales (infiriendo auto si aplica).
//   6. Asignar offsets a locales.
//   7. Calcular frameSize alineado a 16 bytes.
//   8. Convertir offsets a negativos respecto a %rbp.
//   9. Vincular parámetros en los environments.
//  10. Typecheckear el body.
//  11. Verificar retorno en funciones no-void.
//  12. Cerrar scope.
void TypeChecker::visit(FunDecl* f) {
    env.add_level();
    varEnv.add_level();
    functionDepth++;

    retornodefuncion = type_from_ast(f->return_type);

    // Recolectar parámetros y resolver sus tipos.
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

    // Asignar offsets a parámetros (8 bytes por variable).
    int paramSize = assignOffsets(params, 0);

    // Recolectar todas las variables locales del body.
    vector<VarDecl*> localVars;
    collectVars(f->body, localVars);

    // Resolver tipos de locales (sin agregar al environment todavía).
    for (auto v : localVars) {
        if (v->resolvedType == nullptr) {
            Type* t = type_from_ast(v->type);
            if (t->match(voidType)) {
                error("no se puede declarar variable de tipo void.");
                t = intType;
            }
            // Wrap en ArrayType si tiene dimensiones (orden inverso).
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

    // Asignar offsets a variables locales (después de parámetros).
    int totalSize = assignOffsets(localVars, paramSize);

    // Alinear a 16 bytes (convención System V).
    f->frameSize = (totalSize + 15) & ~15;

    // Convertir offsets positivos a negativos respecto a %rbp.
    for (auto p : params) {
        p->offset = p->offset - f->frameSize;
    }
    for (auto v : localVars) {
        v->offset = v->offset - f->frameSize;
    }

    // Agregar parámetros al environment.
    for (auto p : params) {
        bind_var_decl(p);
    }

    // Typecheckear el body.
    f->body->accept(this);

    // Verificación superficial de retorno.
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

// collectVars — recolecta declaraciones de variables locales.
// Recorre recursivamente un statement para encontrar todas las
// VarDecl. No abre scopes: todas las locales comparten el mismo
// frame, como en C tradicional.
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

// assignOffsets — asigna offsets en el stack frame.
// Empaqueta variables en bloques de 8 bytes según tamaño:
//   >8B: tamaño real, 8B: 1 por bloque, 4B: 2 por bloque,
//   1B: 4 por bloque en slots de 2B.
// Ordena de más pesados a más ligeros para optimizar alineamiento.
// Retorna el offset final.
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

// visit(VarDecl) — typecheck de declaración de variable.
// Si ya fue procesada en FunDecl, solo verifica el inicializador
// y la bindea. Si no, resuelve el tipo, infiere auto y verifica
// el inicializador o lista de inicialización de arreglos.
void TypeChecker::visit(VarDecl* v) {
    if (v->resolvedType != nullptr) {
        // Variable ya procesada en FunDecl.
        if (env.check_current(v->name)) {
            error("variable '" + v->name + "' ya declarada en este ámbito.");
            return;
        }
        if (v->initializer) {
            v->initializer->accept(this); Type* initType = v->initializer->resolvedType;
            if (!check_assign(v->resolvedType, initType)) {
                error("tipos incompatibles en inicialización de '" + v->name + "'.");
            }
        }
        bind_var_decl(v);
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

    // Variable no procesada (global o local sin pre-asignación).
    Type* t = type_from_ast(v->type);

    if (t->match(voidType)) {
        error("no se puede declarar variable de tipo void.");
        return;
    }

    // Wrap en ArrayType si tiene dimensiones.
    for (int idx = (int)v->array_sizes.size() - 1; idx >= 0; idx--) {
        int dim = -1;
        if (auto* il = dynamic_cast<NumberLiteralNode*>(v->array_sizes[idx]))
            dim = (int)il->value;
        ArrayType* at = new ArrayType(t, dim);
        typeCache.push_back(at);
        t = at;
    }

    if (env.check_current(v->name)) {
        error("variable '" + v->name + "' ya declarada en este ámbito.");
        return;
    }

    v->resolvedType = t;
    v->memSize = t->size();

    bind_var_decl(v);

    if (v->initializer) {
        v->initializer->accept(this); Type* initType = v->initializer->resolvedType;
        if (!check_assign(t, initType)) {
            error("tipos incompatibles en inicialización de '" + v->name + "'.");
        }
    }
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
    if (v->initializer || !v->init_list.empty() || t->ttype == Type::STRUCT || t->ttype == Type::ARRAY) {
        initialized_vars.insert(v);
    }
}

// visit(StructDecl) — typecheck de declaración de struct.
// Crea el StructType, registra los miembros con sus tipos y
// calcula offsets y tamaño total secuencialmente.
void TypeChecker::visit(StructDecl* s) {
    StructType* st = new StructType(s->name);
    struct_types[s->name] = st;   // Registrar antes de procesar miembros (soporta auto-referencia)
    int offset = 0;

    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);

        // Wrap en ArrayType si tiene dimensiones.
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

// visit(Body) — typecheck de bloque { ... }.
// Abre un nuevo scope para variables locales, salvo que el body
// sea sintético (comma-decl).
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

// visit(ExprStmtNode) — expresión como statement.
// Verifica tipos de la expresión y descarta el valor.
void TypeChecker::visit(ExprStmtNode* s) {
    if (s->expr) s->expr->accept(this);
}

// visit(IfStmt) — typecheck de if/else.
// Verifica que la condición no sea void y procesa ambas ramas.
void TypeChecker::visit(IfStmt* s) {
    s->condition->accept(this); Type* t = s->condition->resolvedType;
    if (t->match(voidType)) {
        error("condición de if no puede ser void.");
    }
    s->then_branch->accept(this);
    if (s->else_branch) s->else_branch->accept(this);
}

// visit(WhileStmt) — typecheck de while.
// Incrementa loopDepth, verifica la condición y procesa el cuerpo.
void TypeChecker::visit(WhileStmt* s) {
    s->condition->accept(this); Type* t = s->condition->resolvedType;
    if (t->match(voidType)) {
        error("condición de while no puede ser void.");
    }
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
}

// visit(DoWhileStmt) — typecheck de do-while.
// Procesa el cuerpo primero (dentro de loopDepth), luego verifica
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

// visit(ForStmt) — typecheck de for.
// Abre scope para la variable de inicialización, procesa init,
// condición, cuerpo e incremento.
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

// visit(SwitchStmt) — typecheck de switch.
// Verifica que la expresión sea int/char/bool, incrementa
// switchDepth (para break), y procesa casos y default.
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

// visit(CaseClause) — typecheck de case.
// Valida el literal del case y typecheckea su cuerpo.
void TypeChecker::visit(CaseClause* s) {
    s->value->accept(this); Type* t = s->value->resolvedType;
    if (!is_switch_index_type(t)) {
        error("valor de case debe ser int o char.");
    }
    for (auto st : s->body) st->accept(this);
}

// visit(FreeStmt) — typecheck de free(ptr).
// Verifica que la expresión sea typecheckeable (debería ser puntero).
void TypeChecker::visit(FreeStmt* s) {
    s->expr->accept(this);
}

// visit(BreakStmt) — verifica break en contexto válido.
// break solo es válido dentro de un loop o switch.
void TypeChecker::visit(BreakStmt* s) {
    if (loopDepth == 0 && switchDepth == 0) {
        error("break fuera de ciclo o switch.", s->loc);
    }
}

// visit(ContinueStmt) — verifica continue en contexto válido.
// continue solo es válido dentro de un loop.
void TypeChecker::visit(ContinueStmt* s) {
    if (loopDepth == 0) {
        error("continue fuera de ciclo.", s->loc);
    }
}

// visit(ReturnStmt) — verifica return.
// Comprueba que función void no retorne valor, que función no-void
// retorne valor, y que los tipos sean compatibles.
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
// Expression type checking
// ============================================================

// visit(BinaryOpNode) — typecheck de operaciones binarias.
// Reglas:
//   Aritméticas: operandos aritméticos o aritmética de punteros
//     (ptr ± int, ptr - ptr). Promueve al tipo más grande.
//   Comparación: operandos aritméticos o punteros; resultado int.
//   Lógicos: operandos no void; resultado int.
void TypeChecker::visit(BinaryOpNode* e) {
    e->left->accept(this); Type* left = e->left->resolvedType;
    e->right->accept(this); Type* right = e->right->resolvedType;

    Type* resultType;
    switch (e->op) {
        // Operaciones aritméticas.
        case BinaryOp::ADD: case BinaryOp::SUB:
        case BinaryOp::MUL: case BinaryOp::DIV: case BinaryOp::MOD:
        case BinaryOp::POW:
            // Aritmética de punteros.
            if ((left->ttype == Type::POINTER && is_integral_type(right)) || 
                (is_integral_type(left) && right->ttype == Type::POINTER && e->op == BinaryOp::ADD)) {
                resultType = left->ttype == Type::POINTER ? left : right;
            } else if (left->ttype == Type::POINTER && right->ttype == Type::POINTER && e->op == BinaryOp::SUB) {
                resultType = intType;
            } else if (left->ttype == Type::POINTER && is_integral_type(right) && e->op == BinaryOp::SUB) {
                resultType = left;
            } else if (is_arithmetic_type(left) && is_arithmetic_type(right)) {
                // Promoción automática al tipo más grande.
                if (left->ttype == Type::DOUBLE || right->ttype == Type::DOUBLE)
                    resultType = doubleType;
                else if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
                    resultType = floatType;
                else
                    resultType = intType;
            } else {
                error("operación aritmética requiere int, char, float, double o aritmética de punteros.");
                resultType = intType;
            }
            break;

        // Comparaciones.
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

        // Lógicos.
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

// visit(UnaryOpNode) — typecheck de operaciones unarias.
//   MINUS: aritmético; integrales promueven a int.
//   PRE_INC/PRE_DEC/POST_INC/POST_DEC: aritmético, mismo tipo.
//   LOG_NOT: no void, resultado int.
//   ADDR: devuelve puntero al tipo del operando.
//   DEREF: devuelve tipo base del puntero.
void TypeChecker::visit(UnaryOpNode* e) {
    e->operand->accept(this); 
    Type* t = e->operand->resolvedType;
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
            if (t->ttype == Type::VOID) {
                error("! no puede aplicarse a void.");
            }
            resultType = intType;
            break;
        case UnaryOp::ADDR:
            {
                PointerType* ptr = new PointerType(t);
                typeCache.push_back(ptr);
                resultType = ptr;
            }
            break;
        case UnaryOp::DEREF:
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

// visit(AssignmentNode) — typecheck de asignación.
// Verifica que el valor sea asignable al target (check_assign),
// que target no sea const, y marca la variable como inicializada.
void TypeChecker::visit(AssignmentNode* e) {
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

// visit(MallocNode) — typecheck de malloc.
// malloc siempre retorna void*.
void TypeChecker::visit(MallocNode* e) {
    e->size->accept(this);
    PointerType* ptr = new PointerType(voidType);
    typeCache.push_back(ptr);
    e->resolvedType = ptr;
}

// visit(PrintfNode) — typecheck de printf.
// Procesa todos los argumentos; retorna void.
void TypeChecker::visit(PrintfNode* e) {
    for (auto a : e->args) a->accept(this);
    e->resolvedType = voidType;
}

// checkFuncCall — verifica argumentos de llamada a función.
// Comprueba cantidad de argumentos y que cada uno sea asignable
// al tipo del parámetro correspondiente. Retorna false si falla
// la cantidad.
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

// visit(FcallNode) — typecheck de llamada a función.
// Busca la función por nombre, verifica argumentos y fija el
// tipo resultante al tipo de retorno.
void TypeChecker::visit(FcallNode* e) {
    if (auto* id = dynamic_cast<IdNode*>(e->callee)) {
        string fname = id->name;

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

// visit(IndexNode) — typecheck de acceso por índice.
// Verifica que el índice sea entero y que la base sea arreglo o
// puntero. Retorna el tipo base.
void TypeChecker::visit(IndexNode* e) {
    e->base->accept(this); Type* base = e->base->resolvedType;
    e->index->accept(this); Type* index = e->index->resolvedType;
    if (!is_switch_index_type(index)) {
        error("índice de arreglo debe ser int o char.");
    }
    if (base->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)base;
        e->resolvedType = at->base;
        return;
    }
    if (base->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)base;
        e->resolvedType = pt->base;
        return;
    }
    error("acceso por índice requiere arreglo o puntero.");
    e->resolvedType = intType;
}

// visit(MemberAccessNode) — typecheck de acceso a miembro (.).
// Verifica que el objeto sea struct y que el miembro exista.
// Retorna el tipo del miembro.
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

// visit(ArrowAccessNode) — typecheck de acceso por flecha (->).
// Verifica que el operando sea puntero a struct y que el miembro
// exista. Retorna el tipo del miembro.
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

// visit(IdNode) — typecheck de identificador.
// Busca la variable en varEnv, asigna el binding y el tipo.
// En contexto rvalue, un arreglo decae a puntero a su primer
// elemento. Verifica uso de variables no inicializadas.
void TypeChecker::visit(IdNode* e) {
    VarDecl* vd = nullptr;
    if (!varEnv.lookup(e->name, vd)) {
        error("variable '" + e->name + "' no declarada.");
        e->resolvedType = intType;
        return;
    }
    e->binding = vd;
    
    // Array-to-pointer decay en contexto rvalue.
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

// Visits de literales: fijan resolvedType según el tipo literal.
void TypeChecker::visit(NumberLiteralNode* e) {
    using LS = Token::LiteralSuffix;
    switch (e->literalSuffix) {
    case LS::SUF_U:
        e->resolvedType = uintType;
        break;
    case LS::SUF_L:
        e->resolvedType = longType;
        break;
    case LS::SUF_LL:
        e->resolvedType = longType;
        break;
    case LS::SUF_UL:
        e->resolvedType = ulongType;
        break;
    case LS::SUF_ULL:
        e->resolvedType = ulongType;
        break;
    default:
        e->resolvedType = intType;
        break;
    }
}
void TypeChecker::visit(FloatLiteralNode* e) {
    e->resolvedType = (e->literalSuffix == Token::LiteralSuffix::SUF_F)
                      ? floatType : doubleType;
}
void TypeChecker::visit(BoolLiteralNode* e) {
    e->resolvedType = boolType;
}
void TypeChecker::visit(CharLiteralNode* e) {
    e->resolvedType = charType;
}
void TypeChecker::visit(StringLiteralNode* e) {
    // Tratado como puntero a char (char*).
    PointerType* pt = new PointerType(charType);
    typeCache.push_back(pt);
    e->resolvedType = pt;
}

// Nodos de tipo en contextos: delegan la resolución a type_from_ast.
void TypeChecker::visit(PrimitiveTypeNode*) { }
void TypeChecker::visit(PointerTypeNode*) { }
void TypeChecker::visit(StructTypeNode*) { }

// visit(SizeOfNode) — typecheck de sizeof.
// Retorna int con el tamaño en bytes del tipo o expresión.
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

