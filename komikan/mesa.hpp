#pragma once
#include <array>
#include <deque>
#include <pthread.h>
#include <string>
#include "board.hpp"
#include "config.hpp"
using namespace std;

enum class Rol {puma, perros};

enum class Estado{
    en_curso, 
    gana_puma, 
    ganan_perros,
    abandono_puma,      
    abandono_perros,  
    interrumpida,       // el servidor pidio terminar (SIGINT)
    limite_movs,        // empate
    error_interno
};
const char* nombre_estado(Estado e);

//el mutex se suelta en cualquier camino de salida.
class Candado{
public:
    explicit Candado(pthread_mutex_t* m) : m_(m) {pthread_mutex_lock(m_);}
    ~Candado() {pthread_mutex_unlock(m_);}
    Candado(const Candado&) = delete;
    Candado& operator=(const Candado&) = delete;
private:
    pthread_mutex_t* m_;
};

struct PruebasMesa;   

class Mesa{
    friend struct PruebasMesa;
public:
    Mesa(int id, int jugador_puma, int jugador_perros, const Tablero& tablero, const Config& cfg);
    ~Mesa();
    Mesa(const Mesa&) = delete;
    Mesa& operator=(const Mesa&) = delete;

    void ejecutar();     // el hilo Juez lanza las 13 fichas, arbitra y espera a todas
    void abandonar(Rol quien);   // el jugador de ese rol fallo / se desconecto
    void solicitar_detencion();  // SIGINT
    bool esperar_fin(int timeout_ms);  // espera que ejecutar() termine 

    int id() const {return id_;}
    Estado estado_final() const { return estado_;}    
    int capturas() const {return capturas_;}
    int movimientos() const { return movs_;}

private:
    struct Jugada { int ficha, desde, hasta, medio; };  
    struct ArgsFicha { Mesa* mesa; int ficha; unsigned int semilla; };

    static void* hilo_ficha(void* arg);
    void ciclo_ficha(int ficha, unsigned int semilla);

    vector<int> vecinos_libres(int nodo) const;
    bool puede_capturar(const Salto& s) const;
    bool es_seguro(int destino, int desde_perro) const;
    bool elegir_jugada(int ficha, unsigned int* semilla, Jugada* out) const;
    bool validar(const Jugada& j) const;
    void aplicar(const Jugada& j);
    bool puma_acorralado() const;
    bool invariantes_ok() const;
    void evaluar_victoria();
    void terminar(Estado e);
    void evento(const string& texto);
    string mensaje_final(Estado e) const;
    string dibujar_tablero() const;
    void emitir(const string& s) const;

    const int id_, jugador_puma_, jugador_perros_;
    const Tablero& tab_;
    const Config cfg_;

    pthread_mutex_t mtx_;
    pthread_cond_t cv_juez_, cv_fichas_, cv_fin_;


    vector<int> celda_;               // -1 libre, 0 puma, 1..12 perro
    array<int, 13> pos_{};            // nodo de cada ficha
    array<bool, 13> vivo_{};
    int capturas_ = 0, movs_ = 0, movs_sin_dibujar_ = 0;
    Estado estado_ = Estado::en_curso;
    int abandono_ = -1;               // -1 nadie, 0 puma, 1 perros
    bool detener_ = false;
    bool finalizada_ = false;         // ejecutar() ya termino (implica que ya estan todas las fichas unidas)
    deque<string> eventos_;           // cola fichas, lo del juez
};
