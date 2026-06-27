#include "visitor.h"
#include <iostream>
#include <functional>

using namespace std;

// ============================================================
// accept() methods for TypeVisitor
// ============================================================

Type* BinaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* UnaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* AssignmentNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* TernaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* FcallNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* MallocNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* IndexNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* MemberAccessNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* ArrowAccessNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* CastNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* SizeOfNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* LambdaExprNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* CaptureNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* IdentifierNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* IntegerLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* FloatLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* BoolLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* CharLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* StringLiteralNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* ParenthesizedExprNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* PrimitiveTypeNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* PointerTypeNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* StructTypeNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* NamedTypeNode::accept(TypeVisitor* v) { return v->visit(this); }

Type* TemplateTypeNode::accept(TypeVisitor* v) { return v->visit(this); }

void ExprStmtNode::accept(TypeVisitor* v) { v->visit(this); }
void DeclStmt::accept(TypeVisitor* v) { v->visit(this); }
void IfStmt::accept(TypeVisitor* v) { v->visit(this); }
void WhileStmt::accept(TypeVisitor* v) { v->visit(this); }
void DoWhileStmt::accept(TypeVisitor* v) { v->visit(this); }
void ForStmt::accept(TypeVisitor* v) { v->visit(this); }
void SwitchStmt::accept(TypeVisitor* v) { v->visit(this); }
void CaseClause::accept(TypeVisitor* v) { v->visit(this); }
void DefaultClause::accept(TypeVisitor* v) { v->visit(this); }
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
    charType = new Type(Type::INT); // treat char as int for now
    retornodefuncion = nullptr;
    loopDepth = 0;
    switchDepth = 0;
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
            case PrimitiveTypeNode::DOUBLE: return floatType;
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
        subs[tdecl->params[i]->name] = type_from_ast(args[i]);
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
    // Promoción: int → float
    if (target->ttype == Type::FLOAT && value->ttype == Type::INT) return true;
    return false;
}

// ============================================================
// Helper: registrar error semántico (no aborta)
// ============================================================

void TypeChecker::error(const string& msg) {
    errors.push_back(msg);
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

// ============================================================
// Built-in function registration
// ============================================================

static void register_builtins(unordered_map<string, FuncInfo>& functions) {
    FuncInfo printInfo;
    printInfo.returnType = new Type(Type::VOID);
    functions["print"] = printInfo;
    functions["printf"] = printInfo;
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
    for (auto p : f->params) {
        env.add_var(p->name, type_from_ast(p->type));
    }
    retornodefuncion = type_from_ast(f->return_type);
    f->body->accept(this);
    env.remove_level();
}

void TypeChecker::visit(VarDecl* v) {
    Type* t = type_from_ast(v->type);

    // Wrap en ArrayType si tiene dimensiones de arreglo
    for (auto s : v->array_sizes) {
        ArrayType* at = new ArrayType(t, -1);
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

    if (env.check(v->name)) {
        error("variable '" + v->name + "' ya declarada en este ámbito.");
        return;
    }
    env.add_var(v->name, t);

    // Verificar inicializador (si no es auto, porque auto ya lo chequeó)
    if (v->initializer) {
        // Si es auto, ya se verificó el inicializador arriba
        bool isAuto = false;
        if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(v->type)) {
            isAuto = (pt->prim == PrimitiveTypeNode::AUTO);
        }
        if (!isAuto) {
            Type* initType = v->initializer->accept(this);
            if (!check_assign(t, initType)) {
                error("tipos incompatibles en inicialización de '" + v->name + "'.");
            }
        }
    }
}

void TypeChecker::visit(StructDecl* s) {
    // Crear StructType y registrar sus miembros
    StructType* st = new StructType(s->name);
    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        st->members[m->name] = mt;
    }
    struct_types[s->name] = st;
}

void TypeChecker::visit(Body* b) {
    env.add_level();
    for (auto v : b->vdlist) v->accept(this);
    for (auto s : b->stmts) s->accept(this);
    env.remove_level();
}

void TypeChecker::visit(ExprStmtNode* s) {
    if (s->expr) s->expr->accept(this);
}

void TypeChecker::visit(DeclStmt* s) {
    s->decl->accept(this);
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
    if (!t->match(intType) && !t->match(charType)) {
        error("expresión de switch debe ser int o char.");
    }
    switchDepth++;
    for (auto c : s->cases) c->accept(this);
    switchDepth--;
}

void TypeChecker::visit(CaseClause* s) {
    Type* t = s->value->accept(this);
    if (!t->match(intType) && !t->match(charType)) {
        error("valor de case debe ser int o char.");
    }
    for (auto st : s->body) st->accept(this);
}

void TypeChecker::visit(DefaultClause* s) {
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
    // break válido dentro de ciclo o switch
    if (loopDepth == 0 && switchDepth == 0) {
        error("break fuera de ciclo o switch.");
    }
}

void TypeChecker::visit(ContinueStmt* s) {
    // continue válido solo dentro de ciclo
    if (loopDepth == 0) {
        error("continue fuera de ciclo.");
    }
}

void TypeChecker::visit(ReturnStmt* s) {
    if (retornodefuncion->match(voidType) && s->expr) {
        error("función void no debe retornar valor.");
        return;
    }
    if (s->expr) {
        Type* t = s->expr->accept(this);
        if (!check_assign(retornodefuncion, t)) {
            error("tipo de retorno incompatible (esperaba " +
                  retornodefuncion->str() + ").");
        }
    }
}

// ============================================================
// Expression type checking
// ============================================================

Type* TypeChecker::visit(BinaryOpNode* e) {
    Type* left = e->left->accept(this);
    Type* right = e->right->accept(this);

    switch (e->op) {
        // Operaciones aritméticas: permiten int o float, promueven a float
        case BinaryOp::ADD: case BinaryOp::SUB:
        case BinaryOp::MUL: case BinaryOp::DIV: case BinaryOp::MOD:
        case BinaryOp::POW:
            if ((left->ttype == Type::INT || left->ttype == Type::FLOAT) &&
                (right->ttype == Type::INT || right->ttype == Type::FLOAT)) {
                // Promoción automática: si alguno es float, resultado float
                if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
                    return floatType;
                return intType;
            }
            error("operación aritmética requiere int o float.");
            return intType;

        // Comparaciones: int o float, resultado bool
        case BinaryOp::EQ: case BinaryOp::NE:
        case BinaryOp::LT: case BinaryOp::GT:
        case BinaryOp::LE: case BinaryOp::GE:
            if ((left->ttype == Type::INT || left->ttype == Type::FLOAT) &&
                (right->ttype == Type::INT || right->ttype == Type::FLOAT))
                return boolType;
            error("comparación requiere int o float.");
            return boolType;

        // Lógicos: operandos bool, resultado bool
        case BinaryOp::LOG_AND: case BinaryOp::LOG_OR:
            if (left->match(boolType) && right->match(boolType))
                return boolType;
            error("operación lógica requiere bool.");
            return boolType;

        default:
            return intType;
    }
}

Type* TypeChecker::visit(UnaryOpNode* e) {
    Type* t = e->operand->accept(this);
    switch (e->op) {
        case UnaryOp::MINUS:
        case UnaryOp::PRE_INC: case UnaryOp::PRE_DEC:
        case UnaryOp::POST_INC: case UnaryOp::POST_DEC:
            if (t->ttype == Type::INT || t->ttype == Type::FLOAT) return t;
            error("operación unaria requiere int o float.");
            return t;
        case UnaryOp::LOG_NOT:
            if (t->match(boolType)) return boolType;
            error("! requiere bool.");
            return boolType;
        case UnaryOp::ADDR:
            // &x devuelve puntero al tipo de x
            {
                PointerType* ptr = new PointerType(t);
                typeCache.push_back(ptr);
                return ptr;
            }
        case UnaryOp::DEREF:
            // *p devuelve el tipo base del puntero
            if (t->ttype == Type::POINTER) {
                PointerType* pt = (PointerType*)t;
                return pt->base;
            }
            error("operando de * debe ser puntero.");
            return intType;
    }
    return intType;
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

Type* TypeChecker::visit(TernaryOpNode* e) {
    Type* cond = e->condition->accept(this);
    if (!cond->match(boolType)) {
        error("condición ternaria debe ser bool.");
    }
    Type* thenType = e->then_expr->accept(this);
    Type* elseType = e->else_expr->accept(this);
    if (!thenType->match(elseType)) {
        // Permitir int→float en ternary
        if (thenType->ttype == Type::FLOAT && elseType->ttype == Type::INT)
            return floatType;
        if (thenType->ttype == Type::INT && elseType->ttype == Type::FLOAT)
            return floatType;
        error("tipos incompatibles en expresión ternaria.");
    }
    return thenType;
}

Type* TypeChecker::visit(MallocNode* e) {
    e->size->accept(this);
    return intType; // returns int* (simplified)
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
                subs[tdecl->params[i]->name] = type_from_ast(e->template_args[i]);

            Type* concrete_ret = type_from_ast(tfunc->return_type);
            if (auto* nt = dynamic_cast<NamedTypeNode*>(tfunc->return_type)) {
                auto sit = subs.find(nt->name);
                if (sit != subs.end()) concrete_ret = sit->second;
            }
            vector<Type*> concrete_params;
            for (auto p : tfunc->params) {
                Type* pt = type_from_ast(p->type);
                if (auto* nt = dynamic_cast<NamedTypeNode*>(p->type)) {
                    auto sit = subs.find(nt->name);
                    if (sit != subs.end()) pt = sit->second;
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

        if (fname == "print" || fname == "printf") {
            for (auto a : e->args) a->accept(this);
            return info.returnType;
        }

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
    if (!index->match(intType) && !index->match(charType)) {
        error("índice de arreglo debe ser int o char.");
    }
    // Acceso a arreglo: devuelve tipo base
    if (base->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)base;
        return at->base;
    }
    // Acceso a puntero: devuelve tipo base (equivalente a *(p + i))
    if (base->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)base;
        return pt->base;
    }
    error("acceso por índice requiere arreglo o puntero.");
    return intType;
}

Type* TypeChecker::visit(MemberAccessNode* e) {
    Type* objType = e->object->accept(this);
    if (objType->ttype != Type::STRUCT) {
        error("acceso a miembro '.' sobre tipo no struct (es " +
              objType->str() + ").");
        return intType;
    }
    StructType* st = (StructType*)objType;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" +
              e->member + "'.");
        return intType;
    }
    return it->second;
}

Type* TypeChecker::visit(ArrowAccessNode* e) {
    Type* ptrType = e->pointer->accept(this);
    if (ptrType->ttype != Type::POINTER) {
        error("acceso '->' requiere puntero (es " + ptrType->str() + ").");
        return intType;
    }
    PointerType* pt = (PointerType*)ptrType;
    if (pt->base->ttype != Type::STRUCT) {
        error("acceso '->' requiere puntero a struct.");
        return intType;
    }
    StructType* st = (StructType*)pt->base;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" +
              e->member + "'.");
        return intType;
    }
    return it->second;
}

Type* TypeChecker::visit(CastNode* e) {
    return type_from_ast(e->target_type);
}

Type* TypeChecker::visit(IdentifierNode* e) {
    if (!env.check(e->name)) {
        error("variable '" + e->name + "' no declarada.");
        return intType;
    }
    return env.lookup(e->name);
}

Type* TypeChecker::visit(IntegerLiteralNode* e) { return intType; }
Type* TypeChecker::visit(FloatLiteralNode* e) { return floatType; }
Type* TypeChecker::visit(BoolLiteralNode* e) { return boolType; }
Type* TypeChecker::visit(CharLiteralNode* e) { return charType; }
Type* TypeChecker::visit(StringLiteralNode* e) { return intType; }
Type* TypeChecker::visit(ParenthesizedExprNode* e) { return e->expr->accept(this); }
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
    for (auto p : e->params) p->accept(this);
    if (e->return_type) type_from_ast(e->return_type);
    if (e->body) e->body->accept(this);
    return intType; // tipo de lambda no especificado
}

Type* TypeChecker::visit(CaptureNode* e) {
    return intType;
}
