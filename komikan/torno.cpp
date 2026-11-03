#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <pthread.h>
#include <unistd.h>
#include "board.hpp"
#include "config.hpp"
#include "mesa.hpp"
using namespace std;

int num_jugadores = 0, max_mesas = 0, max_vivos = 0;
Config cfg;
Tablero tablero;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv_matchmaker = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_main = PTHREAD_COND_INITIALIZER;  // cupo para crear jugadores y fin
deque<int> cola;  // jugadores esperando pareja 
int jugadores_vivos = 0, jueces_vivos = 0;
int mesas_activas = 0;
int resultados[8] = {0}; // partidas terminadas

void dormir_ms(int ms){
    usleep(ms * 1000);
}

void* hilo_jugador(void* arg){
    int id = (int)(intptr_t)arg;
    unsigned int semilla = cfg.seed + 7919u * id + 3u;
    dormir_ms(rand_r(&semilla) % (cfg.llegada_max_ms + 1));  

    Candado g(&mtx);
    cola.push_back(id);
    printf("[jugador %d] llega (en cola: %d)\n", id, (int)cola.size());
    pthread_cond_signal(&cv_matchmaker);
    jugadores_vivos--;
    pthread_cond_broadcast(&cv_main);
    return nullptr;
}

void* hilo_mesa(void* arg){
    Mesa* mesa = static_cast<Mesa*>(arg);
    mesa->ejecutar();   // juega y espera a sus 13 fichas

    Candado g(&mtx);
    int id = mesa->id();
    resultados[(int)mesa->estado_final()]++;
    delete mesa;
    mesas_activas--;
    printf("[mesa %d] cierra\n", id);
    jueces_vivos--;
    pthread_cond_signal(&cv_matchmaker);  // hay una mesa libre
    pthread_cond_broadcast(&cv_main);
    return nullptr;
}

void* hilo_matchmaker(void*){
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int k = 0; k < num_jugadores / 2; k++){
        Candado g(&mtx);
        if (cola.size() < 2 || mesas_activas >= max_mesas)
            pthread_cond_wait(&cv_matchmaker, &mtx);  // espera

        int puma = cola.front(); cola.pop_front();
        int perros = cola.front(); cola.pop_front();
        mesas_activas++;
        jueces_vivos++;

        Mesa* mesa = new Mesa(k, puma, perros, tablero, cfg);
        printf("[mesa %d] abre (puma = jugador %d, perros = jugador %d)\n", k, puma, perros);
        pthread_t juez;
        if (pthread_create(&juez, &attr, hilo_mesa, mesa) != 0){
            fprintf(stderr, "ERROR: no se pudo crear el hilo de la mesa %d\n", k);
            exit(1);
        }
    }
    pthread_attr_destroy(&attr);
    return nullptr;
}

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
        printf("N debe ser par: las partidas son entre parejas de jugadores (recibi N=%ld)\n", n);
        return 1;
    }
    if (!leer_entero(argv[2], &m) || m <= 0 || m > 10000){
        printf("M debe ser un entero positivo (hasta 10000)\n");
        return 1;
    }
    num_jugadores = (int)n;
    max_mesas = (int)m;
    max_vivos = 2 * max_mesas+ 1000;

    string error;
    if ((argc == 4 && !cargar_config(argv[3], cfg, error)) || !validar_config(cfg, num_nodos, error)){
        printf("Config invalida: %s\n", error.c_str());
        return 1;
    }
    printf("Jugadores (N): %d\nMesas maximas (M): %d\nRetardo: %d a %d ms\nSemilla: %u\n",
           num_jugadores, max_mesas, cfg.min_ms, cfg.max_ms, cfg.seed);

    pthread_t matchmaker;
    if (pthread_create(&matchmaker, nullptr, hilo_matchmaker, nullptr) != 0){
        fprintf(stderr, "error, no se pudo crear el hilo del matchmaker\n");
        return 1;
    }

    // un hilo por jugador (stack chico, detached); nunca mas de max_vivos a la vez
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 64*1024);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int i = 0; i < num_jugadores; i++){
        {
            Candado g(&mtx);
            while (jugadores_vivos >= max_vivos) pthread_cond_wait(&cv_main, &mtx);
            jugadores_vivos++;
        }
        pthread_t jugador;
        if (pthread_create(&jugador, &attr, hilo_jugador, (void*)(intptr_t)i) != 0){
            fprintf(stderr, "error, no se pudo crear el hilo del jugador %d\n", i);
            exit(1);
        }
    }
    pthread_attr_destroy(&attr);

    pthread_join(matchmaker, nullptr);
    {
        Candado g(&mtx);   // esperar a que no quede ningun jugador ni juez
        while (jugadores_vivos > 0 || jueces_vivos >0) pthread_cond_wait(&cv_main, &mtx);
    }

    int terminadas = 0;
    for (int i = 0; i < 8; i++) terminadas += resultados[i];
    printf("\nTorneo terminado: %d partidas (puma=%d, perros=%d, abandonos=%d, otras=%d)\n", terminadas, resultados[(int)Estado::gana_puma], resultados[(int)Estado::ganan_perros],
           resultados[(int)Estado::abandono_puma] + resultados[(int)Estado::abandono_perros], resultados[(int)Estado::limite_movs] + resultados[(int)Estado::error_interno]);
    return 0;
}
