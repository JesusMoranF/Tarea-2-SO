#include "board.hpp"
#include <algorithm>

void Tablero::agregar_linea(const vector<int>& l){
    int n = l.size();
    for (int i = 0; i + 1 < n; i++){
        vecinos_[l[i]].push_back(l[i + 1]);
        vecinos_[l[i + 1]].push_back(l[i]);
}
        for (int i = 0; i + 2 < n; i++)                 // tres consecutivos permiten un salto
        saltos_[l[i]].push_back({l[i], l[i + 1], l[i + 2]});
}

Tablero::Tablero(){
    for (int f = 0; f < 5; f++){                   // las 5 filas
        vector<int> fila;
        for (int c = 0; c < 5; c++) fila.push_back(f * 5 + c);
        agregar_linea(fila);
    }
        int cima[5] = {-1, 26, 25, 27, -1};        // el trono 
    for (int c = 0; c < 5; c++){              // las 5 columnas
        vector<int> col;
        if (cima[c] >= 0) col.push_back(cima[c]);
        for (int f = 0; f < 5; f++) col.push_back(f * 5 + c);
        agregar_linea(col);
    }
    agregar_linea({25, 26, 0});                     // diagonales
    agregar_linea({25, 27, 4});
    agregar_linea({26, 27});  
    for (int i = 0; i < num_nodos; i++) sort(vecinos_[i].begin(), vecinos_[i].end());
}

bool Tablero::son_adyacentes(int a, int b) const{
    return find(vecinos_[a].begin(), vecinos_[a].end(), b) != vecinos_[a].end();
}

int Tablero::destino_salto(int origen, int medio) const{
    for (int i = 0; i < (int)saltos_[origen].size(); i++)
        if (saltos_[origen][i].medio == medio) return saltos_[origen][i].destino;
    return -1;
}

int Tablero::num_aristas() const{
    int total = 0;
    for (int i = 0; i < num_nodos; i++) total += vecinos_[i].size();
    return total / 2;
}
