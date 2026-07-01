import os
import subprocess
import sys

COMPILER = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "c--"))
INPUT_DIR = os.path.join(os.path.dirname(__file__), "inputs")
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "outputs")

MODES = {
    "1": ("Tokens", "-t"),
    "2": ("AST", "-a"),
    "3": ("Assembly (codegen)", "-c"),
    "4": ("All (tokens + AST + assembly)", "-A"),
    "5": ("Ejecutable (--exec)", "--exec"),
}

def build_compiler():
    print("Compilando c--...")
    project_dir = os.path.join(os.path.dirname(__file__), "..", "..")
    result = subprocess.run(["make", "-C", project_dir, "all"], capture_output=True, text=True)
    if result.returncode != 0:
        print("Error en compilación:")
        print(result.stderr)
        sys.exit(1)
    print("Compilación exitosa\n")

def choose_inputs():
    files = sorted(f for f in os.listdir(INPUT_DIR) if f.endswith(".cnn"))
    print("Archivos disponibles:")
    for i, f in enumerate(files, 1):
        print(f"  {i}. {f}")
    print("  a. Todos")
    choice = input("\nSelecciona archivos (ej: 1,3 o a): ").strip()
    if choice.lower() == "a":
        return files
    selected = []
    for part in choice.split(","):
        part = part.strip()
        if part.isdigit():
            idx = int(part) - 1
            if 0 <= idx < len(files):
                selected.append(files[idx])
    if not selected:
        print("Ningún archivo válido, ejecutando todos.")
        return files
    return selected

def choose_modes():
    print("\nModos de salida:")
    for key, (name, _) in MODES.items():
        print(f"  {key}. {name}")
    choice = input("\nSelecciona modos (ej: 1,3 o a para todos): ").strip()
    modes = []
    if choice.lower() == "a":
        return [flag for _, flag in MODES.values()]
    for part in choice.split(","):
        part = part.strip()
        if part in MODES:
            modes.append(MODES[part][1])
    if not modes:
        print("Ningún modo válido, usando assembly (-c).")
        return ["-c"]
    return modes

def run_compiler(input_file, flags, output_path):
    cmd = [COMPILER, input_file] + flags
    result = subprocess.run(cmd, capture_output=True, text=True)
    with open(output_path, "w", encoding="utf-8") as f:
        if result.stdout:
            f.write(result.stdout)
        if result.stderr:
            if result.stdout:
                f.write("\n")
            f.write(result.stderr)
    return result.returncode

def main():
    if not os.path.isfile(COMPILER):
        build_compiler()

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    input_files = choose_inputs()
    flags = choose_modes()

    mode_suffix = "_".join(f.lstrip("-") for f in flags)
    for filename in input_files:
        filepath = os.path.join(INPUT_DIR, filename)
        base = os.path.splitext(filename)[0]
        out_name = f"{base}_{mode_suffix}.txt"
        out_path = os.path.join(OUTPUT_DIR, out_name)
        print(f"  {filename} -> {out_name}")
        ret = run_compiler(filepath, flags, out_path)
        if ret != 0:
            print(f"    (exit code {ret})")

    print(f"\nResultados guardados en: {OUTPUT_DIR}/")

if __name__ == "__main__":
    main()
