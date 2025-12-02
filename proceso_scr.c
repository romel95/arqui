// proceso_http.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <sys/sysinfo.h>

#define REPEAT_TIMES 160

const char *base_sites[] = {
    "https://www.jython.org",
    "http://olympus.realpython.org/dice"
};
const int base_count = 2;

const char **sites = NULL;
int num_sites = 0;

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

void worker_process(int worker_id, int num_procs) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[proc %d] No se pudo inicializar CURL\n", worker_id);
        exit(1);
    }

    for (int i = worker_id; i < num_sites; i += num_procs) {
        const char *url = sites[i];
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            printf("[proc %d] Descarga exitosa de %s\n", worker_id, url);
        } else {
            fprintf(stderr, "[proc %d] Error al descargar %s: %s\n",
                    worker_id, url, curl_easy_strerror(res));
        }
    }

    curl_easy_cleanup(curl);
    exit(0); // terminar hijo
}

int main(void) {
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

    int num_procs = get_nprocs(); // similar a cpu_count()
    printf("Creando %d procesos de trabajo\n", num_procs);

    for (int p = 0; p < num_procs; p++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            // Proceso hijo
            worker_process(p, num_procs);
        }
        // El padre sigue el loop creando más hijos
    }

    // Proceso padre espera a todos los hijos
    for (int p = 0; p < num_procs; p++) {
        wait(NULL);
    }

    free(sites);
    curl_global_cleanup();
    return 0;
}
