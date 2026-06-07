import os
import subprocess
import shutil
import sys

# Archivos c++
programa = ["main.cpp", "scanner.cpp", "token.cpp", "parser.cpp", "ast.cpp", "visitor.cpp", "semantic_types.h",
            "TypeChecker.cpp"]

# Compilar
compile = ["g++", "-std=c++17"] + programa + ["-o", "c--"]
print("Compilando:", " ".join(compile))
result = subprocess.run(compile, capture_output=True, text=True)

if result.returncode != 0:
    print("Error en compilación:\n", result.stderr)
    exit(1)

print("Compilación exitosa")

# Ejecutar
input_dir = "inputs"
output_dir = "outputs"
os.makedirs(output_dir, exist_ok=True)

# Si pasan un archivo como argumento, ejecutar solo ese
if len(sys.argv) > 1:
    filename = sys.argv[1]
    if not filename.endswith(".cnn"):
        filename += ".cnn"
    filepath = os.path.join(input_dir, filename) if not os.path.dirname(filename) else filename
    input_files = [os.path.basename(filepath)]
    single_mode = True
else:
    input_files = sorted([f for f in os.listdir(input_dir) if f.endswith(".cnn")])
    single_mode = False

for filename in input_files:
    if single_mode:
        filepath = filename if os.path.dirname(filename) else os.path.join(input_dir, filename)
    else:
        filepath = os.path.join(input_dir, filename)
    print(f"Ejecutando {filename}")
    run_cmd = ["./c--", filepath]
    result = subprocess.run(run_cmd, capture_output=True, text=True)

    base = filename.replace(".cnn", "").replace("input", "")
    output_file = os.path.join(output_dir, f"output{base}.txt")
    with open(output_file, "w", encoding="utf-8") as f:
        f.write("=== STDOUT ===\n")
        f.write(result.stdout)
        f.write("\n=== STDERR ===\n")
        f.write(result.stderr)

    source_name = os.path.splitext(os.path.basename(filepath))[0]
    tokens_file = os.path.join(input_dir, f"{source_name}_tokens.txt")
    ast_file = "ast.dot"

    if os.path.isfile(tokens_file):
        dest_tokens = os.path.join(output_dir, f"tokens_{base}.txt")
        shutil.move(tokens_file, dest_tokens)

    if os.path.isfile(ast_file):
        dest_ast = os.path.join(output_dir, f"ast_{base}.dot")
        shutil.move(ast_file, dest_ast)

        output_img = os.path.join(output_dir, f"ast_{base}.png")
        dot_cmd = ["dot", "-Tpng", dest_ast, "-o", output_img]
        subprocess.run(dot_cmd, capture_output=True, text=True)
