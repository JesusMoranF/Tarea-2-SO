#include <cstdio>
#include "mesa.hpp"
#include <ctime>
#include <unistd.h>

int fallos = 0;
#define check(cond) do {if (!(cond)) {printf("falo linea %d: %s\n", __LINE__, #cond); fallos++;}} while (0)

struct PruebasMesa{
    static bool valida(Mesa& m, int ficha, int hasta, int medio){
        Candado g(&m.mtx_);
        return m.validar({ficha, m.pos_[ficha], hasta, medio});
    }
    static bool jugar(Mesa& m, int ficha, int hasta, int medio){
        Candado g(&m.mtx_);
        Mesa::Jugada j{ficha, m.pos_[ficha], hasta, medio};
        if (!m.validar(j)) return false;
        m.aplicar(j);
        m.evaluar_victoria();
        return true;
    }
    static bool acorralado(Mesa& m) {Candado g(&m.mtx_); return m.puma_acorralado();}
    static bool invariantes(Mesa& m) {Candado g(&m.mtx_); return m.invariantes_ok();}
    static int celda(Mesa& m, int n) {Candado g(&m.mtx_); return m.celda_[n];}

    static void matar(Mesa& m, int perro){
        Candado g(&m.mtx_);
        m.celda_[m.pos_[perro]] = -1;
        m.vivo_[perro] = false;
        m.capturas_++;
    }
};

void* hilo_juez(void* p){static_cast<Mesa*>(p)->ejecutar(); return nullptr;}

Config rapida(unsigned seed){
    Config c;
    c.seed = seed; c.min_ms = 1; c.max_ms = 4; c.silencioso = true;
    return c;
}

Estado jugar(Mesa& m){
    pthread_t j;
    pthread_create(&j, nullptr, hilo_juez, &m);
    pthread_join(j, nullptr);
    return m.estado_final();
}

int main(){
    Tablero tab;

        // 1.- muchas partidas
    int puma = 0, perros = 0, limite = 0;
    
    for (unsigned seed = 1; seed <=60; seed++){
        Mesa m(0, 1, 2, tab, rapida(seed));
        Estado e = jugar(m);
        check(e != Estado::en_curso && e !=Estado::error_interno);
        check((e == Estado::gana_puma) == (m.capturas() >=6));
        puma +=e == Estado::gana_puma;
        perros +=e == Estado::ganan_perros;
        limite += e== Estado::limite_movs;
    }
    printf("60 partidas: puma=%d perros=%d limite=%d\n", puma, perros, limite);

    for (int r = 0; r < 2; r++) {
        Config c = rapida(7); c.min_ms = 20; c.max_ms = 40;
        Mesa m(0, 1, 2, tab, c);
        check(!m.esperar_fin(20));                         // todavia no se ejecuto
        pthread_t j; pthread_create(&j, nullptr, hilo_juez, &m);
        usleep(60000);
        check(!m.esperar_fin(10));                         // la partida sigue
        m.abandonar(r == 0 ? Rol::puma : Rol::perros);
        check(m.esperar_fin(-1));
        pthread_join(j, nullptr);
        check(m.estado_final() == (r == 0 ? Estado::abandono_puma : Estado::abandono_perros));
    }

    // interrupcion del servidor
    {
        Config c = rapida(9); c.min_ms = 20; c.max_ms = 40;
        Mesa m(0, 1, 2, tab, c);
        pthread_t j; pthread_create(&j, nullptr, hilo_juez, &m);
        usleep(60000);
        m.solicitar_detencion();
        pthread_join(j, nullptr);
        check(m.estado_final() ==Estado::interrumpida);
    }

    // el puma parte acorralado, ganan los perros sin mover nada
    {
        Config c = rapida(3);
        c.puma = 0;
        c.perros = {1, 5, 26, 2, 10, 25, 12, 13, 14, 15, 16, 17};
        Mesa m(0, 1, 2, tab, c);
        check(jugar(m) == Estado::ganan_perros && m.movimientos()== 0);
    }

    // limite de movimientos
    {
        Config c = rapida(5); c.max_movs = 10;
        Mesa m(0, 1, 2, tab, c);
        check(jugar(m) == Estado::limite_movs && m.movimientos() ==10);
    }

    {
        Config c = rapida(1);
        c.perros = {17, 5, 6, 0, 1, 3, 4, 9, 14, 15, 19, 20}; 
        Mesa m(0, 1, 2, tab, c);                                
        check(!PruebasMesa::valida(m, 0, 17, -1));   
        check(!PruebasMesa::valida(m, 0, 22, -1));   
        check(PruebasMesa::valida(m, 0, 11, -1));  
        check(!PruebasMesa::valida(m, 0, 2, 7)); 
        check(!PruebasMesa::valida(m, 0, 23, 17));   
        check(!PruebasMesa::valida(m, 2, 7, 6));     
        check(PruebasMesa::valida(m, 0, 22, 17));   
        check(PruebasMesa::jugar(m, 0, 22, 17));
        check(m.capturas() == 1 && PruebasMesa::invariantes(m));
        check(PruebasMesa::celda(m, 17) == -1 && PruebasMesa::celda(m, 22) == 0 && PruebasMesa::celda(m, 12) == -1);
        check(!PruebasMesa::valida(m, 1, 12, -1));  

        Mesa m2(0, 1, 2, tab, c);              
        for (int p = 4; p <= 8; p++) PruebasMesa::matar(m2, p);
        check(m2.capturas() == 5 && PruebasMesa::invariantes(m2));
        check(PruebasMesa::jugar(m2, 0, 22, 17));
        check(m2.capturas() == 6 && m2.estado_final() == Estado::gana_puma);
        check(!PruebasMesa::jugar(m2, 0, 12, -1));  
    }

    {
        Config c = rapida(1);
        c.puma = 0;
        c.perros = {1, 5, 26, 10, 25, 12, 13, 14, 15, 16, 17, 18};   
        Mesa libre(0, 1, 2, tab, c);
        check(!PruebasMesa::acorralado(libre));
        c.perros[5] = 2;                                            
        Mesa cerrado(0, 1, 2, tab, c);
        check(PruebasMesa::acorralado(cerrado));
    }

    {
        Config c = rapida(5); c.min_ms = c.max_ms = 50; c.max_movs = 1;
        Mesa m(0, 1, 2, tab, c);
        timespec a, b;
        clock_gettime(CLOCK_MONOTONIC, &a);
        jugar(m);
        clock_gettime(CLOCK_MONOTONIC, &b);
        long ms = (b.tv_sec - a.tv_sec) * 1000 + (b.tv_nsec - a.tv_nsec)/1000000;
        check(ms >=40);
    }

    if (fallos == 0) printf("todo ok\n");
    if (fallos){
      return 1;
    }else{
      return 0;
}

}
