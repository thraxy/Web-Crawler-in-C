#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>

//defining the max number for each variable 
#define MAX_LINK_LEN 1020
#define MAX_LINKS 1000 
#define MAX_THREADS 10 
#define MAX_FILE_NAME 256

//array that stores the links 
char links[MAX_LINKS][MAX_LINK_LEN];

//number of links to crawl
int num_links = 0;

//to synchronize access to link(s) array
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//file that stores extracted link information
FILE *output_file;

// fucntion that processes a single link
void process_link(char *link) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, link);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

//replacing function to extract link information
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
}

// Function to be executed by each thread
void *thread_func(void *arg) {
    int link_idx;
    while (1) {
        pthread_mutex_lock(&mutex);
        if (num_links == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        link_idx = num_links - 1;
        num_links--;
        pthread_mutex_unlock(&mutex);
        process_link(links[link_idx]);
    }
    return NULL;
}

//function for links in the input file
int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }

//if input containing empty page or page with invalid links 
    FILE *input_file = fopen(argv[1], "r");
    if (!input_file) {
        printf("Error opening input file\n");
        return 1;
    }
    output_file = fopen(argv[2], "w");
    if (!output_file) {
        printf("Error opening output file\n");
        return 1;
    }
    char link[MAX_LINK_LEN];
    while (fgets(link, MAX_LINK_LEN, input_file) != NULL) {
        if (num_links == MAX_LINKS) {
            break;
        }
        int len = strlen(link);
        if (link[len - 1] == '\n') {
            link[len - 1] = '\0';
        }
        strcpy(links[num_links], link);
        num_links++;
    }
    fclose(input_file);
    pthread_t threads[MAX_THREADS];
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    fclose(output_file);
    return 0;
}
