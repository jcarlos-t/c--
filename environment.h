#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

// ============================================================
// Environment<T> — Entorno con anidamiento de ámbitos (scopes)
// ============================================================
// Implementa una tabla de símbolos con soporte para scopes
// anidados. Cada nivel de anidamiento es un "rib" (costilla),
// que es un mapa de nombre → valor.
//
// Estructura interna:
//   ribs = [ {x: 10, y: 20},    ← scope global (nivel 0)
//            {a: 5},            ← scope función (nivel 1)
//            {b: 3} ]           ← scope bloque (nivel 2)
//
// Búsqueda: desde el scope más interno hacia afuera.
//   lookup("x") → busca en nivel 2, luego 1, luego 0 → encuentra 10
//
// Uso en el compilador:
//   - EVALVisitor: Environment<double> para variables → valores
//   - TypeChecker: Environment<Type*> para variables → tipos
//                  Environment<VarDecl*> para variables → declaraciones
//
// Ejemplo típico:
//   Environment<int> env;
//   env.add_level();          // abre scope
//   env.add_var("x", 42);     // define variable
//   int val = env.lookup("x");// obtiene valor
//   env.update("x", 10);      // actualiza
//   env.remove_level();       // cierra scope (descarta x)

template <typename T>
class Environment {
private:
    // Vector de mapas, uno por nivel de scope.
    // ribs[0] = scope más externo (global)
    // ribs[ribs.size()-1] = scope más interno (actual)
    vector<unordered_map<string, T>> ribs;

    // search_rib: busca la variable `var` desde el scope más
    // interno hacia el más externo. Retorna el índice del rib
    // donde se encuentra, o -1 si no existe.
    //
    //   Ej: ribs = [{x:1}, {y:2}], search_rib("x") → 0
    //       ribs = [{x:1}, {x:2}], search_rib("x") → 1 (más interno)
    //       search_rib("z") → -1
    int search_rib(const string& var) const {
        for (int idx = static_cast<int>(ribs.size()) - 1; idx >= 0; --idx) {
            auto it = ribs[idx].find(var);
            if (it != ribs[idx].end())  // encontrado
                return idx;
        }
        return -1; // no encontrado
    }

public:
    Environment() = default;

    // clear: elimina todos los scopes (reinicio completo)
    void clear() {
        ribs.clear();
    }

    // add_level: abre un nuevo scope (nivel de anidamiento).
    // Todas las variables declaradas a partir de ahora se
    // agregarán a este nuevo nivel.
    //
    //   Ej: env.add_level() → ribs = [..., {}]
    void add_level() {
        ribs.emplace_back();
    }

    // add_var: agrega una variable con valor inicial en el
    // scope actual. Si no hay niveles abiertos, error fatal.
    //
    //   Ej: env.add_var("x", 42) → ribs.back()["x"] = 42
    void add_var(const string& var, const T& value) {
        if (ribs.empty()) {
            cerr << "[Error] Environment sin niveles: no se pueden agregar variables.\n";
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = value;
    }

    // add_var: agrega variable con valor por defecto.
    // Solo funciona si T tiene constructor por defecto.
    void add_var(const string& var) {
        if (ribs.empty()) {
            cerr << "[Error] Environment sin niveles: no se pueden agregar variables.\n";
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = T(); // inicializa con valor por defecto
    }

    // remove_level: elimina el scope más interno.
    // Todas las variables declaradas en ese nivel se descartan.
    //
    //   Ej: env.remove_level() → ribs.pop_back()
    //       (se pierden las variables del scope que se cierra)
    bool remove_level() {
        if (!ribs.empty()) {
            ribs.pop_back();
            return true;
        }
        return false;
    }

    // update: modifica el valor de una variable existente en
    // cualquier scope. Busca desde el más interno hacia afuera.
    // Retorna false si la variable no existe.
    //
    //   Ej: env.update("x", 10) → cambia x en el scope donde
    //       fue declarada originalmente
    bool update(const string& x, const T& v) {
        int idx = search_rib(x);
        if (idx < 0) return false;
        ribs[idx][x] = v;
        return true;
    }

    // check: verifica si una variable existe en cualquier scope
    // (búsqueda completa, de adentro hacia afuera).
    //
    //   Ej: env.check("x") → true si x existe en algún nivel
    bool check(const string& x) const {
        return (search_rib(x) >= 0);
    }

    // check_current: verifica si existe en el scope actual
    // (el nivel más interno), sin buscar en scopes superiores.
    //
    //   Ej: env.check_current("x") → true solo si x está en
    //       el scope actual (evita redeclaración)
    bool check_current(const string& x) const {
        if (ribs.empty()) return false;
        return ribs.back().count(x) > 0;
    }

    // lookup: busca una variable y retorna su valor.
    // Si no existe, imprime advertencia y retorna T().
    //
    //   Ej: int val = env.lookup("x") → valor de x
    //       Si no existe: imprime advertencia, retorna 0
    T lookup(const string& x) const {
        int idx = search_rib(x);
        if (idx < 0) {
            cerr << "[Advertencia] Variable no encontrada: " << x << endl;
            return T(); // valor por defecto
        }
        return ribs[idx].at(x);
    }

    // lookup (por referencia): busca una variable y la asigna
    // en v. Retorna true si existe, false si no.
    //
    //   Ej: VarDecl* vd; if (env.lookup("x", vd)) { ... }
    //       → vd recibe el valor y retorna true
    bool lookup(const string& x, T& v) const {
        int idx = search_rib(x);
        if (idx < 0) return false;
        v = ribs[idx].at(x);
        return true;
    }
};

#endif // ENVIRONMENT_H
