
/* p2_ej1.c - Coste de crear hilos y procesos */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include "crono.h"

#define N_HILOS 1000
#define N_PROC 200
#define REP 5

/* El hilo no realiza ningún trabajo. */
static void *nada_hilo(void *arg)
{
    (void)arg;
    return NULL;
}

/* Crear todos los hilos y después esperar a que terminen. */
static double hilos_tanda(int n)
{
    pthread_t *h = malloc(n * sizeof *h);

    if (h == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    double t0 = reloj();

    for (int i = 0; i < n; i++) {
        if (pthread_create(&h[i], NULL, nada_hilo, NULL) != 0) {
            fprintf(stderr, "Error al crear un hilo\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; i++) {
        if (pthread_join(h[i], NULL) != 0) {
            fprintf(stderr, "Error al esperar un hilo\n");
            exit(EXIT_FAILURE);
        }
    }

    double t = reloj() - t0;
    free(h);

    return t / n;
}

/* Crear un hilo y esperar a que termine antes del siguiente. */
static double hilos_uno_a_uno(int n)
{
    pthread_t h;
    double t0 = reloj();

    for (int i = 0; i < n; i++) {
        if (pthread_create(&h, NULL, nada_hilo, NULL) != 0) {
            fprintf(stderr, "Error al crear un hilo\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_join(h, NULL) != 0) {
            fprintf(stderr, "Error al esperar un hilo\n");
            exit(EXIT_FAILURE);
        }
    }

    return (reloj() - t0) / n;
}

/* Crear todos los procesos y esperar después a que terminen. */
static double procesos_tanda(int n)
{
    pid_t *p = malloc(n * sizeof *p);

    if (p == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    double t0 = reloj();

    for (int i = 0; i < n; i++) {
        p[i] = fork();

        if (p[i] == 0) {
            _exit(0);
        }

        if (p[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; i++) {
        if (waitpid(p[i], NULL, 0) < 0) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }

    double t = reloj() - t0;
    free(p);

    return t / n;
}

/* Crear un proceso y esperar a que termine antes del siguiente. */
static double procesos_uno_a_uno(int n)
{
    double t0 = reloj();

    for (int i = 0; i < n; i++) {
        pid_t p = fork();

        if (p == 0) {
            _exit(0);
        }

        if (p < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (waitpid(p, NULL, 0) < 0) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }

    return (reloj() - t0) / n;
}

/* Repetir cada medida e imprimir el mínimo y la media. */
static double mide(const char *nombre, double (*f)(int), int n)
{
    double suma = 0;
    double min = 1e30;

    for (int r = 0; r < REP; r++) {
        double t = f(n);
        suma += t;

        if (t < min) {
            min = t;
        }
    }

    printf("%-18s: %10.1f us %10.1f us\n",
           nombre, min * 1e6, suma / REP * 1e6);

    return min;
}

int main(void)
{
    printf("%-18s %13s %13s (%d repeticiones)\n",
           "", "minimo", "media", REP);

    double a = mide("hilo en tanda", hilos_tanda, N_HILOS);
    double b = mide("hilo uno a uno", hilos_uno_a_uno, N_HILOS);
    double c = mide("proceso en tanda", procesos_tanda, N_PROC);
    double d = mide("proceso uno a uno", procesos_uno_a_uno, N_PROC);

    printf("factor proceso/hilo en tanda : %6.0f x\n", c / a);
    printf("factor proceso/hilo uno a uno : %6.0f x\n", d / b);

    return 0;
}
