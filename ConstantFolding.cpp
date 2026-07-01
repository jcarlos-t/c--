#include "visitor.h"
#include <cmath>
#include <iostream>

using namespace std;

// ============================================================
// ConstantFolding - Evaluación de expresiones constantes
// ============================================================

void ConstantFolding::visit(IntegerLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = e->value;
}

void ConstantFolding::visit(FloatLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = e->value;
}

void ConstantFolding::visit(BoolLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = e->value ? 1.0 : 0.0;
}

void ConstantFolding::visit(CharLiteralNode* e) {
    e->isConstant = true;
    e->constantValue = (double)e->value;
}

void ConstantFolding::visit(StringLiteralNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(IdentifierNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(BinaryOpNode* e) {
    e->left->accept(this);
    e->right->accept(this);
    
    if (e->left->isConstant && e->right->isConstant) {
        e->isConstant = true;
        double left = e->left->constantValue;
        double right = e->right->constantValue;
        
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
                } else {
                    e->constantValue = left / right;
                }
                break;
            case BinaryOp::MOD:
                if (right == 0) {
                    cerr << "Error: módulo por cero" << endl;
                    e->isConstant = false;
                } else {
                    e->constantValue = (long long)left % (long long)right;
                }
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
                break;
        }
    } else {
        e->isConstant = false;
    }
}

void ConstantFolding::visit(UnaryOpNode* e) {
    e->operand->accept(this);
    
    if (e->operand->isConstant) {
        e->isConstant = true;
        double operand = e->operand->constantValue;
        
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
                break;
            default:
                e->isConstant = false;
                break;
        }
    } else {
        e->isConstant = false;
    }
}

void ConstantFolding::visit(AssignmentNode* e) {
    e->target->accept(this);
    e->value->accept(this);
    e->isConstant = false;
}

void ConstantFolding::visit(FcallNode* e) {
    e->callee->accept(this);
    for (auto arg : e->args) {
        arg->accept(this);
    }
    e->isConstant = false;
}

void ConstantFolding::visit(MallocNode* e) {
    e->size->accept(this);
    e->isConstant = false;
}

void ConstantFolding::visit(IndexNode* e) {
    e->base->accept(this);
    e->index->accept(this);
    e->isConstant = false;
}

void ConstantFolding::visit(MemberAccessNode* e) {
    e->object->accept(this);
    e->isConstant = false;
}

void ConstantFolding::visit(ArrowAccessNode* e) {
    e->pointer->accept(this);
    e->isConstant = false;
}

void ConstantFolding::visit(SizeOfNode* e) {
    e->isConstant = true;
    e->constantValue = 0.0;
}

void ConstantFolding::visit(LambdaExprNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(CaptureNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(PrintfNode* e) {
    for (auto arg : e->args) {
        arg->accept(this);
    }
    e->isConstant = false;
}

void ConstantFolding::visit(PrimitiveTypeNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(PointerTypeNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(StructTypeNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(NamedTypeNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(TemplateTypeNode* e) {
    e->isConstant = false;
}

void ConstantFolding::visit(Body* s) {
    for (auto stmt : s->stmts) {
        stmt->accept(this);
    }
}

void ConstantFolding::visit(ExprStmtNode* s) {
    if (s->expr) {
        s->expr->accept(this);
    }
}

void ConstantFolding::visit(IfStmt* s) {
    s->condition->accept(this);
    s->then_branch->accept(this);
    if (s->else_branch) {
        s->else_branch->accept(this);
    }
}

void ConstantFolding::visit(WhileStmt* s) {
    s->condition->accept(this);
    s->body->accept(this);
}

void ConstantFolding::visit(DoWhileStmt* s) {
    s->body->accept(this);
    s->condition->accept(this);
}

void ConstantFolding::visit(ForStmt* s) {
    if (s->init) s->init->accept(this);
    if (s->condition) s->condition->accept(this);
    s->body->accept(this);
    if (s->increment) s->increment->accept(this);
}

void ConstantFolding::visit(SwitchStmt* s) {
    s->expr->accept(this);
    for (auto c : s->cases) {
        c->accept(this);
    }
    for (auto stmt : s->default_body) {
        stmt->accept(this);
    }
}

void ConstantFolding::visit(CaseClause* s) {
    s->value->accept(this);
    for (auto stmt : s->body) {
        stmt->accept(this);
    }
}

void ConstantFolding::visit(BreakStmt*) {
}

void ConstantFolding::visit(ContinueStmt*) {
}

void ConstantFolding::visit(ReturnStmt* s) {
    if (s->expr) {
        s->expr->accept(this);
    }
}

void ConstantFolding::visit(FreeStmt* s) {
    s->expr->accept(this);
}

void ConstantFolding::visit(VarDecl* d) {
    if (d->initializer) {
        d->initializer->accept(this);
    }
}

void ConstantFolding::visit(FunDecl* d) {
    for (auto param : d->params) {
        param->accept(this);
    }
    d->body->accept(this);
}

void ConstantFolding::visit(StructDecl*) {
}

void ConstantFolding::visit(TemplateDecl*) {
}

void ConstantFolding::visit(Program* p) {
    for (auto func : p->functions) {
        func->accept(this);
    }
    for (auto global : p->globals) {
        global->accept(this);
    }
}
