#!/usr/bin/env python3
import os
import subprocess
import re
import sys
import shutil

# Directorios
PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FINAL_TESTS_DIR = os.path.join(PROJECT_DIR, "final_tests")
ASSEMBLY_DIR = os.path.join(PROJECT_DIR, "assembly")
BUILD_DIR = os.path.join(PROJECT_DIR, "final_tests", "build")

# Colores para salida
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
RESET = "\033[0m"
BLUE = "\033[94m"


def get_expected_results(test_file):
    """Extrae los resultados esperados desde los comentarios del archivo de test"""
    expected = []
    with open(test_file, "r") as f:
        for line in f:
            match = re.search(r"esperado:?\s*([0-9.]+)", line)
            if match:
                expected.append(match.group(1))
    return expected


def run_test(test_file):
    """Ejecuta un test individual usando el código generado por GenCode"""
    test_name = os.path.basename(test_file)
    base_name = os.path.splitext(test_name)[0]
    
    print(f"\n{YELLOW}=== Test: {test_name} ==={RESET}")
    
    # Obtener resultados esperados
    expected = get_expected_results(test_file)
    if not expected:
        print(f"{RED}✗ No hay resultados esperados definidos{RESET}")
        return False
    
    # Asegurar que directorios existen
    os.makedirs(ASSEMBLY_DIR, exist_ok=True)
    os.makedirs(BUILD_DIR, exist_ok=True)
    
    try:
        # Paso 1: Compilar el compilador C-- (si no está compilado)
        print(f"  {BLUE}Compilando compilador C--...{RESET}")
        compile_compiler_cmd = [
            "g++", "-o", "compilador",
            "main.cpp", "scanner.cpp", "parser.cpp", "visitor.cpp",
            "ast.cpp", "TypeChecker.cpp", "ConstantFolding.cpp",
            "Gencode.cpp", "token.cpp", "-std=c++17"
        ]
        
        result = subprocess.run(
            compile_compiler_cmd,
            cwd=PROJECT_DIR,
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"{RED}✗ Error al compilar compilador:{RESET}")
            print(result.stderr)
            return False
        
        # Paso 2: Ejecutar compilador C-- para generar ensamblador
        print(f"  {BLUE}Generando código ensamblador...{RESET}")
        
        # Copiar test a inputs/ (porque main busca inputX.cnn)
        # Alternativamente, modificar main, pero mejor copiar temporalmente
        test_input_path = os.path.join(PROJECT_DIR, "inputs", f"{base_name}.cnn")
        shutil.copy(test_file, test_input_path)
        
        # Ejecutar compilador
        result = subprocess.run(
            ["./compilador", test_input_path],
            cwd=PROJECT_DIR,
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"{RED}✗ Error al generar ensamblador:{RESET}")
            print(result.stderr)
            return False
        
        # Paso 3: Compilar el ensamblador con gcc
        print(f"  {BLUE}Compilando ensamblador...{RESET}")
        
        # El ensamblador se genera con el mismo nombre base
        assembly_path = os.path.join(ASSEMBLY_DIR, f"{base_name}.s")
        executable_path = os.path.join(BUILD_DIR, base_name)
        
        compile_assembly_cmd = [
            "gcc", "-o", executable_path, assembly_path
        ]
        
        result = subprocess.run(
            compile_assembly_cmd,
            cwd=PROJECT_DIR,
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"{RED}✗ Error al compilar ensamblador:{RESET}")
            print(result.stderr)
            return False
        
        # Paso 4: Ejecutar el programa compilado
        print(f"  {BLUE}Ejecutando programa compilado...{RESET}")
        
        result = subprocess.run(
            [executable_path],
            cwd=PROJECT_DIR,
            capture_output=True,
            text=True
        )
        
        # Obtener la salida
        actual_output = []
        for line in result.stdout.splitlines():
            stripped = line.strip()
            if stripped:
                actual_output.append(stripped)
        
        print(f"    Salida:   {actual_output}")
        print(f"    Esperado: {expected}")
        
        # Comparar resultados
        passed = True
        if len(actual_output) != len(expected):
            print(f"{RED}✗ Cantidad de resultados incorrecta{RESET}")
            passed = False
        else:
            for actual, exp in zip(actual_output, expected):
                try:
                    actual_num = float(actual)
                    exp_num = float(exp)
                    if abs(actual_num - exp_num) > 0.001:
                        print(f"{RED}✗ {actual} != {exp}{RESET}")
                        passed = False
                except:
                    print(f"{RED}✗ Error al parsear números{RESET}")
                    passed = False
        
        if passed:
            print(f"{GREEN}✓ Test {test_name} pasado{RESET}")
            return True
        else:
            print(f"{RED}✗ Test {test_name} fallado{RESET}")
            return False
            
    except Exception as e:
        print(f"{RED}✗ Error al ejecutar test: {e}{RESET}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print(f"{GREEN}=== Iniciando tests finales (usando GenCode) ==={RESET}")
    
    # Obtener todos los archivos de test
    test_files = sorted([
        os.path.join(FINAL_TESTS_DIR, f)
        for f in os.listdir(FINAL_TESTS_DIR)
        if f.startswith("test") and f.endswith(".cnn")
    ])
    
    if not test_files:
        print(f"{RED}No se encontraron archivos de test{RESET}")
        return 1
    
    # Ejecutar tests
    passed = 0
    failed = 0
    
    for test_file in test_files:
        if run_test(test_file):
            passed += 1
        else:
            failed += 1
    
    # Resumen
    print(f"\n{YELLOW}=== Resumen de tests ==={RESET}")
    print(f"Total tests: {len(test_files)}")
    print(f"{GREEN}Pasados: {passed}{RESET}")
    print(f"{RED}Fallados: {failed}{RESET}")
    
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
