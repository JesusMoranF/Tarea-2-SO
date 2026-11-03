#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <memory>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include "board.hpp"
#include "config.hpp"
#include "mesa.hpp"
using namespace std;

struct Jugador{
    pthread_mutex_t m;
    pthread_cond_t cv;
    shared_ptr<Mesa> mesa;
    Rol rol = Rol::puma;
    bool cancelado = false;
};

int num_jugadores = 0, max_mesas = 0, max_vivos = 0;
Config cfg;
Tablero tablero;
vector<Jugador> jugadores;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv_matchmaker = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_main = PTHREAD_COND_INITIALIZER;  // cupo para crear jugadores y fin
deque<int> cola;  // jugadores esperando pareja 
vector<shared_ptr<Mesa>> activas;
int jugadores_vivos = 0, jueces_vivos = 0;
int mesas_activas = 0, max_activas = 0, partidas_iniciadas = 0;
int resultados[8] = {0}; // partidas terminadas
bool detenido = false, error_hilos = false;
sigset_t senales;

void dormir_ms(int ms){
    usleep(ms * 1000);
}

void avisar(Jugador& j, const shared_ptr<Mesa>& mesa, Rol rol){
    Candado g(&j.m);
    if (mesa) { j.mesa = mesa; j.rol = rol; }
    else if (!j.mesa) j.cancelado = true;
    pthread_cond_signal(&j.cv);
}

void detener(){
    detenido = true;
    for (int i = 0; i < (int)activas.size(); i++) activas[i]->solicitar_detencion();
    for (int i = 0; i < num_jugadores; i++) avisar(jugadores[i], nullptr, Rol::puma);
    pthread_cond_broadcast(&cv_matchmaker);
    pthread_cond_broadcast(&cv_main);
}

void* hilo_jugador(void* arg){
    int id = (int)(intptr_t)arg;
    Jugador& yo = jugadores[id];
    unsigned int semilla = cfg.seed + 7919u * id + 3u;
    dormir_ms(rand_r(&semilla) % (cfg.llegada_max_ms + 1));  

    {
        Candado g(&mtx);
        if (!detenido){
            cola.push_back(id);
            pthread_cond_signal(&cv_matchmaker);
        }
    }
    shared_ptr<Mesa> mesa;
    Rol rol = Rol::puma;
    {
        Candado g(&yo.m);
        while (!yo.mesa && !yo.cancelado) pthread_cond_wait(&yo.cv, &yo.m);
        mesa = move(yo.mesa);
        rol = yo.rol;
    }
    if (mesa){
        bool fallara = count(cfg.abandonan.begin(), cfg.abandonan.end(), id) > 0;
        if (fallara && !mesa->esperar_fin(cfg.abandona_ms)){
            printf("[jugador %d] falla y se desconecta\n", id);
            mesa->abandonar(rol);
        }
        mesa->esperar_fin(-1);
    }
    mesa.reset();

    Candado g(&mtx);
    jugadores_vivos--;
    pthread_cond_broadcast(&cv_main);
    return nullptr;
}

void* hilo_mesa(void* arg){
    shared_ptr<Mesa> mesa = *static_cast<shared_ptr<Mesa>*>(arg);
    delete static_cast<shared_ptr<Mesa>*>(arg);
    mesa->ejecutar();   // juega y espera a sus 13 fichas

    Candado g(&mtx);
    int id = mesa->id();
    resultados[(int)mesa->estado_final()]++;
    activas.erase(find(activas.begin(), activas.end(), mesa));
    mesa.reset();
    mesas_activas--;
    printf("mesas activas: %d/%d (cierra la mesa %d)\n", mesas_activas, max_mesas, id);
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
        while (!detenido && (cola.size() < 2 || mesas_activas >= max_mesas))
            pthread_cond_wait(&cv_matchmaker, &mtx);  // espera
        if (detenido) break;

        int puma = cola.front();   cola.pop_front();
        int perros = cola.front(); cola.pop_front();
        mesas_activas++;
        if (mesas_activas > max_mesas){  // nunca deberia pasar
            fprintf(stderr, "error, mesas activas %d > M=%d\n", mesas_activas, max_mesas);
              exit(1);
    }
        if (mesas_activas > max_activas) max_activas = mesas_activas;
        partidas_iniciadas++;

        shared_ptr<Mesa> mesa = make_shared<Mesa>(k, puma, perros, tablero, cfg);
        activas.push_back(mesa);
        jueces_vivos++;
        printf("mesas activas: %d/%d (abre la mesa %d)\n", mesas_activas, max_mesas, k);
        pthread_t juez;
        shared_ptr<Mesa>* arg = new shared_ptr<Mesa>(mesa);
        if (pthread_create(&juez, &attr, hilo_mesa, arg) != 0){
            fprintf(stderr, "error, no se pudo crear el hilo de la mesa %d\n", k);
            delete arg;
            activas.pop_back();
            jueces_vivos--; mesas_activas--; partidas_iniciadas--;
            error_hilos = true;
            detener();
            break;
        }
        avisar(jugadores[puma], mesa, Rol::puma);
        avisar(jugadores[perros], mesa, Rol::perros);
    }
    pthread_attr_destroy(&attr);
    return nullptr;
}

void* hilo_senales(void*){
    for (;;){
        int senal;
        sigwait(&senales, &senal);
        if (senal == SIGUSR1) return nullptr;
        printf("\n[servidor] senal recibida, no entran mas jugadores y las mesas terminan\n");
        Candado g(&mtx);
        detener();
    }
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
        printf("N debe ser par, las partidas son entre parejas de jugadores (recibi N=%ld)\n", n);
        return 1;
    }
    if (!leer_entero(argv[2], &m) || m <= 0 || m > 10000){
        printf("M debe ser un entero positivo (hasta 10000)\n");
        return 1;
    }
    num_jugadores = (int)n;
    max_mesas = (int)m;
    max_vivos = 2 * max_mesas + 1000;

    string error;
    if ((argc == 4 && !cargar_config(argv[3], cfg, error)) || !validar_config(cfg, num_nodos, error)){
        printf("Config invalida: %s\n", error.c_str());
        return 1;
    }
    printf("Jugadores (N): %d\nMesas maximas (M): %d\nRetardo: %d a %d ms\nSemilla: %u\n",
           num_jugadores, max_mesas, cfg.min_ms, cfg.max_ms, cfg.seed);

    jugadores.resize(num_jugadores);
    for (int i = 0; i < num_jugadores; i++){
        pthread_mutex_init(&jugadores[i].m, nullptr);
        pthread_cond_init(&jugadores[i].cv, nullptr);
    }

    sigemptyset(&senales);
    sigaddset(&senales, SIGINT);
    sigaddset(&senales, SIGTERM);
    sigaddset(&senales, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &senales, nullptr);

    pthread_t hilo_sen, matchmaker;
    if (pthread_create(&hilo_sen, nullptr, hilo_senales, nullptr) != 0 ||
        pthread_create(&matchmaker, nullptr, hilo_matchmaker, nullptr) != 0){
        fprintf(stderr, "error, no se pudieron crear los hilos del servidor\n");
        return 1;
    }

    // un hilo por jugador, nunca mas de max_vivos a la vez
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 64*1024);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int i = 0; i <num_jugadores; i++){
        {
            Candado g(&mtx);
            while (!detenido && jugadores_vivos >= max_vivos) pthread_cond_wait(&cv_main, &mtx);
            if (detenido) break;
            jugadores_vivos++;
        }
        pthread_t jugador;
        if (pthread_create(&jugador, &attr, hilo_jugador, (void*)(intptr_t)i) != 0){
            fprintf(stderr, "error, no se pudo crear el hilo del jugador %d\n", i);
            Candado g(&mtx);
            jugadores_vivos--;
            error_hilos = true;
            detener();
            break;
        }
    }
    pthread_attr_destroy(&attr);

    pthread_join(matchmaker, nullptr);
    {
        Candado g(&mtx);   // esperar a que no quede ningun jugador ni juez
        while (jugadores_vivos > 0 || jueces_vivos > 0) pthread_cond_wait(&cv_main, &mtx);
    }

    pthread_kill(hilo_sen, SIGUSR1);
    pthread_join(hilo_sen, nullptr);
    for (int i = 0; i < num_jugadores; i++){
        pthread_cond_destroy(&jugadores[i].cv);
        pthread_mutex_destroy(&jugadores[i].m);
    }

    int terminadas = 0;
    for (int i = 0; i < 8; i++) terminadas += resultados[i];
    const char* estado_torneo = "terminado";
    if (detenido){
        estado_torneo = "interrumpido";
    }
    printf("\nTorneo %s: %d partidas (puma=%d, perros=%d, abandonos=%d, interrumpidas=%d, otras=%d)\n", estado_torneo, terminadas,  resultados[(int)Estado::gana_puma], resultados[(int)Estado::ganan_perros],
           resultados[(int)Estado::abandono_puma] + resultados[(int)Estado::abandono_perros], resultados[(int)Estado::interrumpida], resultados[(int)Estado::limite_movs] + resultados[(int)Estado::error_interno]);
    
    printf("Maximo de mesas simultaneas: %d/%d\n", max_activas, max_mesas);
    if (detenido) printf("Jugadores sin atender: %d\n", num_jugadores - 2 * partidas_iniciadas);
    if (error_hilos){
        fprintf(stderr, "error, el sistema no dejo crear mas hilos; el torneo se corto\n");
        return 1;
    }
    return 0;
}
