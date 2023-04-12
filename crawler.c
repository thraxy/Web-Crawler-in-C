
// libaries

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

// define constants

#define MAX_URL_LENGTH 1000
#define MAX_THREADS 100
#define MAX_QUE_SIZE 1000

// URL Struct
typedef struct {
    char url[MAX_URL_LENGTH];
    char domain[MAX_URL_LENGTH];
} url_info;

// Queue Struct
typedef struct {
    url_info *url_array[MAX_QUE_SIZE];
    int front;
    int rear;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t empty;
    pthread_cond_t full;
} queue;

// Global Variables
queue queue;
int num_threads;

// initialize queue

void init_queue(queue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    // initialize mutex and condition variables
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->empty, NULL);
    pthread_cond_init(&q->full, NULL);
}

// check if queue is empty or full
int is_empty(queue *q) {
    return q->size == 0;
}

int is_full(queue *q) {
    return q->size == MAX_QUE_SIZE;
}

// enqueue and dequeue functions
void enqueue(queue *q, url_info *url) {
    // lock queue
    pthread_mutex_lock(&q->lock);
    // wait until queue is not full
    while (is_full(q)) {
        pthread_cond_wait(&q->full, &q->lock);
    }
    // add url to queue
    q->rear = (q->rear + 1) % MAX_QUE_SIZE;
    url.info *url = malloc(sizeof(url_info));
    strcpy(url->url, url->url);
    strcpy(url->domain, url->domain);
    q->url_array[q->rear] = url;
    q->size++;
    // signal that queue is not empty
    // unlock queue
    pthread_cond_signal(&q->empty);
    pthread_mutex_unlock(&q->lock);
}

url_info *dequeue(queue *q) {
    // lock queue
    pthread_mutex_lock(&q->lock);
    // wait until queue is not empty
    while (q->size == 0) {
        pthread_cond_wait(&q->empty, &q->lock);
    }
    url_info *url = q->url_array[q->front];
    q->front = (q->front + 1) % MAX_QUE_SIZE;
    q->size--;
    // signal that queue is not full
    // unlock queue
    pthread_cond_signal(&q->full);
    pthread_mutex_unlock(&q->lock);
    return url;
}

// crawler function
void *crawler(void *arg) {
    // initialize curl
    CURL *curl;
    CURLcode res;
    // initialize variables
    char *url;
    url_info info;
    char domain[MAX_URL_LENGTH];
    char filename[MAX_URL_LENGTH];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i;
    // initialize curl and check for errors
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: curl_easy_init() failed\n");
        exit(1);        
    }
    // while loop to crawl through urls
    while (1) {
        info = dequeue(&queue);
        url = info.url;
        // copy domain name
        strcpy(domain, info.domain);
        // set curl options
        // check for errors
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            fprintf(stderr, "Error: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            continue(1);
        }
        // open file
        // check for errors
        sprintf(filename, "%s.txt", domain);
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: fopen() failed\n");
            continue(1);
        }
        // write url to file
        // check for errors
        fprintf(fp, "%s\n", url);
        while ((read = getline(&line, &len, curl)) != NULL) {
            for (i = 0; i < read; i++) {
                if (line[i] == '<') {
                    while (i < read && line[i] != '>') {
                        i++;
                    }
                }
                // check for href
                else if (line[i] == 'h' && i + 7 < read && strncmp(&line[i], "href=", 6) == 0) {
                    i +=6;                    
                    char *start = &line[i];
                    // while loop to find end of url
                    while (i < read && line[i] != '"' && line[i] != '\0') {
                        i++;
                    }
                    // check if url is valid
                    if (i < read) {
                        line[i] = '\0';
                        // check if url is absolute or relative
                        // enqueue url
                        if (strncmp(start, "http", 4) == 0) {
                            url_info *new_url = malloc(sizeof(url_info));
                            strcpy(new_url->url, start);
                            strcpy(new_url->domain, domain);
                            enqueue(&queue, new_url);
                        } else if (strncmp(start, "/", 1) == 0) {
                            char *pos = strchr(url, '/');
                            if (pos) {
                                *pos = '\0';
                                url_info *new_url;
                                sprintf(new_url->url, "%s%s", url, start);
                                strcpy(new_url->domain, domain);
                                enqueue(&queue, new_url);
                            }

                        }
                        
                    }
                }
            }
        }
        // close file
        fclose(fp);
        if (line) {
            free(line);

        }
    }
    // cleanup curl
    // exit thread
    curl_easy_cleanup(curl);
    pthread_exit(NULL);
}

// main function
int main (int argc, char *argv[]) {
    // check for errors
    if (argc < 3) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        exit(1);
    }
    // initialize variables
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // open file
    // check for errors
    fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Error: fopen() failed\n");
        exit(1);
    }
    // initialize threads
    // check for errors
    num_threads = atoi(argv[2]);
    if (num_threads < 1 || num_threads > MAX_THREADS) {
        fprintf(stderr, "Error: Invalid number of threads\n");
        exit(1);
    }

    // initialize queue
    // check for errors
    init_queue(&queue);
    pthread_t threads[num_threads];
    int i;
    for (i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, crawler, NULL)) {
            fprintf(stderr, "Error: pthread_create() failed", i);
            exit(1);
        }
    }
    // read urls from file
    // enqueue urls
    while ((read = getline(&line, &len, fp)) != -1) {
        url_info *url;
        strcpy(url->url, line);
        strcpy(url->domain, line);
        enqueue(&queue, url);
    }
    // close file
    fclose(fp);
    if (line) {
        free(line);
    }
    // join threads
    // check for errors
    for (i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error: pthread_join() failed", i);
            exit(1);
        }
    }
}
