#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"

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

    Scanner scanner(input.c_str());

    Parser parser(&scanner);
    Program* ast = nullptr;

    try {
        ast = parser.parseProgram();
    } catch (const exception& e) {
        cerr << "Error al parsear: " << e.what() << endl;
        ast = nullptr;
        return 1;
    }

    if (ast) {
        TypeChecker tc;
        tc.typecheck(ast);

        // ConstantFolding cf;
        // cf.visit(ast);

        // Extraer nombre base del archivo de entrada
        string inputFile = argv[1];
        size_t lastSlash = inputFile.find_last_of("/\\");
        string filename = inputFile.substr(lastSlash + 1);
        size_t dotPos = filename.find('.');
        string baseName = filename.substr(0, dotPos);

        // Crear carpeta assembly/ si no existe
        system("mkdir -p assembly");

        // Generar nombre de salida usando el nombre base
        string outputFile = "assembly/" + baseName + ".s";

        ofstream outfile(outputFile);
        GenCodeVisitor codegen(outfile);
        codegen.generate(ast);
        outfile.close();

        EVALVisitor interprete;
        interprete.interprete(ast);
    }

    return 0;
}
