#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"
#include "TypeChecker.h"

using namespace std;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cout << "Uso: " << argv[0] << " <archivo.cnn>" << endl;
        return 1;
    }

    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cout << "No se pudo abrir el archivo: " << argv[1] << endl;
        return 1;
    }

    string input, line;
    while (getline(infile, line)) {
        input += line + '\n';
    }
    infile.close();

    Scanner scanner1(input.c_str());
    Scanner scanner2(input.c_str());

    ejecutar_scanner(&scanner1, argv[1]);

    Parser parser(&scanner2);
    Program* ast = nullptr;

    try {
        ast = parser.parseProgram();
    } catch (const exception& e) {
        cerr << "Error al parsear: " << e.what() << endl;
        ast = nullptr;
        return 1;
    }

    if (ast) {
        cout << "\n=== Iniciando verificación de tipos ===\n";
        TypeChecker tc;
        tc.typecheck(ast);

        cout << "\n=== Iniciando ejecución del programa ===\n";
        EVALVisitor interprete;
        interprete.interprete(ast);
    }

    return 0;
}
