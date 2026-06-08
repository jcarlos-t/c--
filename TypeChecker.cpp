#include "TypeChecker.h"
#include <iostream>

using namespace std;

// ============================================================
// accept() methods for TypeVisitor
// ============================================================

Type* BinaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* UnaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* AssignmentNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* TernaryOpNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* CallNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* MallocNode::accept(TypeVisitor* v) { return v->visit(this); }
Type* SubscriptNode::accept(TypeVisitor* v) { return v->visit(this); }
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
void CompoundStmt::accept(TypeVisitor* v) { v->visit(this); }
void Program::accept(TypeVisitor* v) { v->visit(this); }

// ============================================================
// Constructor
// ============================================================

TypeChecker::TypeChecker() {
    intType = new Type(Type::INT);
    boolType = new Type(Type::BOOL);
    voidType = new Type(Type::VOID);
    floatType = new Type(Type::FLOAT);
    charType = new Type(Type::INT); // treat char as int for now
    retornodefuncion = nullptr;
}

// ============================================================
// Helper: convert AST type node to semantic Type
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
            case PrimitiveTypeNode::AUTO:   return intType; // default
        }
    }
    return intType; // default fallback
}

// ============================================================
// Register functions
// ============================================================

void TypeChecker::add_function(FunDecl* fd) {
    if (functions.find(fd->name) != functions.end()) {
        cerr << "Error: función '" << fd->name << "' ya declarada." << endl;
        exit(0);
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
// Typecheck entry
// ============================================================

void TypeChecker::typecheck(Program* program) {
    if (program) program->accept(this);
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
// Visits
// ============================================================

void TypeChecker::visit(Program* p) {
    register_builtins(functions);
    for (auto f : p->functions) add_function(f);
    env.add_level();
    for (auto g : p->globals) g->accept(this);
    for (auto s : p->structs) s->accept(this);
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
    if (env.check(v->name)) {
        cerr << "Error: variable '" << v->name << "' ya declarada." << endl;
        exit(0);
    }
    env.add_var(v->name, t);
    if (v->initializer) {
        Type* initType = v->initializer->accept(this);
        if (!t->match(initType)) {
            cerr << "Error: tipos incompatibles en inicialización de '" << v->name << "'." << endl;
            exit(0);
        }
    }
}

void TypeChecker::visit(StructDecl* s) {
    for (auto m : s->members) m->accept(this);
}

void TypeChecker::visit(CompoundStmt* b) {
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
        cerr << "Error: condición de if debe ser bool." << endl;
        exit(0);
    }
    s->then_branch->accept(this);
    if (s->else_branch) s->else_branch->accept(this);
}

void TypeChecker::visit(WhileStmt* s) {
    Type* t = s->condition->accept(this);
    if (!t->match(boolType)) {
        cerr << "Error: condición de while debe ser bool." << endl;
        exit(0);
    }
    s->body->accept(this);
}

void TypeChecker::visit(DoWhileStmt* s) {
    s->body->accept(this);
    Type* t = s->condition->accept(this);
    if (!t->match(boolType)) {
        cerr << "Error: condición de do-while debe ser bool." << endl;
        exit(0);
    }
}

void TypeChecker::visit(ForStmt* s) {
    if (s->init) s->init->accept(this);
    if (s->condition) {
        Type* t = s->condition->accept(this);
        if (!t->match(boolType)) {
            cerr << "Error: condición de for debe ser bool." << endl;
            exit(0);
        }
    }
    s->body->accept(this);
    if (s->increment) s->increment->accept(this);
}

void TypeChecker::visit(SwitchStmt* s) {
    s->expr->accept(this);
    for (auto c : s->cases) c->accept(this);
}

void TypeChecker::visit(CaseClause* s) {
    s->value->accept(this);
    for (auto st : s->body) st->accept(this);
}

void TypeChecker::visit(DefaultClause* s) {
    for (auto st : s->body) st->accept(this);
}

void TypeChecker::visit(BreakStmt* s) {}
void TypeChecker::visit(ContinueStmt* s) {}

void TypeChecker::visit(FreeStmt* s) {
    s->expr->accept(this);
}

void TypeChecker::visit(ReturnStmt* s) {
    if (retornodefuncion->match(voidType) && s->expr) {
        cerr << "Error: función void no debe retornar valor." << endl;
        exit(0);
    }
    if (s->expr) {
        Type* t = s->expr->accept(this);
        if (!t->match(retornodefuncion)) {
            cerr << "Error: tipo de retorno incompatible." << endl;
            exit(0);
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
        case BinaryOp::ADD: case BinaryOp::SUB:
        case BinaryOp::MUL: case BinaryOp::DIV: case BinaryOp::MOD:
        case BinaryOp::POW:
            if ((left->match(intType) || left->match(floatType)) &&
                (right->match(intType) || right->match(floatType))) {
                if (left->match(floatType) || right->match(floatType))
                    return floatType;
                return intType;
            }
            cerr << "Error: operación aritmética requiere int o float." << endl;
            exit(0);

        case BinaryOp::EQ: case BinaryOp::NE:
        case BinaryOp::LT: case BinaryOp::GT:
        case BinaryOp::LE: case BinaryOp::GE:
            if ((left->match(intType) || left->match(floatType)) &&
                (right->match(intType) || right->match(floatType)))
                return boolType;
            cerr << "Error: comparación requiere int o float." << endl;
            exit(0);

        case BinaryOp::LOG_AND: case BinaryOp::LOG_OR:
            if (left->match(boolType) && right->match(boolType))
                return boolType;
            cerr << "Error: operación lógica requiere bool." << endl;
            exit(0);

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
            if (t->match(intType) || t->match(floatType)) return t;
            cerr << "Error: operación unaria requiere int o float." << endl;
            exit(0);
        case UnaryOp::LOG_NOT:
            if (t->match(boolType)) return boolType;
            cerr << "Error: ! requiere bool." << endl;
            exit(0);
        case UnaryOp::ADDR: return new Type(Type::INT); // pointer type placeholder
        case UnaryOp::DEREF: return intType;
    }
    return intType;
}

Type* TypeChecker::visit(AssignmentNode* e) {
    Type* targetType = e->target->accept(this);
    Type* valueType = e->value->accept(this);
    if (!targetType->match(valueType)) {
        cerr << "Error: tipos incompatibles en asignación." << endl;
        exit(0);
    }
    return targetType;
}

Type* TypeChecker::visit(TernaryOpNode* e) {
    Type* cond = e->condition->accept(this);
    if (!cond->match(boolType)) {
        cerr << "Error: condición ternaria debe ser bool." << endl;
        exit(0);
    }
    Type* thenType = e->then_expr->accept(this);
    Type* elseType = e->else_expr->accept(this);
    if (!thenType->match(elseType)) {
        cerr << "Error: tipos incompatibles en expresión ternaria." << endl;
        exit(0);
    }
    return thenType;
}

Type* TypeChecker::visit(MallocNode* e) {
    e->size->accept(this);
    return intType; // returns int* (simplified)
}

Type* TypeChecker::visit(CallNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->callee)) {
        string fname = id->name;
        auto it = functions.find(fname);
        if (it == functions.end()) {
            cerr << "Error: función no declarada '" << fname << "'." << endl;
            exit(0);
        }
        FuncInfo& info = it->second;

        // Built-in functions like print/printf accept any args
        if (fname == "print" || fname == "printf") {
            for (auto a : e->args) a->accept(this);
            return info.returnType;
        }

        if (e->args.size() != info.paramTypes.size()) {
            cerr << "Error: número de argumentos incorrecto en llamada a '" << fname << "'." << endl;
            exit(0);
        }
        for (size_t i = 0; i < e->args.size(); i++) {
            Type* argType = e->args[i]->accept(this);
            if (!argType->match(info.paramTypes[i])) {
                cerr << "Error: tipo de argumento " << (i+1) << " incorrecto en llamada a '" << fname << "'." << endl;
                exit(0);
            }
        }
        return info.returnType;
    }
    return intType;
}

Type* TypeChecker::visit(SubscriptNode* e) { return intType; }
Type* TypeChecker::visit(MemberAccessNode* e) { return intType; }
Type* TypeChecker::visit(ArrowAccessNode* e) { return intType; }

Type* TypeChecker::visit(CastNode* e) {
    return type_from_ast(e->target_type);
}

Type* TypeChecker::visit(SizeOfNode* e) {
    return intType;
}

Type* TypeChecker::visit(IdentifierNode* e) {
    if (!env.check(e->name)) {
        cerr << "Error: variable '" << e->name << "' no declarada." << endl;
        exit(0);
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
Type* TypeChecker::visit(PointerTypeNode* e) { return intType; }
Type* TypeChecker::visit(StructTypeNode* e) { return intType; }
Type* TypeChecker::visit(NamedTypeNode* e) { return intType; }

Type* TypeChecker::visit(LambdaExprNode* e) { return intType; }

Type* TypeChecker::visit(CaptureNode* e) { return intType; }

void TypeChecker::visit(TemplateDecl* d) {
    if (d->func) d->func->accept(this);
    if (d->struct_decl) d->struct_decl->accept(this);
}
