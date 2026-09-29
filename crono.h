/* crono.h - un reloj de pared fino y portable.
   time() solo cuenta segundos y clock() no mide lo mismo en Windows que en
   Linux, asi que ninguno sirve para medir microsegundos.
   Aqui se usa QueryPerformanceCounter en Windows y
   clock_gettime(CLOCK_MONOTONIC) en Linux. Devuelve segundos.
*/

#ifndef CRONO_H
#define CRONO_H

#ifdef _WIN32

#include <windows.h>

static double reloj(void)
{
    LARGE_INTEGER f, t;

    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);

    return (double)t.QuadPart / (double)f.QuadPart;
}

#else

#include <time.h>

static double reloj(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);

    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

#endif

#endif