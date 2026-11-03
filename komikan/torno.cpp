#include <cstdio>
#include <cstdlib>
#include <string>
#include "board.hpp"
#include "config.hpp"
using namespace std;

int num_jugadores = 0, max_mesas = 0;
Config cfg;

// lee un entero completo, false si no es un numero
bool leer_entero(const char* texto, long* valor){
    char* resto;
    *valor = strtol(texto, &resto, 10);
    return *texto != '\0' && *resto == '\0';
}

int main(int argc, char* argv[]){
    if (argc != 3 && argc != 4){
        printf("Uso: ./torneo N M [config.txt]\n");
        return 1;
    }
  
    long n, m;
    if (!leer_entero(argv[1], &n) || n <= 0 || n > 100000){
        printf("N debe ser un entero positivo (hasta 100000)\n");
        return 1;
    }
    if (n % 2 != 0){
        printf("N debe ser par, las partidas son entre parejas de jugadores (recibi N=%ld)\n", n);
        return 1;
    }
    if (!leer_entero(argv[2], &m) || m <= 0 || m > 10000){
        printf("M debe ser un entero positivo (hasta 10000)\n");
        return 1;
    }
    num_jugadores = (int)n;
    max_mesas = (int)m;

    string error;
    if ((argc == 4 && !cargar_config(argv[3], cfg, error)) || !validar_config(cfg, num_nodos, error)){
        printf("Config invalida: %s\n", error.c_str());
        return 1;
    }
    printf("Jugadores (N): %d\nMesas maximas (M): %d\nRetardo: %d a %d ms\nSemilla: %u\n",
           num_jugadores, max_mesas, cfg.min_ms, cfg.max_ms, cfg.seed);
    return 0;
}
