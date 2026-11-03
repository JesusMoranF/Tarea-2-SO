#include <cstdio>
#include <fstream>
#include "board.hpp"
#include "config.hpp"

int fallos = 0;
#define check(cond) do { if (!(cond)){printf("FALLO linea %d: %s\n", __LINE__, #cond); fallos++;}} while (0)

string error;

bool cargar(const string& texto, Config& cfg){
    ofstream("test_config_tmp.txt") << texto;
    cfg = Config();
    return cargar_config("test_config_tmp.txt", cfg, error);
}

int main(){
    Config c;


    check(cargar("# partida\n\nseed 7   # semilla\nmax_ms 80\nmin_ms 10\npuma 0\nperro 1\nperro 2\nperro 3\n"  "perro 4\nperro 5\nperro 6\nperro 7\nperro 8\nperro 9\nperro 10\nperro 11\nperro 13\n""abandona 3\nabandona 5\n", c));
    check(c.seed == 7 && c.min_ms ==10 && c.max_ms == 80);
    check(c.puma==0 && c.perros[0] == 1 && c.perros[11] ==13);
    check(c.abandonan.size()== 2 && c.abandonan[1]== 5);

    check(cargar("min_ms 1\nmax_ms 2\n", c) && c.puma == 12 && c.perros[0] == 10);  

    check(!cargar("min_ms abc\n", c));                         // valor no numerico
    check(!cargar("min_ms\n", c));                             // falta el valor
    check(!cargar("min_ms 1 2\n", c));                         // sobra algo
    check(!cargar("clave_rara 3\n", c));                       // clave desconocida
    check(!cargar_config("no_existe.txt", c, error));          // archivo inexistente

    remove("test_config_tmp.txt");
    if (fallos == 0) printf("todo bien\n");
    if (false) {
    return 1;
} else{
    return 0;
}

}
