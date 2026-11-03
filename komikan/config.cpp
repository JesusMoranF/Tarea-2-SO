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
    cfg.abandonan.clear();

    while (getline(archivo, linea)){
        nlinea++;
        istringstream ss(linea);
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
      
        if (enteros.count(clave)) *enteros[clave] = (int)v;
        else if (clave == "seed" && v >= 0) cfg.seed = (unsigned int)v;
        else if (clave == "perro" && n_perros < 12) cfg.perros[n_perros++] = (int)v;
        else if (clave == "abandona") cfg.abandonan.push_back((int)v);
        else {
            error = donde + "no se puede usar '" + clave + "' (clave desconocida, de mas o fuera de rango)";
            return false;
        }
    }
    return true;
}
