#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>     
#include <sys/types.h> 
#include <sys/time.h> 
#include <sys/wait.h> 
#define _GNU_SOURCE

#define N_ITER 2000
#define CSV_NOTAS "notas_alumnos.csv"
#define NUMERO_ALUMNOS 500

double nota_final(const double *notas){
    double sum_labs = 0;
    for (int i = 0; i < 8; i++){
        sum_labs += notas[i];
    }
    double promLabs = sum_labs / 8;
    double ex1 = notas[8];
    double ex2 = notas[9];
    
    return 0.6*promLabs + 0.2*ex1 + 0.2*ex2;
}

double mean(const double *arr, int n) {
    double suma = 0;
    for (int i = 0; i < n; i++){
        suma += arr[i];
    }

    return suma / (double)n;
}

double moda(const double *arr, int n){
    int max_count = 0;
    double modo = arr[0];

    for (int i = 0; i < n; ++i) {
        int count = 0;
        for (int j = 0; j < n; ++j)
            if (arr[j] == arr[i]) count++;

        if (count > max_count) {
            max_count = count;
            modo = arr[i];
        }
    }
    return modo;
}

double max(const double *arr, int n){
    double m = arr[0];
    for (int i = 0; i < n; i++){
        if (arr[i] > m){
            m = arr[i];
        }
    }
    return m;
}

double min(const double *arr, int n){
    double m = arr[0];
    for (int i = 0; i < n; i++){
        if (arr[i] < m){
            m = arr[i];
        }
    }
    return m;
}

int cargar_notas(const char *filename,
                        double notas[][10], int numAlum)
{
    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    char line[1024];

    // Saltar encabezado
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }

    int count = 0;
    while (count < numAlum && fgets(line, sizeof(line), f)) {
        char *saveptr = NULL;
        char *token = strtok_r(line, ",", &saveptr);
        if (!token) continue;

        token = strtok_r(NULL, ",", &saveptr);  // lab1

        for (int i = 0; i < 8; ++i) {
            notas[count][i] = token ? atof(token) : 0.0;
            token = strtok_r(NULL, ",", &saveptr);
        }

        // ex1
        notas[count][8] = token ? atof(token) : 0.0;
        token = strtok_r(NULL, ",", &saveptr);

        // ex2
        notas[count][9] = token ? atof(token) : 0.0;

        count++;
    }

    fclose(f);
    return count;
}

//Se va utilizar este método para hallar los tiempos de jecución porque con el clock_t..=clock()
// no arroja tiempos precisos
double ahora_en_segundos(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);  // tv_sec = segundos, tv_usec = microsegundos

    double t = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    return t;
}

void estadisticasXeval(double notas[][10], int numAlum, double media[10], double modaS[10],
                            double minimo[10], double maximo[10])
{
    double columna[NUMERO_ALUMNOS];

    for(int j = 0; j < 10; j++){
        for (int i = 0; i < NUMERO_ALUMNOS; i++)
        {
            columna[i] = notas[i][j];
        }
        media[j] = mean(columna, numAlum);
        modaS[j] = moda(columna, numAlum);
        minimo[j] = min(columna, numAlum);
        maximo[j] = max(columna, numAlum);
    }
}

double cpu_secuencial(void){
    double notas[NUMERO_ALUMNOS][10];
    int num_alum = cargar_notas(CSV_NOTAS, notas, NUMERO_ALUMNOS);

    double media[10];
    double moda_[10];
    double minimo[10];
    double maximo[10];

    double inicio = ahora_en_segundos();

    for (int i = 0; i < N_ITER; i++){
        estadisticasXeval(notas, num_alum, media, moda_, minimo, maximo);
    }
    double fin = ahora_en_segundos();

    double t_ejecucion = fin - inicio;

    return t_ejecucion;

    printf("Tiempo de ejecucion: %.6f segundos\n", t_ejecucion);
    
}

typedef struct {
    int iterations;
    int num_alumnos;
    double (*notas)[10];
} CpuThreadArgs;

void *cpu_worker(void *arg) {
    CpuThreadArgs *a = (CpuThreadArgs*)arg;
    double media[10], modo[10], minimo[10], maximo[10];
    for (int i = 0; i < a->iterations; ++i) {
        estadisticasXeval(a->notas, a->num_alumnos,
                              media, modo, minimo, maximo);
    }
    return NULL;
}

double cpu_threading(int num_threads) {
    double notas[NUMERO_ALUMNOS][10];
    int num_alumnos = cargar_notas(CSV_NOTAS, notas, NUMERO_ALUMNOS);

    pthread_t threads[num_threads];
    CpuThreadArgs args[num_threads];

    int base = N_ITER / num_threads;
    int extra = N_ITER % num_threads;

    double inicio = ahora_en_segundos();

    for (int t = 0; t < num_threads; ++t) {
        args[t].iterations = base + (t < extra ? 1 : 0);
        args[t].notas      = notas;
        args[t].num_alumnos= num_alumnos;
        pthread_create(&threads[t], NULL, cpu_worker, &args[t]);
    }
    for (int t = 0; t < num_threads; ++t)
        pthread_join(threads[t], NULL);

    double fin = ahora_en_segundos();

    double t_ejecucion = fin - inicio;

    return t_ejecucion;

    printf("Tiempo de ejecucion con %.2d hilos: %.6f segundos\n", num_threads, t_ejecucion);
}


double cpu_multiprocessing(int num_proc) {
    int base = N_ITER / num_proc;
    int extra = N_ITER % num_proc;

    double inicio = ahora_en_segundos();

    for (int p = 0; p < num_proc; ++p) {
        pid_t pid = fork();
        if (pid == 0) {
            double notas[NUMERO_ALUMNOS][10];
            int num_alumnos = cargar_notas(CSV_NOTAS, notas, NUMERO_ALUMNOS);

            int iteraciones = base + (p < extra ? 1 : 0);
            double media[10], modo[10], minimo[10], maximo[10];
            for (int i = 0; i < iteraciones; ++i) {
                estadisticasXeval(notas, num_alumnos,
                                      media, modo, minimo, maximo);
            }
            _exit(0);
        }
    }

    for (int p = 0; p < num_proc; ++p)
        wait(NULL);

    double fin = ahora_en_segundos();

    double t_ejecucion = fin - inicio;

    return t_ejecucion;

    printf("Tiempo de ejecucion con %.2d procesos: %.6f segundos\n", num_proc, t_ejecucion);
}

double io_secuencial(void) {
    double notas[NUMERO_ALUMNOS][10];
    double media[10];
    double modas[10];
    double minimo[10];
    double maximo[10];

    double inicio = ahora_en_segundos();

    for (int i = 0; i < N_ITER; i++) {
        int numAlum = cargar_notas(CSV_NOTAS,
                                   notas, NUMERO_ALUMNOS);

        if (numAlum <= 0) continue;

        estadisticasXeval(notas, numAlum,
                          media, modas, minimo, maximo);
    }

    double fin = ahora_en_segundos();

    double t_ejecucion = fin - inicio;

    return t_ejecucion;

    printf("Tiempo de ejecucion: %.6f segundos\n", t_ejecucion);
}

typedef struct {
    int iteraciones;
} IoArgs;

void* io_trabajo(void* arg) {
    IoArgs* args = (IoArgs*)arg;

    double notas[NUMERO_ALUMNOS][10];
    double media[10];
    double modas[10];
    double minimo[10];
    double maximo[10];

    for (int i = 0; i < args->iteraciones; i++) {
        int numAlum = cargar_notas(CSV_NOTAS,
                                   notas, NUMERO_ALUMNOS);
        if (numAlum <= 0) continue;

        estadisticasXeval(notas, numAlum,
                          media, modas, minimo, maximo);
    }

    return NULL;
}

double io_threading(int num_threads) {
    pthread_t threads[num_threads];
    IoArgs args[num_threads];

    int base  = N_ITER / num_threads;
    int extra = N_ITER % num_threads;

    double inicio = ahora_en_segundos();

    for (int i = 0; i < num_threads; i++) {
        args[i].iteraciones = base + (i < extra ? 1 : 0);
        pthread_create(&threads[i], NULL, io_trabajo, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    double fin = ahora_en_segundos();

    double t_ejecucion = fin - inicio;

    return t_ejecucion;

    printf("Tiempo de ejecucion con %.2d hilos: %.6f segundos\n", num_threads, t_ejecucion);
}

double io_multiprocessing(int num_proc) {
    int base  = N_ITER / num_proc;
    int extra = N_ITER % num_proc;

    double inicio = ahora_en_segundos();

    for (int p = 0; p < num_proc; p++) {
        pid_t pid = fork();

        if (pid == 0) {
            // PROCESO HIJO
            double notas[NUMERO_ALUMNOS][10];
            double media[10];
            double modas[10];
            double minimo[10];
            double maximo[10];

            int iteraciones = base + (p < extra ? 1 : 0);

            for (int i = 0; i < iteraciones; i++) {
                int numAlum = cargar_notas(CSV_NOTAS,
                                           notas, NUMERO_ALUMNOS);
                if (numAlum <= 0) continue;

                estadisticasXeval(notas, numAlum,
                                  media, modas, minimo, maximo);
            }

            _exit(0);
        }
    }

    // PADRE: esperar a todos los hijos
    for (int p = 0; p < num_proc; p++) {
        wait(NULL);
    }

    double fin = ahora_en_segundos();

    double t_ejecucion = fin - inicio;

    return t_ejecucion;

    printf("Tiempo de ejecucion con %.2d procesos: %.6f segundos\n", num_proc, t_ejecucion);
}


/*void main(void){
    //cpu_secuencial();
    //cpu_threading(30);
    //cpu_multiprocessing(10);
    //io_secuencial();
    //io_threading(10);
    //io_multiprocessing(5);
}*/

