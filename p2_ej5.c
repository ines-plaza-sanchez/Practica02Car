/* p2_ej5.c - un cerrojo, muchos cerrojos o ninguno (apartado 2.2.4). */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "crono.h"

#define CASILLAS 256
#define MUESTRAS 4000000
#define REP 5
#define MAX_HILOS 24

/* ---------------------------------------------------------
   Datos comunes
   --------------------------------------------------------- */

typedef struct {
    int id;
} Argumento;

/* Generador pseudoaleatorio propio.
   No usamos rand(), porque puede introducir sincronizacion interna. */
static unsigned siguiente(unsigned *s)
{
    *s = *s * 1103515245u + 12345u;
    return (*s >> 16) & 255u;
}


/* =========================================================
   VERSION A: UN SOLO CERROJO GLOBAL
   ========================================================= */

static long long hist_global[CASILLAS];

static pthread_mutex_t cerrojo_global =
    PTHREAD_MUTEX_INITIALIZER;

static void *v_global(void *arg)
{
    Argumento *a = (Argumento *)arg;
    unsigned semilla = (unsigned)a->id + 1u;

    for (int i = 0; i < MUESTRAS; i++) {

        unsigned casilla = siguiente(&semilla);

        pthread_mutex_lock(&cerrojo_global);

        hist_global[casilla]++;

        pthread_mutex_unlock(&cerrojo_global);
    }

    return NULL;
}


/* =========================================================
   VERSION B: UN CERROJO POR CASILLA
   ========================================================= */

static long long hist_casilla[CASILLAS];

static pthread_mutex_t cerrojos[CASILLAS];

static void *v_casilla(void *arg)
{
    Argumento *a = (Argumento *)arg;
    unsigned semilla = (unsigned)a->id + 1u;

    for (int i = 0; i < MUESTRAS; i++) {

        unsigned casilla = siguiente(&semilla);

        pthread_mutex_lock(&cerrojos[casilla]);

        hist_casilla[casilla]++;

        pthread_mutex_unlock(&cerrojos[casilla]);
    }

    return NULL;
}


/* =========================================================
   VERSION C: HISTOGRAMA PRIVADO POR HILO
   ========================================================= */

static long long hist_privado[MAX_HILOS][CASILLAS];
static long long hist_total[CASILLAS];

static void *v_privado(void *arg)
{
    Argumento *a = (Argumento *)arg;
    int id = a->id;

    unsigned semilla = (unsigned)id + 1u;

    for (int i = 0; i < MUESTRAS; i++) {

        unsigned casilla = siguiente(&semilla);

        /*
         * Cada hilo escribe solamente en su propio histograma.
         * No hace falta ningun mutex.
         */
        hist_privado[id][casilla]++;
    }

    return NULL;
}


/* =========================================================
   FUNCIONES AUXILIARES
   ========================================================= */

static long long suma_histograma(const long long hist[CASILLAS])
{
    long long total = 0;

    for (int i = 0; i < CASILLAS; i++)
        total += hist[i];

    return total;
}


/* Crea P hilos, ejecuta la funcion indicada y espera a todos. */
static void lanza_hilos(void *(*funcion)(void *), int P)
{
    pthread_t h[MAX_HILOS];
    Argumento args[MAX_HILOS];

    for (int i = 0; i < P; i++) {

        args[i].id = i;

        pthread_create(
            &h[i],
            NULL,
            funcion,
            &args[i]);
    }

    for (int i = 0; i < P; i++)
        pthread_join(h[i], NULL);
}


/* =========================================================
   MEDIDA VERSION A
   ========================================================= */

static double mide_global(int P, int *correcto)
{
    double mejor = 1e30;

    long long esperado =
        (long long)P * MUESTRAS;

    *correcto = 1;

    for (int r = 0; r < REP; r++) {

        memset(hist_global, 0, sizeof hist_global);

        double t0 = reloj();

        lanza_hilos(v_global, P);

        double t = reloj() - t0;

        if (suma_histograma(hist_global) != esperado)
            *correcto = 0;

        if (t < mejor)
            mejor = t;
    }

    return mejor;
}


/* =========================================================
   MEDIDA VERSION B
   ========================================================= */

static double mide_casilla(int P, int *correcto)
{
    double mejor = 1e30;

    long long esperado =
        (long long)P * MUESTRAS;

    *correcto = 1;

    for (int r = 0; r < REP; r++) {

        memset(hist_casilla, 0, sizeof hist_casilla);

        double t0 = reloj();

        lanza_hilos(v_casilla, P);

        double t = reloj() - t0;

        if (suma_histograma(hist_casilla) != esperado)
            *correcto = 0;

        if (t < mejor)
            mejor = t;
    }

    return mejor;
}


/* =========================================================
   MEDIDA VERSION C
   ========================================================= */

static double mide_privado(int P, int *correcto)
{
    double mejor = 1e30;

    long long esperado =
        (long long)P * MUESTRAS;

    *correcto = 1;

    for (int r = 0; r < REP; r++) {

        memset(hist_privado, 0, sizeof hist_privado);
        memset(hist_total, 0, sizeof hist_total);

        double t0 = reloj();

        lanza_hilos(v_privado, P);

        /*
         * Una vez terminados los hilos,
         * se combinan los histogramas privados.
         */
        for (int h = 0; h < P; h++) {

            for (int c = 0; c < CASILLAS; c++) {

                hist_total[c] +=
                    hist_privado[h][c];
            }
        }

        double t = reloj() - t0;

        if (suma_histograma(hist_total) != esperado)
            *correcto = 0;

        if (t < mejor)
            mejor = t;
    }

    return mejor;
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    int hilos[] = {1, 2, 4, 8, 12, 24};

    double base_global = 0.0;
    double base_casilla = 0.0;
    double base_privado = 0.0;


    /* Inicializar los 256 mutex de la version B. */
    for (int i = 0; i < CASILLAS; i++) {

        pthread_mutex_init(
            &cerrojos[i],
            NULL);
    }


    printf(
        " P   global(s)   xN   casilla(s)   xN   privado(s)   xN   ok\n"
    );


    for (int i = 0; i < 6; i++) {

        int P = hilos[i];

        int ok_global;
        int ok_casilla;
        int ok_privado;


        double tg =
            mide_global(P, &ok_global);

        double tc =
            mide_casilla(P, &ok_casilla);

        double tp =
            mide_privado(P, &ok_privado);


        /*
         * Con un hilo tenemos la referencia
         * para calcular el factor de degradacion.
         */
        if (P == 1) {
            base_global = tg;
            base_casilla = tc;
            base_privado = tp;
        }


        double fg = tg / base_global;
        double fc = tc / base_casilla;
        double fp = tp / base_privado;


        int ok =
            ok_global &&
            ok_casilla &&
            ok_privado;


        printf(
            "%2d   %9.4f  %5.2f   %10.4f  %5.2f   "
            "%10.4f  %5.2f   %s\n",
            P,
            tg, fg,
            tc, fc,
            tp, fp,
            ok ? "si" : "NO");
    }


    /* Liberar los 256 mutex. */
    for (int i = 0; i < CASILLAS; i++) {

        pthread_mutex_destroy(
            &cerrojos[i]);
    }


    return 0;
}