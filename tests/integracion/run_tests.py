#!/usr/bin/env python3
import os
import subprocess
import re
import sys
import tempfile
import shutil

# Directorios
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(os.path.dirname(SCRIPT_DIR))
COMPILER = os.path.join(PROJECT_DIR, "c--")

# Colores para salida
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
RESET = "\033[0m"
BLUE = "\033[94m"


def get_expected_results(test_file):
    expected = []
    with open(test_file, "r") as f:
        for line in f:
            match = re.search(r"esperado:?\s*([0-9.]+)", line)
            if match:
                expected.append(match.group(1))
    return expected


def run_test(test_file, build_dir):
    test_name = os.path.basename(test_file)
    base_name = os.path.splitext(test_name)[0]

    print(f"\n{YELLOW}=== Test: {test_name} ==={RESET}")

    expected = get_expected_results(test_file)
    if not expected:
        print(f"{RED}\u2717 No hay resultados esperados definidos{RESET}")
        return False

    try:
        if not os.path.isfile(COMPILER):
            print(f"  {BLUE}Compilando c--...{RESET}")
            result = subprocess.run(
                ["make", "-C", PROJECT_DIR],
                capture_output=True, text=True
            )
            if result.returncode != 0:
                print(f"{RED}\u2717 Error al compilar:{RESET}")
                print(result.stderr)
                return False

        executable_path = os.path.join(build_dir, base_name)

        print(f"  {BLUE}Generando binario...{RESET}")
        result = subprocess.run(
            [COMPILER, test_file, "--exec", "-o", executable_path],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            print(f"{RED}\u2717 Error al generar binario:{RESET}")
            print(result.stderr)
            return False

        print(f"  {BLUE}Ejecutando programa...{RESET}")
        result = subprocess.run(
            [executable_path],
            capture_output=True, text=True
        )

        actual_output = []
        for line in result.stdout.splitlines():
            stripped = line.strip()
            if stripped:
                actual_output.append(stripped)

        print(f"    Salida:   {actual_output}")
        print(f"    Esperado: {expected}")

        passed = True
        if len(actual_output) != len(expected):
            print(f"{RED}\u2717 Cantidad de resultados incorrecta{RESET}")
            passed = False
        else:
            for actual, exp in zip(actual_output, expected):
                try:
                    actual_num = float(actual)
                    exp_num = float(exp)
                    if abs(actual_num - exp_num) > 0.001:
                        print(f"{RED}\u2717 {actual} != {exp}{RESET}")
                        passed = False
                except:
                    print(f"{RED}\u2717 Error al parsear números{RESET}")
                    passed = False

        if passed:
            print(f"{GREEN}\u2713 Test {test_name} pasado{RESET}")
            return True
        else:
            print(f"{RED}\u2717 Test {test_name} fallado{RESET}")
            return False

    except Exception as e:
        print(f"{RED}\u2717 Error al ejecutar test: {e}{RESET}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print(f"{GREEN}=== Iniciando tests (usando GenCode) ==={RESET}")

    test_files = sorted([
        os.path.join(SCRIPT_DIR, f)
        for f in os.listdir(SCRIPT_DIR)
        if f.startswith("test") and f.endswith(".cnn")
    ])

    if not test_files:
        print(f"{RED}No se encontraron archivos de test{RESET}")
        return 1

    build_dir = tempfile.mkdtemp(prefix="c--_tests_")
    try:
        passed = 0
        failed = 0

        for test_file in test_files:
            if run_test(test_file, build_dir):
                passed += 1
            else:
                failed += 1

        print(f"\n{YELLOW}=== Resumen de tests ==={RESET}")
        print(f"Total tests: {len(test_files)}")
        print(f"{GREEN}Pasados: {passed}{RESET}")
        print(f"{RED}Fallados: {failed}{RESET}")

        return 0 if failed == 0 else 1
    finally:
        shutil.rmtree(build_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
