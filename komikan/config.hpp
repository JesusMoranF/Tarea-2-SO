#pragma once
#include <array>
#include <string>
#include <vector>
using namespace std;

struct Config{

    int min_ms = 50;                // retardo minimo por movimiento
    int max_ms = 300;               // retardo maximo por movimiento
    unsigned int seed = 42;         // semilla de todas las decisiones aleatorias

    int puma = 12;                  
    array<int, 12> perros = {10, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

    int max_movs = 1500;            // limite de movimientos por partida 
    int torpeza_pct = 20;           // % de veces que un perro mueve sin cuidarse
    int periodo_tablero = 10;       // el juez dibuja el tablero cada K movimientos

    int llegada_max_ms = 20;       
    vector<int> abandonan;         
    int abandona_ms = 500;         

    bool silencioso = false;       


};


// Lee un archivo clave valor y devuelve false con un mensaje de 'error' si es invalido.
bool cargar_config(const string& ruta, Config& cfg, string& error);

// revisa que los valores sean coherentes para un tablero de nodos
bool validar_config(const Config& cfg, int nodos, string& error);
