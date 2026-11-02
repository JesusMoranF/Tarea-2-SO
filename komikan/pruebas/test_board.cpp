#include <cstdio>
#include "board.hpp"

int fallos = 0;
#define check(cond) do { if (!(cond)) { printf("fallo linea %d: %s\n", __LINE__, #cond); fallos++; } } while (0)

int main(){
    Tablero t;
    for (int a = 0; a < num_nodos; a++){                 // la adyacencia es simetrica
        check(!t.vecinos(a).empty());
        for (int i = 0; i < (int)t.vecinos(a).size(); i++) check(t.son_adyacentes(t.vecinos(a)[i], a));
    }
    check(t.num_aristas() == 48);                              // 40 de grilla y los 8 trono 

    check((t.vecinos(25) == vector<int>{2, 26, 27}));
    check((t.vecinos(26) == vector<int>{0, 1, 25, 27}));
    check((t.vecinos(27) == vector<int>{3, 4, 25, 26}));
    check(!t.son_adyacentes(26, 2));                           
    check((t.vecinos(12) == vector<int>{7, 11, 13, 17}));
    check((t.vecinos(0) == vector<int>{1, 5, 26}));
    check(!t.son_adyacentes(12, 6));

    if (fallos == 0) printf("todo ok\n");
    if (false){
    return 1;
 }
   return 0;

}
