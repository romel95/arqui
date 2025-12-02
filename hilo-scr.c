// hilo_http.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/sysinfo.h>   // get_nprocs() en Linux

// Número de repeticiones, similar a multiplicar la lista en Python
#define REPEAT_TIMES 160

const char *base_sites[] = {
    "https://www.jython.org",
    "http://olympus.realpython.org/dice"
};
const int base_count = 2;

const char **sites = NULL;
int num_sites = 0;

int current_index = 0;
pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;

// Callback para descartar los datos recibidos
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb; // decimos a libcurl que consumimos todo
}

void *worker(void *arg) {
    (void)arg;
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "No se pudo inicializar CURL\n");
        return NULL;
    }

    while (1) {
        int idx;

        // Tomar siguiente índice de manera segura
        pthread_mutex_lock(&index_mutex);
        if (current_index >= num_sites) {
            pthread_mutex_unlock(&index_mutex);
            break;
        }
        idx = current_index++;
        pthread_mutex_unlock(&index_mutex);

        const char *url = sites[idx];
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            printf("Descarga exitosa de %s\n", url);
        } else {
            fprintf(stderr, "Error al descargar %s: %s\n",
                    url, curl_easy_strerror(res));
        }
    }

    curl_easy_cleanup(curl);
    return NULL;
}

int main(void) {
    // Construir lista de sitios repetidos
    num_sites = base_count * REPEAT_TIMES;
    sites = malloc(num_sites * sizeof(char *));
    if (!sites) {
        perror("malloc");
        return 1;
    }
    for (int i = 0; i < num_sites; i++) {
        sites[i] = base_sites[i % base_count];
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    int num_cpus = get_nprocs(); // similar a cpu_count()
    pthread_t *threads = malloc(num_cpus * sizeof(pthread_t));
    if (!threads) {
        perror("malloc");
        return 1;
    }

    printf("Creando %d hilos de trabajo\n", num_cpus);

    // Crear hilos
    for (int i = 0; i < num_cpus; i++) {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
            perror("pthread_create");
        }
    }

    // Esperar a que terminen
    for (int i = 0; i < num_cpus; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(sites);
    curl_global_cleanup();

    return 0;
}
