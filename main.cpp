#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"
#include "ast_printer.h"

using namespace std;

enum Mode { RUN, TOKENS, AST, CODEGEN };

static Mode parseMode(int argc, const char* argv[]) {
    for (int i = 2; i < argc; i++) {
        string arg(argv[i]);
        if (arg == "--tokens")  return TOKENS;
        if (arg == "--ast")     return AST;
        if (arg == "--codegen") return CODEGEN;
    }
    return RUN;
}

int main(int argc, const char* argv[]) {
    if (argc < 2 || string(argv[1]) == "--help") {
        cout << "Uso: " << argv[0] << " <archivo.cnn> [opciones]\n"
                "Opciones:\n"
                "  --tokens           Imprime los tokens (scanner)\n"
                "  --ast              Imprime el AST (parser)\n"
                "  --codegen          Imprime el assembly x86-64 generado\n"
                "  -o <archivo.s>     Archivo de salida para el assembly\n"
                "  Sin flags:         Compila y ejecuta el programa" << endl;
        return argv[1] == "--help" ? 0 : 1;
    }

    Mode mode = parseMode(argc, argv);
    string outputFile = "";

    // Flags compatibles con modo RUN
    for (int i = 2; i < argc; i++) {
        string arg(argv[i]);
        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
    }

    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cerr << "No se pudo abrir el archivo: " << argv[1] << endl;
        return 1;
    }

    string input, line;
    while (getline(infile, line)) {
        input += line + '\n';
    }
    infile.close();

    if (mode == TOKENS) {
        Scanner scanner(input.c_str());
        Token* tok;
        while (true) {
            tok = scanner.nextToken();
            cout << *tok << endl;
            bool done = (tok->type == Token::END || tok->type == Token::ERR);
            delete tok;
            if (done) break;
        }
        return 0;
    }

    // Para AST, CODEGEN y RUN: parsear
    Scanner scanner(input.c_str());
    Parser parser(&scanner);
    Program* ast = nullptr;

    try {
        ast = parser.parseProgram();
    } catch (const exception& e) {
        cerr << "Error al parsear: " << e.what() << endl;
        return 1;
    }

    if (!ast) return 1;

    if (mode == AST) {
        printAST(ast, cout);
        return 0;
    }

    // Para CODEGEN y RUN: typecheck + constant folding
    TypeChecker tc;
    tc.typecheck(ast);

    ConstantFolding cf;
    ast->accept(&cf);

    if (mode == CODEGEN) {
        GenCodeVisitor codegen(cout);
        codegen.generate(ast);
        return 0;
    }

    // Modo RUN: comportamiento actual
    if (outputFile.empty()) {
        string inputFile = argv[1];
        size_t lastSlash = inputFile.find_last_of("/\\");
        string filename = inputFile.substr(lastSlash + 1);
        size_t dotPos = filename.find('.');
        string baseName = filename.substr(0, dotPos);

        system("mkdir -p assembly");
        outputFile = "assembly/" + baseName + ".s";
    } else {
        size_t lastSlash = outputFile.find_last_of("/\\");
        if (lastSlash != string::npos) {
            string dir = outputFile.substr(0, lastSlash);
            string cmd = "mkdir -p " + dir;
            system(cmd.c_str());
        }
    }

    {
        ofstream outfile(outputFile);
        GenCodeVisitor codegen(outfile);
        codegen.generate(ast);
    }

    return 0;
}
