/* p2_ej3.c la misma suma, arreglada de tres formas (apartados 2.2.2,
2.2.5 y 2.2.11), y cronometrada. */

#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include "crono.h"

#define K 5000000   /* incrementos por hilo */
#define REP 5       

/* INCORRECTA: solo de referencia */
static volatile long long c_suelto;

static long long c_mutex;
static pthread_mutex_t cerrojo = PTHREAD_MUTEX_INITIALIZER;

static atomic_long c_atomico;

static long long c_priv;
static pthread_mutex_t cerrojo_final = PTHREAD_MUTEX_INITIALIZER;

static void *v_suelto(void *a)
{
    (void)a;
    for (int i = 0; i < K; i++) c_suelto++;
    return NULL;
}

/* Versión con mutex: un cerrojo alrededor del incremento */
static void *v_mutex(void *a)
{
    (void)a;
    for (int i = 0; i < K; i++) {
        pthread_mutex_lock(&cerrojo);
        c_mutex++;
        pthread_mutex_unlock(&cerrojo);
    }
    return NULL;
}

/* Versión atómica: atomic_fetch_add sin cerrojo */
static void *v_atomico(void *a)
{
    (void)a;
    for (int i = 0; i < K; i++) {
        atomic_fetch_add(&c_atomico, 1);
    }
    return NULL;
}

/* Versión con acumulador privado */
static void *v_priv(void *a)
{
    (void)a;
    /* volatile A PROPOSITO: sin el, -O2 convierte el bucle en mio = K y se
    mediria nada. Asi escribe en memoria en cada vuelta, como los otros. */
    volatile long long mio = 0; /* local: nadie mas la ve */

    for (int i = 0; i < K; i++) mio++;
    
    /* UNA sola vez por hilo */
    pthread_mutex_lock(&cerrojo_final);
    c_priv += mio;
    pthread_mutex_unlock(&cerrojo_final);
    
    return NULL;
}

static void r_suelto(void)  { c_suelto = 0; }
static void r_mutex(void)   { c_mutex = 0; }
static void r_atomico(void) { atomic_store(&c_atomico, 0); }
static void r_priv(void)    { c_priv = 0; }

/* Lanza P hilos con la funcion f y devuelve el mejor tiempo de REP. */
static double cronometra(void *(*f)(void*), void (*reinicia)(void), int P)
{
    double mejor = 1e30;
    pthread_t h[32];
    
    for (int r = 0; r < REP; r++) {
        reinicia();
        double t0 = reloj();
        
        for (int i = 0; i < P; i++) pthread_create(&h[i], NULL, f, NULL);
        for (int i = 0; i < P; i++) pthread_join(h[i], NULL);
        
        double t = reloj() - t0;
        if (t < mejor) mejor = t;
    }
    return mejor;
}

int main(void)
{
    int hilos[] = { 1, 2, 4, 8, 12, 24 };
    
    printf("   P  suelto(s)   mutex(s)  atomico(s)  privado(s)  correctas  suelto\n");
    
    for (int k = 0; k < 6; k++) {
        int P = hilos[k];
        long long esperado = (long long)P * K;
        
        double ts = cronometra(v_suelto, r_suelto, P);
        double tm = cronometra(v_mutex, r_mutex, P);
        double ta = cronometra(v_atomico, r_atomico, P);
        double tp = cronometra(v_priv, r_priv, P);
        
        int ok = c_mutex == esperado && atomic_load(&c_atomico) == esperado && c_priv == esperado;
        
        printf("%4d %10.4f %10.4f %11.4f %11.4f  %-9s  %s\n", P, ts, tm, ta, tp,
               ok ? "las 3 si" : "NO", c_suelto == esperado ? "si" : "NO");
    }
    return 0;
}