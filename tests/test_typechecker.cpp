#include <iostream>
#include <sstream>
#include <cassert>
#include "scanner.h"
#include "parser.h"
#include "visitor.h"

using namespace std;

#define T_ASSERT(cond) do { if (!(cond)) { cerr << "FAIL: " #cond " (line " << __LINE__ << ")" << endl; exit(1); } } while(0)

int passes = 0;

void test(const string& name) { cout << "  " << name << "... "; }
void pass() { cout << "PASS" << endl; passes++; }

string get_errors(const string& code) {
    stringstream err_stream;
    auto old_cerr = cerr.rdbuf(err_stream.rdbuf());
    auto old_cout = cout.rdbuf(err_stream.rdbuf());

    Scanner sc(code.c_str());
    Parser parser(&sc);
    try {
        Program* p = parser.parseProgram();
        TypeChecker tc;
        bool ok = tc.check(p);
        if (ok) cout << "Revisión exitosa" << endl;
        delete p;
    } catch (const exception&) {}

    cerr.rdbuf(old_cerr);
    cout.rdbuf(old_cout);
    return err_stream.str();
}

bool has_error(const string& code, const string& substr) {
    string errors = get_errors(code);
    return errors.find(substr) != string::npos;
}

bool ok(const string& code) {
    string errors = get_errors(code);
    return errors.find("Revisión exitosa") != string::npos;
}

void test_arithmetic_types() {
    test("arithmetic_types");
    T_ASSERT(ok("void main() { int x; x = 2 + 3; }"));
    T_ASSERT(ok("void main() { float x; x = 2.0 + 3; }"));
    T_ASSERT(ok("void main() { int x; x = 2 * 3; }"));
    T_ASSERT(ok("void main() { int x; x = 2 ** 3; }"));
    T_ASSERT(has_error("void main() { int x; x = true + 3; }", "Error semántico"));
    pass();
}

void test_comparison_types() {
    test("comparison_types");
    T_ASSERT(ok("void main() { bool x; x = 2 < 3; }"));
    T_ASSERT(ok("void main() { bool x; x = 2.0 == 3.0; }"));
    T_ASSERT(ok("void main() { bool x; x = 2 != 3; }"));
    T_ASSERT(has_error("void main() { int x; x = true < false; }", "Error semántico"));
    pass();
}

void test_logical_operators() {
    test("logical_operators");
    T_ASSERT(ok("void main() { bool x; x = true && false; }"));
    T_ASSERT(ok("void main() { bool x; x = true || false; }"));
    T_ASSERT(ok("void main() { bool x; x = !true; }"));
    T_ASSERT(has_error("void main() { bool x; x = 5 && 3; }", "Error semántico"));
    pass();
}

void test_conditions() {
    test("conditions");
    T_ASSERT(ok("void main() { if (true) {} }"));
    T_ASSERT(ok("void main() { while (true) {} }"));
    T_ASSERT(has_error("void main() { if (5) {} }", "Error semántico"));
    T_ASSERT(has_error("void main() { while (3) {} }", "Error semántico"));
    pass();
}

void test_void_variable() {
    test("void_variable");
    T_ASSERT(has_error("void main() { void x; }", "no se puede declarar variable de tipo void"));
    T_ASSERT(has_error("void main(void p) { }", "parámetro no puede"));
    pass();
}

void test_return_type() {
    test("return_type");
    T_ASSERT(ok("void main() { return; }"));
    T_ASSERT(ok("int main() { return 5; }"));
    T_ASSERT(has_error("void main() { return 5; }", "no debe retornar valor"));
    T_ASSERT(has_error("int main() { return; }", "debe retornar"));
    T_ASSERT(has_error("int bad() { int x; x = 5; }", "no garantiza retorno"));
    pass();
}

void test_shadowing() {
    test("shadowing");
    T_ASSERT(ok("void main() { int x; x = 1; { int x; x = 2; } }"));
    T_ASSERT(has_error("void main() { int x; int x; }", "ya declarada en este ámbito"));
    pass();
}

void test_undeclared_variable() {
    test("undeclared_variable");
    T_ASSERT(has_error("void main() { x = 5; }", "variable"));
    pass();
}

void test_function_call() {
    test("function_call");
    T_ASSERT(ok("int f(int a) { return a; } void main() { f(5); }"));
    T_ASSERT(has_error("int f(int a) { return a; } void main() { f(5, 6); }", "argumentos incorrecto"));
    T_ASSERT(has_error("void main() { f(5); }", "función no declarada"));
    pass();
}

void test_break_continue() {
    test("break_continue");
    T_ASSERT(ok("void main() { while (true) { break; } }"));
    T_ASSERT(ok("void main() { while (true) { continue; } }"));
    T_ASSERT(has_error("void main() { break; }", "break fuera de ciclo"));
    T_ASSERT(has_error("void main() { continue; }", "continue fuera de ciclo"));
    pass();
}

void test_pointer_operations() {
    test("pointer_operations");
    T_ASSERT(ok("void main() { int x; int* p; p = &x; *p = 5; }"));
    T_ASSERT(has_error("void main() { int x; *x = 5; }", "operando de * debe ser puntero"));
    pass();
}

void test_struct_types() {
    test("struct_types");
    T_ASSERT(ok("struct Point { int x; int y; }; void main() { struct Point p; p.x = 5; }"));
    T_ASSERT(has_error("struct Point { int x; int y; }; void main() { struct Point p; p.z = 5; }", "no tiene miembro"));
    T_ASSERT(has_error("void main() { int x; x.y = 5; }", "no struct"));
    pass();
}

void test_array_types() {
    test("array_types");
    T_ASSERT(ok("void main() { int arr[5]; arr[0] = 1; }"));
    T_ASSERT(has_error("void main() { int x; x[0] = 1; }", "requiere arreglo o puntero"));
    pass();
}

void test_template_struct() {
    test("template_struct");
    T_ASSERT(ok("template<typename T> struct Box { T val; }; void main() { Box<int> b; b.val = 5; }"));
    T_ASSERT(has_error("template<typename T> struct Box { T val; }; void main() { Box<int> b; b.xxx = 5; }", "no tiene miembro"));
    pass();
}

void test_promotion() {
    test("promotion");
    T_ASSERT(ok("void main() { int x; float y; y = x; }"));
    T_ASSERT(ok("void main() { int x; float y; y = x + 1.0; }"));
    T_ASSERT(has_error("void main() { bool x; x = 5; }", "Error semántico"));
    pass();
}

void test_location_in_errors() {
    test("location_in_errors");
    T_ASSERT(has_error("int main() { return; }", "line 1:14"));
    T_ASSERT(has_error("void main() { break; }", "line 1:15"));
    pass();
}

int main() {
    test_arithmetic_types();
    test_comparison_types();
    test_logical_operators();
    test_conditions();
    test_void_variable();
    test_return_type();
    test_shadowing();
    test_undeclared_variable();
    test_function_call();
    test_break_continue();
    test_pointer_operations();
    test_struct_types();
    test_array_types();
    test_template_struct();
    test_promotion();
    test_location_in_errors();
    cout << "\n=== TypeChecker: " << passes << " passed ===" << endl;
    return 0;
}
