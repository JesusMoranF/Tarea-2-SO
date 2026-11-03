#include "mesa.hpp"
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <cstdlib>

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

// Un perro en destino es seguro si el puma no puede saltarlo.
bool Mesa::es_seguro(int destino, int desde_perro) const{
    for (int i = 0; i < (int)tab_.saltos(pos_[0]).size(); i++){
        Salto s = tab_.saltos(pos_[0])[i];
        if (s.medio == destino && (celda_[s.destino] == -1 || s.destino == desde_perro)) return false;
    }
    return true;
}

bool Mesa::elegir_jugada(int ficha, unsigned int* semilla, Jugada* out) const{
    int desde = pos_[ficha];
    vector<int> libres = vecinos_libres(desde);

    if (ficha == 0){   // el puma prefiere capturar
        vector<Salto> capturas;
        for (int i = 0; i < (int)tab_.saltos(desde).size(); i++)
            if (puede_capturar(tab_.saltos(desde)[i])) capturas.push_back(tab_.saltos(desde)[i]);
        if (!capturas.empty()){
            Salto s = capturas[rand_r(semilla) % capturas.size()];
            *out = {0, desde, s.destino, s.medio};
            return true;
        }
        
    }else if (!libres.empty() && (int)(rand_r(semilla) % 100) >= cfg_.torpeza_pct){
        // un perro que piensa solo se acerca al puma por casillas donde no lo puedan saltar, si no hay ninguna, se queda quieto. Con probabilidad torpeza_pct se equivoca y mueve al azar, para que haya handicap o si no el puma nunca ganaria

        vector<int> dist(num_nodos, -1), cola(1, pos_[0]);    
        dist[pos_[0]] = 0;
         for (int i = 0; i < (int)cola.size(); i++)
              for (int k = 0; k < (int)tab_.vecinos(cola[i]).size(); k++){
                int v = tab_.vecinos(cola[i])[k];
                if (dist[v] == -1) {dist[v] = dist[cola[i]] + 1; cola.push_back(v);}
            }
        vector<int> acercan;
        for (int i = 0; i < (int)libres.size(); i++)
            if (dist[libres[i]] < dist[desde] && es_seguro(libres[i], desde)) acercan.push_back(libres[i]);
        if (acercan.empty()) return false;
        libres = acercan;
    }
    if (libres.empty()) return false;
    *out = {ficha, desde, libres[rand_r(semilla) % libres.size()], -1};
    return true;
}

// El Juez valida la jugada y debe respetar la linea de salto y casillas.
bool Mesa::validar(const Jugada& j) const{
    if (estado_ != Estado::en_curso || !vivo_[j.ficha]) return false;
    if (pos_[j.ficha] != j.desde || celda_[j.hasta] != -1) return false;     // destino ocupado
    if (j.medio < 0) return tab_.son_adyacentes(j.desde, j.hasta);          // paso
    return j.ficha == 0 && celda_[j.medio] > 0 && tab_.destino_salto(j.desde, j.medio) == j.hasta;                 
}

void Mesa::aplicar(const Jugada& j){
    celda_[j.desde] = -1;
    celda_[j.hasta] = j.ficha;
    pos_[j.ficha] = j.hasta;
    movs_++;
    movs_sin_dibujar_++;
    string de_a = to_string(j.desde) + " -> " + to_string(j.hasta);
    if (j.medio >= 0) {
        int perro = celda_[j.medio];
        vivo_[perro] = false;
        celda_[j.medio] = -1;
        capturas_++;
        evento("puma salta " + de_a + " y captura al perro " + to_string(perro) + " [capturas=" + to_string(capturas_) + "]");
    }else{
        evento((j.ficha == 0 ? string("puma") : "perro " + to_string(j.ficha)) + " mueve " + de_a);
    }
}

bool Mesa::invariantes_ok() const{
    int vivas = 0, ocupadas = 0;
    for (int f = 0; f < 13; f++)
        if (vivo_[f]) {
            vivas++;
            if (celda_[pos_[f]] != f) return false;       // cada ficha viva esta en su casilla
        }
    
    for (int n = 0; n < num_nodos; n++)
        if (celda_[n] != -1) ocupadas++;
    return vivo_[0] && ocupadas == vivas && 13 - vivas == capturas_;
}

void Mesa::evaluar_victoria(){
    if (estado_ != Estado::en_curso) return;
    if (!invariantes_ok())  terminar(Estado::error_interno);
    else if (capturas_ >= 6)   terminar(Estado::gana_puma);
    else if (puma_acorralado())    terminar(Estado::ganan_perros);
    else if (cfg_.max_movs > 0 && movs_ >= cfg_.max_movs) terminar(Estado::limite_movs);
}

// El primer cierre gana, despierta a todas las fichas como broadcast y al juez
void Mesa::terminar(Estado e){
    if (estado_ != Estado::en_curso) return;
    estado_ = e;
    evento(mensaje_final(e));
    pthread_cond_broadcast(&cv_fichas_);
}

void Mesa::evento(const string& texto){
    eventos_.push_back("Mesa " + to_string(id_) + ": " + texto);
    pthread_cond_signal(&cv_juez_);
}

string Mesa::mensaje_final(Estado e) const{
    string jp = jugador + to_string(jugador_puma_), jd = jugador + to_string(jugador_perros_);

    if (e == Estado::gana_puma){
        return "*** gana el puma (" + jp + ") con " + to_string(capturas_) + " capturas ***";
    } 
    else if (e == Estado::ganan_perros){
        return "*** ganan los perros (" + jd + "): el puma quedo acorralado ***";
    } 
    else if (e == Estado::abandono_puma){
        return "*** el " + jp + " (Puma) abandono: gana el " + jd + " (Perros) por abandono ***";
    } 
    else if (e == Estado::abandono_perros){
        return "*** el " + jd + " (Perros) abandono: gana el " + jp + " (Puma) por abandono ***";
    } 
    else if (e == Estado::interrumpida){
        return "*** partida interrumpida por el servidor ***";
    } 
    else if (e == Estado::limite_movs){
        return "*** empate: se alcanzo el limite de " + to_string(cfg_.max_movs) + " movimientos ***";
    } 
    else{
        return "*** error, invariante del tablero violado ***";
    }
}


string Mesa::dibujar_tablero() const{
    string c[num_nodos];                                  
    for (int n = 0; n < num_nodos; n++){
       if (celda_[n] == -1){
          c[n] = ".";
            }else if (celda_[n] == 0){
          c[n] = "P";
            }else{
           c[n] = "D";
      }     
    }
    
    string s = "Mesa " + to_string(id_) + ": tablero (movs=" + to_string(movs_) + ", capturas=" + to_string(capturas_) + ")\n"; s += "       " + c[25] + "\n      " + c[26] + "   " + c[27] + "\n";       
    
    for (int f = 0; f < 5; f++) {
        s += "    ";
        for (int col = 0; col < 5; col++) s += c[f * 5 + col] + " ";
        s += "\n";
    }
    return s;
}


void* Mesa::hilo_ficha(void* arg){
    ArgsFicha* a = static_cast<ArgsFicha*>(arg);
    a->mesa->ciclo_ficha(a->ficha, a->semilla);
    return nullptr;
}

void Mesa::ciclo_ficha(int ficha, unsigned int semilla){
    for (;;) {
        // retardo aleatorio 
        long ms = cfg_.min_ms + rand_r(&semilla) % (cfg_.max_ms - cfg_.min_ms + 1L);
        timespec t = {ms / 1000, (ms % 1000) * 1000000L};
        nanosleep(&t, nullptr);

        Jugada j;
        bool hay = elegir_jugada(ficha, &semilla, &j);       // se elige la jugada mirando el tablero

        Candado g(&mtx_);                                    // recien despúes se toma el mutex
        if (estado_ != Estado::en_curso || !vivo_[ficha]) return;
        if (hay && validar(j)){
            aplicar(j);
            evaluar_victoria();
 }
    }
}

void Mesa::ejecutar(){
 {
        Candado g(&mtx_);
        evaluar_victoria();    // por si el puma ya esta acorralado namas comenzar
 }
    // lanzar las 13 fichas
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);
    ArgsFicha args[13];
    pthread_t hilos[13];
    int creados = 0;
    for (int f = 0; f < 13; f++){
         args[f] = {this, f, cfg_.seed + (unsigned)id_ * 1000u + (unsigned)f * 7u+1u};
        if (pthread_create(&hilos[f], &attr, hilo_ficha, &args[f]) != 0){
            Candado g(&mtx_);
            terminar(Estado::error_interno);
            break;
        }
     creados++;
    }
    pthread_attr_destroy(&attr);

    for (int f = 0; f < creados; f++) pthread_join(hilos[f], nullptr);
}
