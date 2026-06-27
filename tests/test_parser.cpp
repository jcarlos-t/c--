#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include "scanner.h"
#include "parser.h"
#include "ast.h"

using namespace std;

#define T_ASSERT(cond) do { if (!(cond)) { cerr << "FAIL: " #cond " (line " << __LINE__ << ")" << endl; exit(1); } } while(0)

int passes = 0, fails = 0;

void test(const string& name) { cout << "  " << name << "... "; }
void pass() { cout << "PASS" << endl; passes++; }

Program* parse_safe(const string& code) {
    Scanner sc(code.c_str());
    Parser parser(&sc);
    try {
        Program* p = parser.parseProgram();
        return p;
    } catch (const exception& e) {
        cerr << "PARSE ERROR: " << e.what() << endl;
        return nullptr;
    }
}

void test_pow_operator() {
    test("pow_operator");
    Program* p = parse_safe("void main() { int x; x = 2 ** 3; }");
    T_ASSERT(p != nullptr);
    T_ASSERT(p->functions.size() == 1);
    FunDecl* f = p->functions[0];
    Body* b = f->body;
    T_ASSERT(b != nullptr);
    T_ASSERT(b->stmts.size() == 2);
    ExprStmtNode* es = dynamic_cast<ExprStmtNode*>(b->stmts[1]);
    T_ASSERT(es != nullptr);
    AssignmentNode* an = dynamic_cast<AssignmentNode*>(es->expr);
    T_ASSERT(an != nullptr);
    BinaryOpNode* bn = dynamic_cast<BinaryOpNode*>(an->value);
    T_ASSERT(bn != nullptr);
    T_ASSERT(bn->op == BinaryOp::POW);
    delete p;
    pass();
}

void test_right_assoc_pow() {
    test("right_assoc_pow");
    Program* p = parse_safe("void main() { int x; x = 2 ** 3 ** 2; }");
    T_ASSERT(p != nullptr);
    FunDecl* f = p->functions[0];
    Body* b = f->body;
    ExprStmtNode* es = dynamic_cast<ExprStmtNode*>(b->stmts[1]);
    AssignmentNode* an = dynamic_cast<AssignmentNode*>(es->expr);
    BinaryOpNode* bn = dynamic_cast<BinaryOpNode*>(an->value);
    T_ASSERT(bn != nullptr);
    T_ASSERT(bn->op == BinaryOp::POW);
    // Right-assoc: 2 ** (3 ** 2) -> left is 2, right is POW(3,2)
    IntegerLiteralNode* left = dynamic_cast<IntegerLiteralNode*>(bn->left);
    T_ASSERT(left != nullptr);
    T_ASSERT(left->value == 2);
    BinaryOpNode* right = dynamic_cast<BinaryOpNode*>(bn->right);
    T_ASSERT(right != nullptr);
    T_ASSERT(right->op == BinaryOp::POW);
    delete p;
    pass();
}

void test_template_struct_parse() {
    test("template_struct_parse");
    Program* p = parse_safe(
        "template<typename T>\n"
        "struct Pair {\n"
        "  T first;\n"
        "  T second;\n"
        "};\n");
    T_ASSERT(p != nullptr);
    T_ASSERT(p->templates.size() == 1);
    TemplateDecl* td = p->templates[0];
    T_ASSERT(!td->is_function);
    T_ASSERT(td->params.size() == 1);
    T_ASSERT(td->params[0]->name == "T");
    T_ASSERT(td->struct_decl != nullptr);
    T_ASSERT(td->struct_decl->name == "Pair");
    T_ASSERT(td->struct_decl->members.size() == 2);
    delete p;
    pass();
}

void test_template_type_instantiation() {
    test("template_type_instantiation");
    Program* p = parse_safe(
        "template<typename T>\n"
        "struct Box { T value; };\n"
        "void main() { Box<int> b; }\n");
    T_ASSERT(p != nullptr);
    T_ASSERT(p->templates.size() == 1);
    T_ASSERT(p->functions.size() == 1);
    Body* b = p->functions[0]->body;
    T_ASSERT(b != nullptr);
    T_ASSERT(b->stmts.size() == 1);
    DeclStmt* ds = dynamic_cast<DeclStmt*>(b->stmts[0]);
    T_ASSERT(ds != nullptr);
    TemplateTypeNode* tt = dynamic_cast<TemplateTypeNode*>(ds->decl->type);
    T_ASSERT(tt != nullptr);
    T_ASSERT(tt->name == "Box");
    T_ASSERT(tt->type_args.size() == 1);
    delete p;
    pass();
}

void test_error_with_line_col() {
    test("error_with_line_col");
    Scanner sc2("void main() { int x; x = ; }");
    Parser parser(&sc2);
    bool caught = false;
    try {
        parser.parseProgram();
    } catch (const exception& e) {
        string msg = e.what();
        T_ASSERT(msg.find("line 1:") != string::npos);
        caught = true;
    }
    T_ASSERT(caught);
    pass();
}

void test_all_submitted_inputs() {
    test("all_submitted_inputs");
    const char* files[] = {
        "inputs/input1.cnn", "inputs/input2.cnn", "inputs/input3.cnn",
        "inputs/input4.cnn", "inputs/input5.cnn", "inputs/input6.cnn",
        "inputs/input7.cnn", "inputs/input8.cnn", "inputs/input9.cnn",
        "inputs/input10.cnn", nullptr
    };
    for (int i = 0; files[i]; i++) {
        ifstream infile(files[i]);
        T_ASSERT(infile.is_open());
        string input, line;
        while (getline(infile, line)) input += line + '\n';
        infile.close();

        Scanner sc(input.c_str());
        Parser parser(&sc);
        try {
            Program* p = parser.parseProgram();
            T_ASSERT(p != nullptr);
            delete p;
        } catch (const exception& e) {
            cerr << "  " << files[i] << " FAILED: " << e.what() << endl;
            T_ASSERT(false);
        }
    }
    pass();
}

int main() {
    test_pow_operator();
    test_right_assoc_pow();
    test_template_struct_parse();
    test_template_type_instantiation();
    test_error_with_line_col();
    test_all_submitted_inputs();
    cout << "\n=== Parser: " << passes << " passed, " << fails << " failed ===" << endl;
    return fails > 0 ? 1 : 0;
}
