#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//the constants 
#define MAX_URL_LENGTH 200
#define MAX_PAGES_TO_CRAWL 1000
#define NUM_THREADS 10

//url structs 
typedef struct {
    char url[MAX_URL_LENGTH];
} url_t;

typedef struct {
    url_t urls[MAX_PAGES_TO_CRAWL];
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue_t;

//queue structs 
void queue_init(queue_t* queue) {
    queue->head = 0;
    queue->tail = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

void queue_push(queue_t* queue, const char* url) {
    pthread_mutex_lock(&queue->mutex);
    while ((queue->tail + 1) % MAX_PAGES_TO_CRAWL == queue->head) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    snprintf(queue->urls[queue->tail].url, MAX_URL_LENGTH, "%s", url);
    queue->tail = (queue->tail + 1) % MAX_PAGES_TO_CRAWL;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

void queue_pop(queue_t* queue, char* url) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == queue->tail) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    snprintf(url, MAX_URL_LENGTH, "%s", queue->urls[queue->head].url);
    queue->head = (queue->head + 1) % MAX_PAGES_TO_CRAWL;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
}

queue_t url_queue;

void* worker_thread(void* arg) {
    while (1) {
        char url[MAX_URL_LENGTH];
        queue_pop(&url_queue, url);
        
        // downloading web page and extract links
        
        // store relevant information in data structure
        printf("Crawling %s\n", url);
    }
    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    queue_init(&url_queue);

    FILE* input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char url[MAX_URL_LENGTH];
    while (fgets(url, MAX_URL_LENGTH, input_file) != NULL) {
    
    
        // remove newline character from end of url
        url[strcspn(url, "\n")] = '\0';
        queue_push(&url_queue, url);
    }

    fclose(input_file);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

//
