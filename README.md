# Torneo de Komikan 

Tarea 2 - Sistemas Operativos (UDP)

Estudiante: Jesús Morán

## Compilar

```
g++ -Wall -Wextra -std=c++17 torneo.cpp board.cpp config.cpp mesa.cpp -o torneo -pthread
```

## Ejecutar

```
./torneo N M [config.txt]
```

- `N`: cantidad de jugadores del torneo. Debe ser positivo y par, de lo contrario el programa avisa y termina.
- `M`: maximo de mesas jugando al mismo tiempo positivo. Si hay mas parejas que mesas, esperan de forma pasiva hasta que se libera una.
- `config.txt`: archivo de configuración.

Cada vez que una mesa abre o cierra se imprime `mesas activas: k/M`, el programa lleva ese contador con el mutex del torneo y termina con error si alguna vez `k > M`. Al final muestra el resumen de partidas y el máximo de mesas simultaneas observado.

**Ctrl+C (SIGINT/SIGTERM)**: deja de entrar gente, avisa a todas las mesas activas (que terminan como `interrumpida`), cancela a los jugadores que aun esperan mesa, espera a que no quede ningun hilo, libera todo y sale con codigo 0. 

Codigos de salida: 0 = termino bien o por Ctrl+C; 

Codigos de salida: 1 = argumentos o config invalidos, o el sistema no dejo crear mas hilos.

## Archivo de configuracion

Una linea por parametro clave valor. Lineas en blanco y comentarios con `#` permitidos, y las claves pueden ir en cualquier orden. Las que no aparezcan usan el valor por defecto.

| clave             | valor por defecto | significado                                    |
|-------------------|---------|----------------------------------------------------------|
| `min_ms`          | 50      | retardo minimo por movimiento                            |
| `max_ms`          | 300     | retardo maximo por movimiento                            |
| `seed`            | 42      | semilla de todas las decisiones aleatorias               |
| `puma`            | 12      | nodo inicial del puma                                    |
| `perro`           | -       | nodo inicial de un perro, se repite 12 veces             |
| `max_movs`        | 1500    | limite de movimientos por partida (0 = sin limite)       |
| `periodo_tablero` | 10      | el Juez dibuja el tablero cada K movimientos             |
| `llegada_max_ms`  | 20      | los jugadores llegan con un retardo en [0, este] (0 a 1000) |
| `abandona`        | -       | id de un jugador que falla (se repite por cada uno)      |
| `abandona_ms`     | 500     | ms despues de empezar su partida en que ese jugador falla|

La disposicion inicial es todo o no funciona, si el archivo trae `puma` o `perro` debe traer el puma y los 12 perros, no menos, en nodos distintos y existentes. Si falta algo el programa avisa y termina.
Estan cargados estos de ejemplos: `config.txt` (la disposición por defecto), `config_prueba.txt` (otra disposición), `config_abandono.txt` (falla el jugador 1 a los 200 ms) y `config_rapida.txt` (partidas rápidas para pruebas).

## Tablero

28 nodos, la grilla 5x5 y el trono arriba.

```
                  25
          26              27
  0       1       2       3       4
  5       6       7       8       9
 10      11      12      13      14
 15      16      17      18      19
 20      21      22      23      24
```

- `0..24`: grilla, nodo = fila*5 + columna (solo conexiones ortogonales, sin diagonales).
- `25` = T (cima), `26` y `27` = nodos laterales del trono.


El grafo se define por lineas de nodos colineale, dos nodos consecutivos son adyacentes y tres consecutivos permiten un saltota.

## Interpretaciones


- **Saltos validos**: en linea recta sobre un perro hacia una casilla libre, por ejemplo 25 -> 26 -> 0, 25 -> 2 -> 7, 26 -> 1 -> 6. Capturar no es obligatorio.
- **Victoria del puma**: 6 capturas. **Victoria de los perros**: el puma sin pasos ni saltos.
- **Empate por limite de movimientos**: la tarea solo contempla victoria de uno u otro, (`max_movs`) es una salvavidas contra una partida que nunca termine (livelock).
- **Abandono**: cada jugador es un hilo que vive mientras dura su partida. Los jugadores marcados con `abandona` en el config fallan a los `abandona_ms` de empezar, su hilo
  avisa a la mesa con `Mesa::abandonar(rol)` y el Juez, que dormia en su variable de condicion, cierra la partida como abandono y declara ganador al oponente. La falla es simulada, pero el camino de deteccion es el mismo
  que tendria una falla real. El Juez también detecta en el mismo bucle un tablero inconsistente (`error_interno`), la interrupción del servidor y la falla al crear sus fichas. Quien queda de Puma y quien de
  Perros depende del orden de llegada, asi que un mismo id puede tener cualquiera de los dos roles.



## Limites

- `N` hasta 100000 y `M` hasta 10000. `llegada_max_ms` se limita a 1000 ms porque un hilo dormido no se puede despertar con Ctrl+C. Los hilos de jugadores y de jueces son *detached* y se cuentan, y main no mantiene mas de `2*M + 1000`
  jugadores vivos a la vez, asi un N grande no agota el limite de hilos del sistema (con 40000 jugadores y M=500, config rapida, termina sin errores).
- Si aun asi el sistema no deja crear un hilo, el programa lo avisa por stderr, cierra el torneo ordenadamente y sale con codigo 1.


