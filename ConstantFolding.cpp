#include "visitor.h"
#include <cmath>
#include <iostream>

using namespace std;

// ============================================================
// ConstantFolding - Evaluación de expresiones constantes
// ============================================================

// Literales - siempre son constantes
double ConstantFolding::visit(IntegerLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = e->value;
    return e->value;
}

double ConstantFolding::visit(FloatLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = e->value;
    return e->value;
}

double ConstantFolding::visit(BoolLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = e->value ? 1.0 : 0.0;
    return e->constantValue;
}

double ConstantFolding::visit(CharLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = (double)e->value;
    return e->constantValue;
}

double ConstantFolding::visit(StringLiteralNode* e) {
    e->isConstant = false;
    return 0.0;
}

// Identificadores - no son constantes (no conocemos su valor en compilación)
double ConstantFolding::visit(IdentifierNode* e) {
    e->isConstant = false;
    return 0.0;
}

// Operaciones binarias - constante si ambos operandos lo son
double ConstantFolding::visit(BinaryOpNode* e) {
    double left = e->left->accept(this);
    double right = e->right->accept(this);
    
    if (e->left->isConstant && e->right->isConstant) {
        e->isConstant = true;
        
        switch (e->op) {
            case BinaryOp::ADD:
                e->constantValue = left + right;
                break;
            case BinaryOp::SUB:
                e->constantValue = left - right;
                break;
            case BinaryOp::MUL:
                e->constantValue = left * right;
                break;
            case BinaryOp::DIV:
                if (right == 0) {
                    cerr << "Error: división por cero" << endl;
                    e->isConstant = false;
                    return 0.0;
                }
                e->constantValue = left / right;
                break;
            case BinaryOp::MOD:
                if (right == 0) {
                    cerr << "Error: módulo por cero" << endl;
                    e->isConstant = false;
                    return 0.0;
                }
                e->constantValue = (long long)left % (long long)right;
                break;
            case BinaryOp::POW:
                e->constantValue = pow(left, right);
                break;
            case BinaryOp::EQ:
                e->constantValue = (left == right) ? 1.0 : 0.0;
                break;
            case BinaryOp::NE:
                e->constantValue = (left != right) ? 1.0 : 0.0;
                break;
            case BinaryOp::LT:
                e->constantValue = (left < right) ? 1.0 : 0.0;
                break;
            case BinaryOp::GT:
                e->constantValue = (left > right) ? 1.0 : 0.0;
                break;
            case BinaryOp::LE:
                e->constantValue = (left <= right) ? 1.0 : 0.0;
                break;
            case BinaryOp::GE:
                e->constantValue = (left >= right) ? 1.0 : 0.0;
                break;
            case BinaryOp::LOG_AND:
                e->constantValue = (left != 0 && right != 0) ? 1.0 : 0.0;
                break;
            case BinaryOp::LOG_OR:
                e->constantValue = (left != 0 || right != 0) ? 1.0 : 0.0;
                break;
            default:
                e->isConstant = false;
                return 0.0;
        }
        
        return e->constantValue;
    }
    
    e->isConstant = false;
    return 0.0;
}

// Operaciones unarias - constante si el operando lo es
double ConstantFolding::visit(UnaryOpNode* e) {
    double operand = e->operand->accept(this);
    
    if (e->operand->isConstant) {
        e->isConstant = true;
        
        switch (e->op) {
            case UnaryOp::MINUS:
                e->constantValue = -operand;
                break;
            case UnaryOp::LOG_NOT:
                e->constantValue = (operand == 0) ? 1.0 : 0.0;
                break;
            case UnaryOp::PRE_INC:
            case UnaryOp::POST_INC:
                e->constantValue = operand + 1;
                break;
            case UnaryOp::PRE_DEC:
            case UnaryOp::POST_DEC:
                e->constantValue = operand - 1;
                break;
            case UnaryOp::ADDR:
            case UnaryOp::DEREF:
                e->isConstant = false;
                return 0.0;
            default:
                e->isConstant = false;
                return 0.0;
        }
        
        return e->constantValue;
    }
    
    e->isConstant = false;
    return 0.0;
}

// Asignación - no es constante (modifica estado)
double ConstantFolding::visit(AssignmentNode* e) {
    e->target->accept(this);
    e->value->accept(this);
    e->isConstant = false;
    return 0.0;
}

// Llamadas a funciones - nunca son constantes (pueden tener side effects)
double ConstantFolding::visit(FcallNode* e) {
    e->callee->accept(this);
    for (auto arg : e->args) {
        arg->accept(this);
    }
    e->isConstant = false;
    return 0.0;
}

// Malloc - no es constante
double ConstantFolding::visit(MallocNode* e) {
    e->size->accept(this);
    e->isConstant = false;
    return 0.0;
}

// Index - no es constante (acceso a array)
double ConstantFolding::visit(IndexNode* e) {
    e->base->accept(this);
    e->index->accept(this);
    e->isConstant = false;
    return 0.0;
}

// Acceso a miembros - no es constante
double ConstantFolding::visit(MemberAccessNode* e) {
    e->object->accept(this);
    e->isConstant = false;
    return 0.0;
}

double ConstantFolding::visit(ArrowAccessNode* e) {
    e->pointer->accept(this);
    e->isConstant = false;
    return 0.0;
}

// SizeOf - siempre es constante (se conoce en compilación)
double ConstantFolding::visit(SizeOfNode* e) {
    e->isConstant = true;
    // El valor real se calculará en GenCodeVisitor según el tipo
    e->constantValue = 0.0;
    return 0.0;
}

// Lambda - no es constante
double ConstantFolding::visit(LambdaExprNode* e) {
    e->isConstant = false;
    return 0.0;
}

double ConstantFolding::visit(CaptureNode* e) {
    e->isConstant = false;
    return 0.0;
}

// Printf - no es constante (tiene side effects)
double ConstantFolding::visit(PrintfNode* e) {
    for (auto arg : e->args) {
        arg->accept(this);
    }
    e->isConstant = false;
    return 0.0;
}

// Nodos de tipo - no son expresiones evaluables
double ConstantFolding::visit(PrimitiveTypeNode* e) {
    e->isConstant = false;
    return 0.0;
}

double ConstantFolding::visit(PointerTypeNode* e) {
    e->isConstant = false;
    return 0.0;
}

double ConstantFolding::visit(StructTypeNode* e) {
    e->isConstant = false;
    return 0.0;
}

double ConstantFolding::visit(NamedTypeNode* e) {
    e->isConstant = false;
    return 0.0;
}

double ConstantFolding::visit(TemplateTypeNode* e) {
    e->isConstant = false;
    return 0.0;
}

// ============================================================
// Statements - recorrer recursivamente
// ============================================================

int ConstantFolding::visit(Body* s) {
    for (auto stmt : s->stmts) {
        stmt->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(ExprStmtNode* s) {
    if (s->expr) {
        s->expr->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(IfStmt* s) {
    s->condition->accept(this);
    s->then_branch->accept(this);
    if (s->else_branch) {
        s->else_branch->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(WhileStmt* s) {
    s->condition->accept(this);
    s->body->accept(this);
    return 0;
}

int ConstantFolding::visit(DoWhileStmt* s) {
    s->body->accept(this);
    s->condition->accept(this);
    return 0;
}

int ConstantFolding::visit(ForStmt* s) {
    if (s->init) s->init->accept(this);
    if (s->condition) s->condition->accept(this);
    s->body->accept(this);
    if (s->increment) s->increment->accept(this);
    return 0;
}

int ConstantFolding::visit(SwitchStmt* s) {
    s->expr->accept(this);
    for (auto c : s->cases) {
        c->accept(this);
    }
    for (auto stmt : s->default_body) {
        stmt->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(CaseClause* s) {
    s->value->accept(this);
    for (auto stmt : s->body) {
        stmt->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(BreakStmt* s) {
    return 0;
}

int ConstantFolding::visit(ContinueStmt* s) {
    return 0;
}

int ConstantFolding::visit(ReturnStmt* s) {
    if (s->expr) {
        s->expr->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(FreeStmt* s) {
    s->expr->accept(this);
    return 0;
}

// ============================================================
// Declarations
// ============================================================

int ConstantFolding::visit(VarDecl* d) {
    if (d->initializer) {
        d->initializer->accept(this);
    }
    return 0;
}

int ConstantFolding::visit(FunDecl* d) {
    for (auto param : d->params) {
        param->accept(this);
    }
    d->body->accept(this);
    return 0;
}

int ConstantFolding::visit(StructDecl* d) {
    return 0;
}

int ConstantFolding::visit(TemplateDecl* d) {
    return 0;
}

int ConstantFolding::visit(Program* p) {
    for (auto func : p->functions) {
        func->accept(this);
    }
    for (auto global : p->globals) {
        global->accept(this);
    }
    return 0;
}
