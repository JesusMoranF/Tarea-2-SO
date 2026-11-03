#include "mesa.hpp"
#include <cerrno>
#include <cstdio>
#include <ctime>

const char* nombre_estado(Estado e){
    const char* nombres[] = {"en_curso", "gana_puma", "ganan_perros", "abandono_puma", "abandono_perros", "interrumpida", "limite_movs", "error_interno"};
       return nombres[(int)e];
}


Mesa::Mesa(int id, int jugador_puma, int jugador_perros, const Tablero& tablero, const Config& cfg)
    : id_(id), jugador_puma_(jugador_puma), jugador_perros_(jugador_perros),
    tab_(tablero), cfg_(cfg), celda_(num_nodos, -1){
    pthread_mutex_init(&mtx_, nullptr);
    pthread_cond_init(&cv_juez_, nullptr);
    pthread_cond_init(&cv_fichas_, nullptr);
    pthread_cond_init(&cv_fin_, nullptr);

    pos_[0] = cfg_.puma;
    for (int i = 0; i < 12; i++) pos_[i + 1] = cfg_.perros[i];
    for (int f = 0; f < 13; f++){
        celda_[pos_[f]] = f;
      vivo_[f] = true;
    }
}

Mesa::~Mesa(){
    pthread_cond_destroy(&cv_fin_);
    pthread_cond_destroy(&cv_fichas_);
    pthread_cond_destroy(&cv_juez_);
    pthread_mutex_destroy(&mtx_);
}

void Mesa::emitir(const string& s) const{
    if (cfg_.silencioso || s.empty()) return;
    fwrite(s.data(), 1, s.size(), stdout);     
}

vector<int> Mesa::vecinos_libres(int nodo) const{
    vector<int> libres;
    for (int i = 0; i < (int)tab_.vecinos(nodo).size(); i++)
        if (celda_[tab_.vecinos(nodo)[i]] == -1) libres.push_back(tab_.vecinos(nodo)[i]);
    return libres;
}

bool Mesa::puede_capturar(const Salto& s) const {          // hay un perro en medio y destino libre
    return celda_[s.medio] > 0 && celda_[s.destino] == -1;
}

bool Mesa::puma_acorralado() const{
    int p = pos_[0];
    if (!vecinos_libres(p).empty()) return false;
    for (int i = 0; i < (int)tab_.saltos(p).size(); i++)
        if (puede_capturar(tab_.saltos(p)[i])) return false;
    return true;
}

// El Juez valida la jugada y debe respetar la linea de salto y casillas.
bool Mesa::validar(const Jugada& j) const{
    if (estado_ != Estado::en_curso || !vivo_[j.ficha]) return false;
    if (pos_[j.ficha] != j.desde || celda_[j.hasta] != -1) return false;     // destino ocupado
    if (j.medio < 0) return tab_.son_adyacentes(j.desde, j.hasta);          // paso
    return j.ficha == 0 && celda_[j.medio] > 0 && tab_.destino_salto(j.desde, j.medio) == j.hasta;                 
}
