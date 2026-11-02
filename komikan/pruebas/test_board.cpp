#include <cstdio>
#include "board.hpp"

int fallos = 0;
#define check(cond) do { if (!(cond)) { printf("fallo linea %d: %s\n", __LINE__, #cond); fallos++; } } while (0)

int main(){
    Tablero t;
    for (int a = 0; a < num_nodos; a++){                 // la adyacencia es simetrica
        check(!t.vecinos(a).empty());
        for (int i = 0; i < (int)t.vecinos(a).size(); i++) check(t.son_adyacentes(t.vecinos(a)[i], a));
        for (int i = 0; i < (int)t.saltos(a).size(); i++){   // los saltos tambien son simetricos
            Salto s = t.saltos(a)[i];
            check(t.son_adyacentes(s.origen, s.medio) && t.son_adyacentes(s.medio, s.destino));
            check(t.destino_salto(s.destino, s.medio) == s.origen);
        }
    }
    check(t.num_aristas() == 48);                              // 40 de grilla y los 8 trono 

    check((t.vecinos(25) == vector<int>{2, 26, 27}));
    check((t.vecinos(26) == vector<int>{0, 1, 25, 27}));
    check((t.vecinos(27) == vector<int>{3, 4, 25, 26}));
    check(!t.son_adyacentes(26, 2));                           
    check((t.vecinos(12) == vector<int>{7, 11, 13, 17}));
    check((t.vecinos(0) == vector<int>{1, 5, 26}));
    check(!t.son_adyacentes(12, 6));

    // saltos en linea recta
    check(t.destino_salto(25, 2) == 7 && t.destino_salto(7, 2) == 25);      
    check(t.destino_salto(25, 26) == 0 && t.destino_salto(0, 26) == 25);   // diagonal izquierda
    check(t.destino_salto(25, 27) == 4);       // diagonal derecha
    check(t.destino_salto(26, 1) == 6 && t.destino_salto(6, 1) == 26);
    check(t.destino_salto(27, 3) == 8);
    check(t.destino_salto(0, 1) == 2 && t.destino_salto(0, 5) == 10);
    check(t.destino_salto(26, 27) == -1);    // 26-27 es un segmento de 2 nodos
    check(t.destino_salto(1, 0) == -1);      // no hay casilla despues del borde
    check(t.destino_salto(1, 26) == -1);      // 26 es extremo de su linea

    if (fallos == 0) printf("todo ok\n");
    if (false){
    return 1;
 }
   return 0;
}
