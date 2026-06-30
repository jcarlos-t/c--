#include "visitor.h"
#include <iostream>
#include <sstream>
#include <functional>
#include <algorithm>

using namespace std;

namespace {

bool is_integral_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR;
}

bool is_arithmetic_type(Type* t) {
    return is_integral_type(t) ||
           t->ttype == Type::FLOAT ||
           t->ttype == Type::DOUBLE;
}

bool is_switch_index_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR;
}

} // namespace

// ============================================================
// accept() methods for TypeVisitor
// ============================================================

Type* BinaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* UnaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* AssignmentNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* FcallNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* MallocNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* IndexNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* MemberAccessNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* ArrowAccessNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* SizeOfNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* LambdaExprNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* CaptureNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* IdentifierNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* IntegerLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* FloatLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* BoolLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* CharLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* StringLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* PrintfNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* PrimitiveTypeNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* PointerTypeNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* StructTypeNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* NamedTypeNode::accept(TypeVisitor* v) { return v->visit(this); }

Type* TemplateTypeNode::accept(TypeVisitor* v) { return v->visit(this); }

void ExprStmtNode::accept(TypeVisitor* v) { v->visit(this); }
void IfStmt::accept(TypeVisitor* v) { v->visit(this); }
void WhileStmt::accept(TypeVisitor* v) { v->visit(this); }
void DoWhileStmt::accept(TypeVisitor* v) { v->visit(this); }
void ForStmt::accept(TypeVisitor* v) { v->visit(this); }
void SwitchStmt::accept(TypeVisitor* v) { v->visit(this); }
void CaseClause::accept(TypeVisitor* v) { v->visit(this); }
void BreakStmt::accept(TypeVisitor* v) { v->visit(this); }
void ContinueStmt::accept(TypeVisitor* v) { v->visit(this); }
void ReturnStmt::accept(TypeVisitor* v) { v->visit(this); }
void FreeStmt::accept(TypeVisitor* v) { v->visit(this); }
void VarDecl::accept(TypeVisitor* v) { v->visit(this); }
void FunDecl::accept(TypeVisitor* v) { v->visit(this); }
void StructDecl::accept(TypeVisitor* v) { v->visit(this); }
void TemplateDecl::accept(TypeVisitor* v) { v->visit(this); }
void Body::accept(TypeVisitor* v) { v->visit(this); }
void Program::accept(TypeVisitor* v) { v->visit(this); }

// ============================================================
// Constructor / Destructor
// ============================================================

TypeChecker::TypeChecker() {
    intType = new Type(Type::INT);
    boolType = new Type(Type::BOOL);
    voidType = new Type(Type::VOID);
    floatType = new Type(Type::FLOAT);
    doubleType = new Type(Type::DOUBLE);
    charType = new Type(Type::CHAR);
    retornodefuncion = nullptr;
    loopDepth = 0;
    switchDepth = 0;
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
// Helper: convertir nodo tipo del AST a Type* semántico
// ============================================================

Type* TypeChecker::type_from_ast(Exp* t) {
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
    if (auto* pt = dynamic_cast<PointerTypeNode*>(t)) {
        Type* base = type_from_ast(pt->base);
        PointerType* ptr = new PointerType(base);
        typeCache.push_back(ptr);
        return ptr;
    }
    if (auto* st = dynamic_cast<StructTypeNode*>(t)) {
        auto it = struct_types.find(st->name);
        if (it != struct_types.end()) return it->second;
        error("struct '" + st->name + "' no declarado.");
        return intType;
    }
    if (auto* nt = dynamic_cast<NamedTypeNode*>(t)) {
        error("tipo no reconocido: '" + nt->name + "'.");
        return intType;
    }
    if (auto* tt = dynamic_cast<TemplateTypeNode*>(t)) {
        return instantiate_template(tt->name, tt->type_args);
    }
    return intType; // fallback
}

// ============================================================
// Helper: instanciar template struct
// ============================================================

StructType* TypeChecker::instantiate_template(const string& name, const vector<TypeNode*>& args) {
    string concrete_name = name + "<";
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) concrete_name += ",";
        if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(args[i])) {
            switch (pt->prim) {
                case PrimitiveTypeNode::INT: concrete_name += "int"; break;
                case PrimitiveTypeNode::FLOAT: concrete_name += "float"; break;
                case PrimitiveTypeNode::CHAR: concrete_name += "char"; break;
                case PrimitiveTypeNode::DOUBLE: concrete_name += "double"; break;
                case PrimitiveTypeNode::BOOL: concrete_name += "bool"; break;
                default: concrete_name += "?"; break;
            }
        } else if (auto* st = dynamic_cast<StructTypeNode*>(args[i])) {
            concrete_name += st->name;
        } else {
            concrete_name += "?";
        }
    }
    concrete_name += ">";

    auto cit = struct_types.find(concrete_name);
    if (cit != struct_types.end()) return cit->second;

    auto it = template_decls.find(name);
    if (it == template_decls.end() || it->second->is_function) {
        error("template '" + name + "' no declarado.");
        StructType* err = new StructType("error");
        typeCache.push_back(err);
        return err;
    }

    TemplateDecl* tdecl = it->second;
    StructDecl* orig = tdecl->struct_decl;
    unordered_map<string, Type*> subs;
    for (size_t i = 0; i < tdecl->params.size() && i < args.size(); i++) {
        subs[tdecl->params[i]] = type_from_ast(args[i]);
    }

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

    StructType* st = new StructType(concrete_name);
    for (auto member : orig->members) {
        Type* mt = substitute(member->type);
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
// Helper: verifica compatibilidad de tipos en asignación
// Permite promoción automática int → float
// ============================================================

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
// Helper: registrar error semántico (no aborta)
// ============================================================

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
// Registrar funciones
// ============================================================

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
// Typecheck entry point
// ============================================================

void TypeChecker::typecheck(Program* program) {
    hasError = false;
    errors.clear();
    if (program) program->accept(this);
    if (hasError) {
        for (auto& e : errors) cerr << "Error semántico: " << e << endl;
        exit(1);
    }
    cout << "Revisión exitosa" << endl;
}

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
// Built-in function registration
// ============================================================

static void register_builtins(unordered_map<string, FuncInfo>& functions) {
    (void)functions;
}

// ============================================================
// Visits: declaraciones y statements
// ============================================================

void TypeChecker::visit(Program* p) {
    register_builtins(functions);
    for (auto f : p->functions) add_function(f);
    env.add_level();
    for (auto g : p->globals) g->accept(this);
    for (auto s : p->structs) s->accept(this);
    for (auto t : p->templates) t->accept(this);
    for (auto f : p->functions) f->accept(this);
    env.remove_level();
}

void TypeChecker::visit(FunDecl* f) {
    env.add_level();
    
    // Recolectar parámetros
    vector<VarDecl*> params;
    for (auto p : f->params) {
        Type* t = type_from_ast(p->type);
        if (t->match(voidType))
            error("parámetro no puede ser de tipo void.");
        p->memSize = t->size();
        p->resolvedType = t;
        params.push_back(p);
    }
    
    // Asignar offsets a parámetros con bin packing
    assignOffsetsWithBinPacking(params, 0);
    int paramSize = 0;
    for (auto p : params) {
        int end = p->offset + p->memSize;
        if (end > paramSize) paramSize = end;
    }
    
    // Recolectar todas las variables locales del body para calcular offsets
    vector<VarDecl*> localVars;
    collectVars(f->body, localVars);
    
    // Asignar tipos y tamaños a variables locales (sin agregar al environment)
    for (auto v : localVars) {
        if (v->resolvedType == nullptr) {
            Type* t = type_from_ast(v->type);
            if (t->match(voidType)) {
                error("no se puede declarar variable de tipo void.");
                t = intType;
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
            // Inferencia auto
            if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(v->type)) {
                if (pt->prim == PrimitiveTypeNode::AUTO) {
                    if (!v->initializer) {
                        error("variable 'auto' necesita inicializador para inferir tipo.");
                        t = intType;
                    } else {
                        t = v->initializer->accept(this);
                    }
                }
            }
            v->resolvedType = t;
            v->memSize = t->size();
        }
    }
    
    // Asignar offsets a variables locales con bin packing (después de parámetros)
    assignOffsetsWithBinPacking(localVars, paramSize);
    
    // Calcular tamaño total usado
    int totalSize = paramSize;
    for (auto v : localVars) {
        int end = v->offset + v->memSize;
        if (end > totalSize) totalSize = end;
    }
    
    // Calcular frame size total (alineado a 16 bytes)
    f->frameSize = (totalSize + 15) & ~15;
    
    // Convertir offsets a negativos (stack frame)
    for (auto p : params) {
        p->offset = p->offset - f->frameSize;
    }
    for (auto v : localVars) {
        v->offset = v->offset - f->frameSize;
    }
    
    retornodefuncion = type_from_ast(f->return_type);
    
    // Agregar parámetros al environment
    for (auto p : params) {
        env.add_var(p->name, p->resolvedType);
    }
    
    // Procesar body (las variables locales se agregarán al environment cuando se visiten)
    f->body->accept(this);

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
    
    env.remove_level();
}

// Recolectar todas las variables declaradas en un statement (recursivamente)
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

// Asignar offsets con bin packing: ordenar por tamaño descendente y agrupar en slots de 8 bytes
void TypeChecker::assignOffsetsWithBinPacking(vector<VarDecl*>& vars, int startOffset) {
    if (vars.empty()) return;
    
    // Ordenar por tamaño descendente (8, 4, 1)
    sort(vars.begin(), vars.end(), [](VarDecl* a, VarDecl* b) {
        return a->memSize > b->memSize;
    });
    
    // Estructura para trackear slots
    struct Slot {
        int used = 0;  // bytes usados en este slot
    };
    vector<Slot> slots;
    slots.push_back(Slot());  // primer slot
    
    // Asignar cada variable al primer slot donde quepa
    for (auto v : vars) {
        int size = v->memSize;
        if (size == 0) continue; // Skip variables with size 0 (void)
        bool placed = false;
        
        for (size_t i = 0; i < slots.size(); i++) {
            // Verificar alineación
            int align = size;
            if (align > 8) align = 8;
            
            // Calcular offset dentro del slot con alineación
            int offsetInSlot = slots[i].used;
            if (offsetInSlot % align != 0) {
                offsetInSlot += align - (offsetInSlot % align);
            }
            
            // Si cabe en este slot
            if (offsetInSlot + size <= 8) {
                v->offset = startOffset + i * 8 + offsetInSlot;
                slots[i].used = offsetInSlot + size;
                placed = true;
                break;
            }
        }
        
        // Si no cabe en ningún slot, crear uno nuevo
        if (!placed) {
            Slot newSlot;
            v->offset = startOffset + slots.size() * 8;
            newSlot.used = size;
            slots.push_back(newSlot);
        }
    }
}

void TypeChecker::visit(VarDecl* v) {
    // Si ya tiene resolvedType, fue procesado por visit(FunDecl*) con bin packing
    // Verificar duplicados antes de agregar al environment
    if (v->resolvedType != nullptr) {
        if (env.check_current(v->name)) {
            error("variable '" + v->name + "' ya declarada en este ámbito.");
            return;
        }
        env.add_var(v->name, v->resolvedType);
        if (v->initializer) {
            Type* initType = v->initializer->accept(this);
            if (!check_assign(v->resolvedType, initType)) {
                error("tipos incompatibles en inicialización de '" + v->name + "'.");
            }
        }
        return;
    }
    
    Type* t = type_from_ast(v->type);

    if (t->match(voidType)) {
        error("no se puede declarar variable de tipo void.");
        return;
    }

    // Wrap en ArrayType si tiene dimensiones de arreglo
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
                t = v->initializer->accept(this);
            }
        }
    }

    if (env.check_current(v->name)) {
        error("variable '" + v->name + "' ya declarada en este ámbito.");
        return;
    }
    
    // Guardar tipo resuelto y tamaño
    v->resolvedType = t;
    v->memSize = t->size();
    
    env.add_var(v->name, t);

    // Verificar inicializador
    if (v->initializer) {
        Type* initType = v->initializer->accept(this);
        if (!check_assign(t, initType)) {
            error("tipos incompatibles en inicialización de '" + v->name + "'.");
        }
    }
}

void TypeChecker::visit(StructDecl* s) {
    // Crear StructType y registrar sus miembros
    StructType* st = new StructType(s->name);
    int offset = 0;
    
    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        st->members[m->name] = mt;
        s->memberOffsets[m->name] = offset;
        s->memberSizes[m->name] = mt->size(); // add size!
        offset += mt->size();
    }
    
    s->totalSize = offset;
    struct_types[s->name] = st;
}

void TypeChecker::visit(Body* b) {
    env.add_level();
    for (auto s : b->stmts) s->accept(this);
    env.remove_level();
}

void TypeChecker::visit(ExprStmtNode* s) {
    if (s->expr) s->expr->accept(this);
}

void TypeChecker::visit(IfStmt* s) {
    Type* t = s->condition->accept(this);
    if (!t->match(boolType)) {
        error("condición de if debe ser bool.");
    }
    s->then_branch->accept(this);
    if (s->else_branch) s->else_branch->accept(this);
}

void TypeChecker::visit(WhileStmt* s) {
    Type* t = s->condition->accept(this);
    if (!t->match(boolType)) {
        error("condición de while debe ser bool.");
    }
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
}

void TypeChecker::visit(DoWhileStmt* s) {
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
    Type* t = s->condition->accept(this);
    if (!t->match(boolType)) {
        error("condición de do-while debe ser bool.");
    }
}

void TypeChecker::visit(ForStmt* s) {
    env.add_level();
    if (s->init) s->init->accept(this);
    if (s->condition) {
        Type* t = s->condition->accept(this);
        if (!t->match(boolType)) {
            error("condición de for debe ser bool.");
        }
    }
    loopDepth++;
    s->body->accept(this);
    loopDepth--;
    if (s->increment) s->increment->accept(this);
    env.remove_level();
}

void TypeChecker::visit(SwitchStmt* s) {
    Type* t = s->expr->accept(this);
    if (!is_switch_index_type(t)) {
        error("expresión de switch debe ser int o char.");
    }
    switchDepth++;
    for (auto cc : s->cases) cc->accept(this);
    for (auto st : s->default_body) st->accept(this);
    switchDepth--;
}

void TypeChecker::visit(CaseClause* s) {
    Type* t = s->value->accept(this);
    if (!is_switch_index_type(t)) {
        error("valor de case debe ser int o char.");
    }
    for (auto st : s->body) st->accept(this);
}

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

void TypeChecker::visit(BreakStmt* s) {
    if (loopDepth == 0 && switchDepth == 0) {
        error("break fuera de ciclo o switch.", s->loc);
    }
}

void TypeChecker::visit(ContinueStmt* s) {
    if (loopDepth == 0) {
        error("continue fuera de ciclo.", s->loc);
    }
}

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
        Type* t = s->expr->accept(this);
        if (!check_assign(retornodefuncion, t)) {
            error("tipo de retorno incompatible (esperaba " +
                  retornodefuncion->str() + ").", s->loc);
        }
    }
}

// ============================================================
// Expression type checking
// ============================================================

Type* TypeChecker::visit(BinaryOpNode* e) {
    Type* left = e->left->accept(this);
    Type* right = e->right->accept(this);

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

        // Comparaciones: int, char, float o double, resultado bool
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
    return resultType;
}

Type* TypeChecker::visit(UnaryOpNode* e) {
    Type* t = e->operand->accept(this);
    Type* resultType;
    switch (e->op) {
        case UnaryOp::MINUS:
            if (is_arithmetic_type(t)) {
                // char promueve a int en operaciones unarias aritméticas
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
            {
                PointerType* ptr = new PointerType(t);
                typeCache.push_back(ptr);
                resultType = ptr;
            }
            break;
        case UnaryOp::DEREF:
            // *p devuelve el tipo base del puntero
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
    return resultType;
}

Type* TypeChecker::visit(AssignmentNode* e) {
    Type* targetType = e->target->accept(this);
    Type* valueType = e->value->accept(this);
    if (!check_assign(targetType, valueType)) {
        error("tipos incompatibles en asignación (se esperaba " +
              targetType->str() + ").");
    }
    return targetType;
}

Type* TypeChecker::visit(MallocNode* e) {
    e->size->accept(this);
    PointerType* ptr = new PointerType(voidType);
    typeCache.push_back(ptr);
    return ptr;
}

Type* TypeChecker::visit(PrintfNode* e) {
    for (auto a : e->args) a->accept(this);
    return voidType;
}

Type* TypeChecker::visit(FcallNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->callee)) {
        string fname = id->name;

        if (!e->template_args.empty()) {
            auto tit = template_decls.find(fname);
            if (tit == template_decls.end() || !tit->second->is_function) {
                error("template de función '" + fname + "' no declarado.");
                return intType;
            }
            TemplateDecl* tdecl = tit->second;
            FunDecl* tfunc = tdecl->func;
            unordered_map<string, Type*> subs;
            for (size_t i = 0; i < tdecl->params.size() && i < e->template_args.size(); i++)
                subs[tdecl->params[i]] = type_from_ast(e->template_args[i]);

            Type* concrete_ret;
            if (auto* nt = dynamic_cast<NamedTypeNode*>(tfunc->return_type)) {
                auto sit = subs.find(nt->name);
                concrete_ret = (sit != subs.end()) ? sit->second : intType;
            } else {
                concrete_ret = type_from_ast(tfunc->return_type);
            }
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
            FuncInfo info;
            info.returnType = concrete_ret;
            info.paramTypes = concrete_params;
            functions[fname] = info;

            if (e->args.size() != info.paramTypes.size()) {
                error("número de argumentos incorrecto en llamada a '" + fname +
                      "' (esperaba " + to_string(info.paramTypes.size()) +
                      ", recibió " + to_string(e->args.size()) + ").");
                return info.returnType;
            }
            for (size_t i = 0; i < e->args.size(); i++) {
                Type* argType = e->args[i]->accept(this);
                if (!check_assign(info.paramTypes[i], argType)) {
                    error("tipo de argumento " + to_string(i+1) +
                          " incorrecto en llamada a '" + fname + "'.");
                }
            }
            return info.returnType;
        }

        auto it = functions.find(fname);
        if (it == functions.end()) {
            error("función no declarada '" + fname + "'.");
            return intType;
        }
        FuncInfo& info = it->second;

        if (e->args.size() != info.paramTypes.size()) {
            error("número de argumentos incorrecto en llamada a '" + fname +
                  "' (esperaba " + to_string(info.paramTypes.size()) +
                  ", recibió " + to_string(e->args.size()) + ").");
            return info.returnType;
        }
        for (size_t i = 0; i < e->args.size(); i++) {
            Type* argType = e->args[i]->accept(this);
            if (!check_assign(info.paramTypes[i], argType)) {
                error("tipo de argumento " + to_string(i+1) +
                      " incorrecto en llamada a '" + fname + "'.");
            }
        }
        return info.returnType;
    }
    error("llamada a función no reconocida.");
    return intType;
}

Type* TypeChecker::visit(IndexNode* e) {
    Type* base = e->base->accept(this);
    Type* index = e->index->accept(this);
    // El índice debe ser int
    if (!is_switch_index_type(index)) {
        error("índice de arreglo debe ser int o char.");
    }
    // Acceso a arreglo: devuelve tipo base
    if (base->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)base;
        e->resolvedType = at->base;
        return at->base;
    }
    // Acceso a puntero: devuelve tipo base (equivalente a *(p + i))
    if (base->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)base;
        e->resolvedType = pt->base;
        return pt->base;
    }
    error("acceso por índice requiere arreglo o puntero.");
    e->resolvedType = intType;
    return intType;
}

Type* TypeChecker::visit(MemberAccessNode* e) {
    Type* objType = e->object->accept(this);
    if (objType->ttype != Type::STRUCT) {
        error("acceso a miembro '.' sobre tipo no struct (es " +
              objType->str() + ").");
        e->resolvedType = intType;
        return intType;
    }
    StructType* st = (StructType*)objType;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" +
              e->member + "'.");
        e->resolvedType = intType;
        return intType;
    }
    e->resolvedType = it->second;
    return it->second;
}

Type* TypeChecker::visit(ArrowAccessNode* e) {
    Type* ptrType = e->pointer->accept(this);
    if (ptrType->ttype != Type::POINTER) {
        error("acceso '->' requiere puntero (es " + ptrType->str() + ").");
        e->resolvedType = intType;
        return intType;
    }
    PointerType* pt = (PointerType*)ptrType;
    if (pt->base->ttype != Type::STRUCT) {
        error("acceso '->' requiere puntero a struct.");
        e->resolvedType = intType;
        return intType;
    }
    StructType* st = (StructType*)pt->base;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" +
              e->member + "'.");
        e->resolvedType = intType;
        return intType;
    }
    e->resolvedType = it->second;
    return it->second;
}

Type* TypeChecker::visit(IdentifierNode* e) {
    if (!env.check(e->name)) {
        error("variable '" + e->name + "' no declarada.");
        e->resolvedType = intType; // fallback type
        return intType;
    }
    e->resolvedType = env.lookup(e->name);
    return e->resolvedType;
}

Type* TypeChecker::visit(IntegerLiteralNode* e) { 
    e->resolvedType = intType;
    return intType; 
}
Type* TypeChecker::visit(FloatLiteralNode* e) { 
    e->resolvedType = floatType;
    return floatType; 
}
Type* TypeChecker::visit(BoolLiteralNode* e) { 
    e->resolvedType = boolType;
    return boolType; 
}
Type* TypeChecker::visit(CharLiteralNode* e) { 
    e->resolvedType = charType;
    return charType; 
}
Type* TypeChecker::visit(StringLiteralNode* e) { 
    e->resolvedType = intType;
    return intType; 
}
Type* TypeChecker::visit(PrimitiveTypeNode* e) { return type_from_ast(e); }
Type* TypeChecker::visit(PointerTypeNode* e) { return type_from_ast(e); }
Type* TypeChecker::visit(StructTypeNode* e) { return type_from_ast(e); }
Type* TypeChecker::visit(NamedTypeNode* e) { return type_from_ast(e); }

Type* TypeChecker::visit(TemplateTypeNode* e) { return type_from_ast(e); }

Type* TypeChecker::visit(SizeOfNode* e) {
    e->target_type->accept(this);
    return intType;
}

Type* TypeChecker::visit(LambdaExprNode* e) {
    env.add_level();
    for (auto p : e->params) p->accept(this);
    Type* savedRet = retornodefuncion;
    if (e->return_type)
        retornodefuncion = type_from_ast(e->return_type);
    else
        retornodefuncion = voidType;
    if (e->body) e->body->accept(this);
    retornodefuncion = savedRet;
    env.remove_level();
    return retornodefuncion;
}

Type* TypeChecker::visit(CaptureNode* e) {
    return intType;
}
