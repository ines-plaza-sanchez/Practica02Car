/*
 * p2_ej4.c - Ejercicio 4: construir un interbloqueo y corregirlo.
 * Adaptado para macOS (POSIX).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include "crono.h"

#define TRANSFERENCIAS 200000
#define SALDO_INICIAL  1000000LL
#define VUELTAS_SIN_AVANCE 10

typedef struct {
    int id;
    long long saldo;
    pthread_mutex_t cerrojo;
} Cuenta;

static Cuenta cuentas[2];
static int ordenar; /* 0 = versión mala, 1 = versión buena */
static atomic_long progreso[2];
static atomic_int terminado;

static void espera_100ms(void)
{
    struct timespec pausa = {0, 100000000L};
    while (nanosleep(&pausa, &pausa) == -1) {
        /* Reanudar la pausa con el tiempo restante si llega una señal. */
    }
}

static void transferir(Cuenta *desde, Cuenta *hacia, long long cantidad)
{
    Cuenta *primero = desde;
    Cuenta *segundo = hacia;

    /* La versión buena solicita siempre los cerrojos por id creciente. */
    if (ordenar && primero->id > segundo->id) {
        primero = hacia;
        segundo = desde;
    }

    pthread_mutex_lock(&primero->cerrojo);
    pthread_mutex_lock(&segundo->cerrojo);

    desde->saldo -= cantidad;
    hacia->saldo += cantidad;

    pthread_mutex_unlock(&segundo->cerrojo);
    pthread_mutex_unlock(&primero->cerrojo);
}

static void *trabajador(void *arg)
{
    int yo = (int)(size_t)arg;
    Cuenta *origen = &cuentas[yo];
    Cuenta *destino = &cuentas[1 - yo];

    for (int i = 1; i <= TRANSFERENCIAS; i++) {
        transferir(origen, destino, 1);
        atomic_store(&progreso[yo], i);
    }
    return NULL;
}

static void *vigilante(void *arg)
{
    (void)arg;
    long ant0 = -1, ant1 = -1;
    int quietas = 0;

    while (!atomic_load(&terminado)) {
        espera_100ms();

        long p0 = atomic_load(&progreso[0]);
        long p1 = atomic_load(&progreso[1]);

        if (p0 == ant0 && p1 == ant1)
            quietas++;
        else
            quietas = 0;

        ant0 = p0;
        ant1 = p1;

        if (quietas >= VUELTAS_SIN_AVANCE && !atomic_load(&terminado)) {
            printf("*** INTERBLOQUEO *** hilo 0 parado en la transferencia %ld de %d\n",
                   p0, TRANSFERENCIAS);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2 ||
        (strcmp(argv[1], "mala") != 0 && strcmp(argv[1], "buena") != 0)) {
        fprintf(stderr, "Uso: %s mala|buena\n", argv[0]);
        return 2;
    }

    ordenar = (strcmp(argv[1], "buena") == 0);

    for (int i = 0; i < 2; i++) {
        cuentas[i].id = i;
        cuentas[i].saldo = SALDO_INICIAL;
        if (pthread_mutex_init(&cuentas[i].cerrojo, NULL) != 0) {
            fprintf(stderr, "Error al inicializar un cerrojo\n");
            return EXIT_FAILURE;
        }
    }

    pthread_t h[2], v;
    double t0 = reloj();

    if (pthread_create(&v, NULL, vigilante, NULL) != 0) {
        fprintf(stderr, "Error al crear el hilo vigilante\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 2; i++) {
        if (pthread_create(&h[i], NULL, trabajador, (void *)(size_t)i) != 0) {
            fprintf(stderr, "Error al crear un hilo trabajador\n");
            atomic_store(&terminado, 1);
            pthread_join(v, NULL);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (pthread_join(h[i], NULL) != 0) {
            fprintf(stderr, "Error al esperar un hilo trabajador\n");
            atomic_store(&terminado, 1);
            pthread_join(v, NULL);
            return EXIT_FAILURE;
        }
    }

    double t = reloj() - t0;
    atomic_store(&terminado, 1);
    pthread_join(v, NULL);

    long long suma = cuentas[0].saldo + cuentas[1].saldo;
    printf("terminado en %.4f s\n", t);
    printf("saldos: A = %lld B = %lld    (suma %lld, tiene que ser %lld)\n",
           cuentas[0].saldo, cuentas[1].saldo, suma, 2 * SALDO_INICIAL);

    for (int i = 0; i < 2; i++)
        pthread_mutex_destroy(&cuentas[i].cerrojo);

    return (suma == 2 * SALDO_INICIAL) ? EXIT_SUCCESS : EXIT_FAILURE;
}
