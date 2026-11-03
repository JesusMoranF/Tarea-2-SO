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
