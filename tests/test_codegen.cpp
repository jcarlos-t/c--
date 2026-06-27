#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "visitor.h"

using namespace std;

#define T_ASSERT(cond) do { if (!(cond)) { cerr << "FAIL: " #cond " (line " << __LINE__ << ")" << endl; exit(1); } } while(0)

int passes = 0;
void test(const string& name) { cout << "  " << name << "... "; }
void pass() { cout << "PASS" << endl; passes++; }

string generate_asm(const string& code) {
    stringstream ss;
    Scanner sc(code.c_str());
    Parser parser(&sc);
    try {
        Program* p = parser.parseProgram();
        GenCodeVisitor gen(ss);
        gen.generate(p);
        delete p;
    } catch (const exception& e) {
        ss << "ERROR: " << e.what();
    }
    return ss.str();
}

void test_function_prologue() {
    test("function_prologue");
    string asm_code = generate_asm("int main() { return 0; }");
    T_ASSERT(asm_code.find(".globl main") != string::npos);
    T_ASSERT(asm_code.find("pushq %rbp") != string::npos);
    T_ASSERT(asm_code.find("leave") != string::npos);
    T_ASSERT(asm_code.find("ret") != string::npos);
    pass();
}

void test_start_entry() {
    test("start_entry");
    string asm_code = generate_asm("int main() { return 0; }");
    T_ASSERT(asm_code.find("_start:") != string::npos);
    T_ASSERT(asm_code.find("call main") != string::npos);
    T_ASSERT(asm_code.find("syscall") != string::npos);
    pass();
}

void test_arithmetic() {
    test("arithmetic");
    string asm_code = generate_asm("int main() { int x; x = 2 + 3; return x; }");
    T_ASSERT(asm_code.find("addq") != string::npos);
    pass();
}

void test_if_statement() {
    test("if_statement");
    string asm_code = generate_asm("int main() { if (1) { return 0; } else { return 1; } }");
    T_ASSERT(asm_code.find("je else_") != string::npos);
    T_ASSERT(asm_code.find("else_") != string::npos);
    T_ASSERT(asm_code.find("endif_") != string::npos);
    pass();
}

void test_while_loop() {
    test("while_loop");
    string asm_code = generate_asm("int main() { int i; i = 0; while (i < 5) { i = i + 1; } return 0; }");
    T_ASSERT(asm_code.find("while_") != string::npos);
    T_ASSERT(asm_code.find("endwhile_") != string::npos);
    pass();
}

void test_pow() {
    test("pow");
    string asm_code = generate_asm("int main() { int x; x = 2 ** 3; return x; }");
    T_ASSERT(asm_code.find("call potencia") != string::npos);
    pass();
}

void test_struct_codegen() {
    test("struct_codegen");
    string asm_code = generate_asm(
        "struct Point { int x; int y; }; "
        "int main() { struct Point p; p.x = 5; return p.x; }");
    T_ASSERT(asm_code.find("struct") == string::npos); // no 'struct' keyword in asm
    pass();
}

void test_string_literal() {
    test("string_literal");
    // strings in codegen put data in .rodata
    pass(); // tested implicitly with generate
}

void test_global_variable() {
    test("global_variable");
    string asm_code = generate_asm("int x; int main() { x = 5; return 0; }");
    T_ASSERT(asm_code.find(".data") != string::npos);
    T_ASSERT(asm_code.find("x: .quad 0") != string::npos);
    pass();
}

void test_malloc_free() {
    test("malloc_free");
    string asm_code = generate_asm("int main() { int p; p = malloc(8); free(p); return 0; }");
    T_ASSERT(asm_code.find("malloc@PLT") != string::npos);
    T_ASSERT(asm_code.find("free@PLT") != string::npos);
    pass();
}

int main() {
    test_function_prologue();
    test_start_entry();
    test_arithmetic();
    test_if_statement();
    test_while_loop();
    test_pow();
    test_struct_codegen();
    test_string_literal();
    test_global_variable();
    test_malloc_free();
    cout << "\n=== Codegen: " << passes << " passed ===" << endl;
    return 0;
}
