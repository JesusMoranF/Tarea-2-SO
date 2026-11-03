#include "config.hpp"
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

bool cargar_config(const string& ruta, Config& cfg, string& error){
    ifstream archivo(ruta);
    if (!archivo){
        error = "no se pudo abrir '" + ruta + "'";
        return false;
    }
  
    map<string, int*> enteros = {{"min_ms", &cfg.min_ms},{"max_ms", &cfg.max_ms},{"puma", &cfg.puma},{"max_movs", &cfg.max_movs}, {"torpeza_pct", &cfg.torpeza_pct}, {"periodo_tablero", &cfg.periodo_tablero},
    {"llegada_max_ms", &cfg.llegada_max_ms}, {"abandona_ms", &cfg.abandona_ms}};
    string linea;
    int nlinea = 0, n_perros = 0;
    bool tiene_puma = false;
    cfg.abandonan.clear();

    while (getline(archivo, linea)){
        nlinea++;
        istringstream ss(linea.substr(0, linea.find('#')));   
        string clave, valor, sobra;
        if (!(ss >> clave)) continue;                
        string donde = ruta + ", linea " + to_string(nlinea) + ": ";
        if (!(ss >> valor) || (ss >> sobra)){
            error = donde + "se esperaba 'clave valor'";
            return false;
        }
      
        char* fin;
        long v = strtol(valor.c_str(), &fin, 10);
        if (*fin != '\0' || v < -1000000000L || v > 4294967195L){ //limites bastante "flexibles"
            error = donde + "'" + valor + "' no es un numero valido";
            return false;
        }

        if (clave == "puma") tiene_puma = true;
        if (enteros.count(clave)) *enteros[clave] = (int)v;
        else if (clave == "seed" && v >= 0) cfg.seed = (unsigned int)v;
        else if (clave == "perro" && n_perros < 12) cfg.perros[n_perros++] = (int)v;
        else if (clave == "abandona") cfg.abandonan.push_back((int)v);
        else {
            error = donde + "no se puede usar '" + clave + "' (clave desconocida, de mas o fuera de rango)";
            return false;
        }
    }
        // la disposicion es todo o nada: o no se da, o se da completa
    if ((tiene_puma || n_perros > 0) && (!tiene_puma || n_perros != 12)) {
        error = ruta + ": la disposicion inicial necesita 'puma' y exactamente 12 lineas 'perro' (hay " + to_string(n_perros) + ")";
        return false;
    }
    return true;
}

bool validar_config(const Config& cfg, int nodos, string& error){
    error.clear();

    if (cfg.min_ms < 0 || cfg.max_ms < cfg.min_ms)
        error = "retardos invalidos: se requiere 0 <= min_ms <= max_ms";
        
    else if (cfg.torpeza_pct < 0 || cfg.torpeza_pct > 100)
        error = "torpeza_pct debe estar entre 0 y 100";
    else if (cfg.max_movs < 0 || cfg.periodo_tablero < 1 || cfg.abandona_ms < 0 || (unsigned)cfg.llegada_max_ms > 1000)
        error = "max_movs y abandona_ms no pueden ser negativos, periodo_tablero debe ser >= 1 y llegada_max_ms debe estar entre 0 y 1000";
        
    for (int i = 0; i < (int)cfg.abandonan.size() && error.empty(); i++)
        if (cfg.abandonan[i] < 0) error = "abandona: el id de un jugador no puede ser negativo";

    int fichas[13] = {cfg.puma};     // el puma y los 12 perros
    for (int i = 0; i < 12; i++) fichas[i + 1] = cfg.perros[i];
    for (int i = 0; i < 13 && error.empty(); i++){
        
        if (fichas[i] < 0 || fichas[i] >= nodos)
            error = "una ficha esta en el nodo " + to_string(fichas[i]) + ", fuera del tablero (0 a " + to_string(nodos - 1) + ")";
        
        for (int j = 0; j < i && error.empty(); j++)
            if (fichas[i] == fichas[j]) error = "hay dos fichas en el nodo " + to_string(fichas[i]);
    }
    return error.empty();
}
