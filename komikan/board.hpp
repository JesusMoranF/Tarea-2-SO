#pragma once
#include <vector>
using namespace std;

const int num_nodos = 28;

struct Salto {int origen, medio, destino; };   // medio es la casilla que se salta (donde esta la ficha capturada)

class Tablero{
public:
    Tablero();
    const vector<int>& vecinos(int nodo) const {return vecinos_[nodo]; }
    const vector<Salto>& saltos(int nodo) const {return saltos_[nodo];}
    bool son_adyacentes(int a, int b) const;
    int destino_salto(int origen, int medio) const;    // -1 si no hay salto
    int num_aristas() const;

private:
    void agregar_linea(const vector<int>& linea);
    vector<int> vecinos_[num_nodos];
    vector<Salto> saltos_[num_nodos];
};
