#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"

using namespace std;

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        cout << "Uso: " << argv[0] << " <archivo.cnn> [--no-run] [-o output.s]" << endl;
        return 1;
    }

    bool runInterpreter = true;
    string outputFile = "";
    
    for (int i = 2; i < argc; i++) {
        if (string(argv[i]) == "--no-run") {
            runInterpreter = false;
        } else if (string(argv[i]) == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
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

        // Generar nombre de salida
        if (outputFile.empty()) {
            string inputFile = argv[1];
            size_t lastSlash = inputFile.find_last_of("/\\");
            string filename = inputFile.substr(lastSlash + 1);
            size_t dotPos = filename.find('.');
            string baseName = filename.substr(0, dotPos);

            // Crear carpeta assembly/ si no existe
            system("mkdir -p assembly");
            outputFile = "assembly/" + baseName + ".s";
        } else {
            // Crear directorio del archivo de salida si es necesario
            size_t lastSlash = outputFile.find_last_of("/\\");
            if (lastSlash != string::npos) {
                string dir = outputFile.substr(0, lastSlash);
                string cmd = "mkdir -p " + dir;
                system(cmd.c_str());
            }
        }

        ofstream outfile(outputFile);
        GenCodeVisitor codegen(outfile);
        codegen.generate(ast);
        outfile.close();

        if (runInterpreter) {
            EVALVisitor interprete;
            interprete.interprete(ast);
        }
    }

    return 0;
}
