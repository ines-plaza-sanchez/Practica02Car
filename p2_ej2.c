/* p2_ej2.c - con que frecuencia falla una carrera (apartado 2.2.1). */

#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>

#define PRUEBAS 20

/*
 * volatile obliga a que el contador este en memoria.
 * IMPORTANTE: volatile NO arregla la carrera de datos.
 */
static long long contador;

static int vueltas_por_hilo;


/*
 * Funcion que ejecuta cada hilo.
 * Todos los hilos incrementan el mismo contador compartido.
 *
 * contador++ NO es una operacion atomica:
 * leer -> sumar 1 -> escribir.
 */
static void *sube(void *arg)
{
    (void)arg;

    for (int i = 0; i < vueltas_por_hilo; i++) {
        contador++;
    }

    return NULL;
}


/*
 * Ejecuta una prueba con:
 *
 * P = numero de hilos
 * K = incrementos que realiza cada hilo
 *
 * El resultado correcto deberia ser P * K.
 */
static long long una_ejecucion(int P, int K)
{
    pthread_t h[32];

    contador = 0;
    vueltas_por_hilo = K;

    /* Crear los P hilos. */
    for (int i = 0; i < P; i++) {
        pthread_create(&h[i], NULL, sube, NULL);
    }

    /* Esperar a que terminen todos. */
    for (int i = 0; i < P; i++) {
        pthread_join(h[i], NULL);
    }

    return contador;
}


int main(void)
{
    /*
     * Numero de hilos que se van a probar.
     */
    int hilos[] = {2, 4, 8, 12, 24};

    /*
     * Numero de incrementos que realiza cada hilo.
     */
    int vueltas[] = {1000, 100000, 10000000};


    /*
     * Recorremos los tres valores de K.
     */
    for (int k = 0; k < 3; k++) {

        printf("K = %d incrementos por hilo\n", vueltas[k]);
        printf(" hilos    esperado   minimo obtenido   fallan   perdida media\n");


        /*
         * Para cada K probamos 2, 4, 8, 12 y 24 hilos.
         */
        for (int p = 0; p < 5; p++) {

            /*
             * Si no existiese la carrera, el resultado deberia ser
             * exactamente numero de hilos * vueltas por hilo.
             */
            long long esperado =
                (long long)hilos[p] * vueltas[k];

            long long minimo = esperado;

            int fallan = 0;
            double perdida = 0;


            /*
             * Repetimos cada combinacion 20 veces.
             */
            for (int r = 0; r < PRUEBAS; r++) {

                long long v =
                    una_ejecucion(hilos[p], vueltas[k]);

                /*
                 * Si el resultado no coincide con el esperado,
                 * esa ejecucion ha fallado.
                 */
                if (v != esperado) {
                    fallan++;
                }

                /*
                 * Guardamos el menor resultado obtenido.
                 */
                if (v < minimo) {
                    minimo = v;
                }

                /*
                 * Calculamos el porcentaje de incrementos perdidos.
                 */
                perdida +=
                    100.0 *
                    (double)(esperado - v) /
                    (double)esperado;
            }


            /*
             * PRId64 permite imprimir enteros de 64 bits
             * de forma portable entre Windows, macOS y Linux.
             */
            printf(" %5d %11" PRId64 " %17" PRId64
                   " %3d/%d %10.1f %%\n",
                   hilos[p],
                   (int64_t)esperado,
                   (int64_t)minimo,
                   fallan,
                   PRUEBAS,
                   perdida / PRUEBAS);
        }

        printf("\n");
    }

    return 0;
}