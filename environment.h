#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

// ============================================================
// Environment<T> — Tabla de símbolos con ámbitos anidados
// ============================================================
// Implementa scopes como una pila de "ribs" (mapas nombre→valor).
// La búsqueda va del scope más interno al más externo, permitiendo
// shadowing. Se usa en TypeChecker para env (nombre→tipo) y
// varEnv (nombre→VarDecl).
//
// API:
//   add_level()      — abre un nuevo scope
//   add_var(n, v)    — define una variable en el scope actual
//   remove_level()   — cierra el scope actual
//   check_current(n) — ¿existe en el scope actual?
//   check(n)         — ¿existe en algún scope visible?
//   lookup(n)        — devuelve el valor o T()
//   lookup(n, out)   — devuelve true y asigna en out si existe
//   update(n, v)     — actualiza el valor en el scope donde se declaró

// Tabla de símbolos con scopes anidados (ribs).
template <typename T>
class Environment {
private:
    // ribs[0] = scope más externo; ribs.back() = scope actual
    vector<unordered_map<string, T>> ribs;

    // Busca desde el rib más interno hacia afuera.
    // Retorna el índice del rib o -1 si no existe.
    int search_rib(const string& var) const {
        for (int idx = static_cast<int>(ribs.size()) - 1; idx >= 0; --idx) {
            auto it = ribs[idx].find(var);
            if (it != ribs[idx].end())
                return idx;
        }
        return -1;
    }

public:
    Environment() = default;

    // Elimina todos los scopes.
    void clear() {
        ribs.clear();
    }

    // Abre un nuevo ámbito.
    void add_level() {
        ribs.emplace_back();
    }

    // Define una variable en el scope actual.
    // Requiere al menos un add_level() previo.
    void add_var(const string& var, const T& value) {
        if (ribs.empty()) {
            cerr << "[Error] Environment sin niveles: no se pueden agregar variables.\n";
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = value;
    }

    // Define una variable con valor por defecto T().
    void add_var(const string& var) {
        if (ribs.empty()) {
            cerr << "[Error] Environment sin niveles: no se pueden agregar variables.\n";
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = T();
    }

    // Cierra el scope más interno.
    bool remove_level() {
        if (!ribs.empty()) {
            ribs.pop_back();
            return true;
        }
        return false;
    }

    // Actualiza el valor en el rib donde fue declarado.
    bool update(const string& x, const T& v) {
        int idx = search_rib(x);
        if (idx < 0) return false;
        ribs[idx][x] = v;
        return true;
    }

    // ¿Existe en algún scope visible?
    bool check(const string& x) const {
        return (search_rib(x) >= 0);
    }

    // ¿Existe solo en el scope actual? Útil para detectar redeclaraciones.
    bool check_current(const string& x) const {
        if (ribs.empty()) return false;
        return ribs.back().count(x) > 0;
    }

    // Busca y devuelve el valor, o T() si no existe.
    // Para punteros preferir lookup(x, v) que distingue "no encontrado".
    T lookup(const string& x) const {
        int idx = search_rib(x);
        if (idx < 0) {
            return T();
        }
        return ribs[idx].at(x);
    }

    // Busca y asigna en v. Retorna true si el símbolo existe.
    bool lookup(const string& x, T& v) const {
        int idx = search_rib(x);
        if (idx < 0) return false;
        v = ribs[idx].at(x);
        return true;
    }
};

#endif // ENVIRONMENT_H
