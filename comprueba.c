/* comprueba.c - ejecutalo ANTES de la sesion. Si no dice ENTORNO LISTO,
la practica no se puede hacer en clase. */

#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include "crono.h"

static atomic_long terminados;

static void *trabajador(void *arg)
{
    (void)arg;
    atomic_fetch_add(&terminados, 1);
    return NULL;
}

int main(void)
{
    pthread_t h[4];

    double t0 = reloj();

    for (int i = 0; i < 4; i++)
        pthread_create(&h[i], NULL, trabajador, NULL);

    for (int i = 0; i < 4; i++)
        pthread_join(h[i], NULL);

    double t = reloj() - t0;

    double paso = 1.0; /* el menor salto que da el reloj */

    for (int i = 0; i < 1000; i++) {
        double a = reloj(), b;
        do
            b = reloj();
        while (b == a);

        if (b - a < paso)
            paso = b - a;
    }

    long n = atomic_load(&terminados);

    printf("hilos terminados : %ld de 4\n", n);
    printf("crear y esperar 4 hilos : %.3f ms\n", t * 1e3);
    printf("resolucion del reloj : %.0f ns\n", paso * 1e9);

    printf("%s\n",
           (n == 4 && paso < 1e-6)
               ? "ENTORNO LISTO"
               : "ALGO FALLA: avisa antes de la sesion");

    return 0;
}