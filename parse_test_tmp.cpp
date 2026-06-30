#include <iostream>
#include "scanner.h"
#include "parser.h"
using namespace std;
int main() {
    const char* code = "int main() { int i; while (i < 5) {} }";
    Scanner sc(code);
    Parser p(&sc);
    try {
        auto* prog = p.parseProgram();
        cout << "OK\n";
        delete prog;
    } catch (exception& e) {
        cout << "ERR: " << e.what() << endl;
    }
}
