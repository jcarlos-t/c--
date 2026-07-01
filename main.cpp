#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"
#include "ast_printer.h"

using namespace std;

enum Mode { RUN, TOKENS, AST, CODEGEN, ALL };

static string readFile(const string& path) {
    ifstream infile(path);
    if (!infile.is_open()) {
        cerr << "No se pudo abrir el archivo: " << path << endl;
        exit(1);
    }
    string input, line;
    while (getline(infile, line))
        input += line + '\n';
    return input;
}

static Program* parse(const string& input) {
    Scanner scanner(input.c_str());
    Parser parser(&scanner);
    try {
        Program* ast = parser.parseProgram();
        if (!ast) {
            cerr << "Error al parsear" << endl;
            exit(1);
        }
        return ast;
    } catch (const exception& e) {
        cerr << "Error al parsear: " << e.what() << endl;
        exit(1);
    }
}

static void typecheckAndFold(Program* ast) {
    TypeChecker tc;
    tc.typecheck(ast);
    ConstantFolding cf;
    ast->accept(&cf);
}

static string generateAssembly(Program* ast) {
    ostringstream ss;
    GenCodeVisitor codegen(ss);
    codegen.generate(ast);
    return ss.str();
}

static void assemble(const string& asmCode, const string& outputFile) {
    string tmpAsm = outputFile + ".s";
    {
        ofstream out(tmpAsm);
        out << asmCode;
    }
    string cmd = "gcc -no-pie " + tmpAsm + " -o " + outputFile;
    if (system(cmd.c_str()) != 0) {
        cerr << "Error al ensamblar" << endl;
        exit(1);
    }
    remove(tmpAsm.c_str());
}

int main(int argc, const char* argv[]) {
    if (argc < 2 || string(argv[1]) == "--help") {
        cout << "Uso: " << argv[0] << " <archivo.c--> [opciones]\n"
                "Opciones:\n"
                "  -t, --tokens        Imprime los tokens y termina\n"
                "  -a, --ast           Imprime el AST y termina\n"
                "  -c, --codegen       Imprime el assembly x86-64 y termina\n"
                "  -A, --all           Imprime tokens, AST y assembly\n"
                "  -o <archivo>        Guarda texto de salida (sin --exec);\n"
                "                      nombre del binario (con --exec)\n"
                "  -e, --exec          Genera ejecutable a partir del assembly\n"
                "  Sin flags:          Imprime el assembly por stdout" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile;
    Mode mode = RUN;
    bool linkExec = false;

    for (int i = 2; i < argc; i++) {
        string arg(argv[i]);
        if (arg == "-o" && i + 1 < argc)
            outputFile = argv[++i];
        else if (arg == "--exec" || arg == "-e")
            linkExec = true;
        else if (arg == "--tokens" || arg == "-t")
            mode = TOKENS;
        else if (arg == "--ast" || arg == "-a")
            mode = AST;
        else if (arg == "--codegen" || arg == "-c")
            mode = CODEGEN;
        else if (arg == "--all" || arg == "-A")
            mode = ALL;
    }

    string binaryName;
    if (linkExec) {
        if (!outputFile.empty()) {
            binaryName = outputFile;
        } else {
            size_t dot = inputFile.rfind('.');
            binaryName = (dot != string::npos) ? inputFile.substr(0, dot) : inputFile;
            binaryName += ".out";
        }
    }

    ofstream outFile;
    ostream* out = &cout;
    if (!outputFile.empty() && !linkExec) {
        outFile.open(outputFile);
        if (!outFile) {
            cerr << "Error al abrir " << outputFile << endl;
            return 1;
        }
        out = &outFile;
    }

    string input = readFile(inputFile);

    if (mode == TOKENS) {
        Scanner scanner(input.c_str());
        Token* tok;
        while (true) {
            tok = scanner.nextToken();
            *out << *tok << endl;
            bool done = (tok->type == Token::END || tok->type == Token::ERR);
            delete tok;
            if (done) break;
        }
        return 0;
    }

    Program* ast = parse(input);

    if (mode == AST) {
        printAST(ast, *out);
        return 0;
    }

    typecheckAndFold(ast);
    string asmCode = generateAssembly(ast);

    if (mode == CODEGEN || mode == RUN) {
        *out << asmCode;
        outFile.close();
        if (linkExec) assemble(asmCode, binaryName);
        return 0;
    }

    if (mode == ALL) {
        *out << "--- Tokens ---\n";
        {
            Scanner scanner(input.c_str());
            Token* tok;
            while (true) {
                tok = scanner.nextToken();
                *out << *tok << endl;
                bool done = (tok->type == Token::END || tok->type == Token::ERR);
                delete tok;
                if (done) break;
            }
        }
        *out << "--- AST ---\n";
        printAST(ast, *out);
        *out << "--- Assembly ---\n";
        *out << asmCode;
        outFile.close();
        if (linkExec) assemble(asmCode, binaryName);
        return 0;
    }

    return 0;
}
