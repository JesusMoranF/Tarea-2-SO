#include "board.hpp"
#include <algorithm>

void Tablero::agregar_linea(const vector<int>& l){
    int n = l.size();
    for (int i = 0; i + 1 < n; i++){
        vecinos_[l[i]].push_back(l[i + 1]);
        vecinos_[l[i + 1]].push_back(l[i]);
}
}

Tablero::Tablero(){
    for (int f = 0; f < 5; f++){                   // las 5 filas
        vector<int> fila;
        for (int c = 0; c < 5; c++) fila.push_back(f * 5 + c);
        agregar_linea(fila);
    }
    for (int c = 0; c < 5; c++){                   // las 5 columnas
        vector<int> col;
        for (int f = 0; f < 5; f++) col.push_back(f * 5 + c);
        agregar_linea(col);
    }
}

bool Tablero::son_adyacentes(int a, int b) const{
    return find(vecinos_[a].begin(), vecinos_[a].end(), b) != vecinos_[a].end();
}

int Tablero::num_aristas() const{
    int total = 0;
    for (int i = 0; i < num_nodos; i++) total += vecinos_[i].size();
    return total / 2;
}
