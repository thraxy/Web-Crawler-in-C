
// Standard library includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Libaries for project
// pthread for multithreading
// curl for http requests
// tidy for html parsing
#include <pthread.h>
#include <curl/curl.h>
// // I did not implement tidy yet but after some research I found that it is a good library for parsing html
// #include <tidy/tidy.h> 
// #include <tidy/buffio.h>

// Define constants
#define MAX_URL_LENGTH 1000 // max length of a url
#define MAX_THREADS 100 // max number of threads
#define MAX_QUE_SIZE 10 // max size of queue

/*
**************************************************************************************************************

I cleaned up the code from the previous crawler.c file and added comments to explain what each part does.
I did not implement tidy yet but I think it would be a good library to use for parsing html.
We need to implement multithreading. Right now the program should only use one thread.
We need to implement an output file. Right now the program does not output "anything".
So we need to implement it where it outputs the url and the html to a file.

So we can use the other file as a reference for multithreading. Since it was messy. 
We know what parts of the code we need to use for multithreading. So we can just add those parts into this file.

What we need to my knowledge is:
1. A multithread implementation of the crawler function
2. A multithread implementation of the queue functions
3. A output function that outputs the url and html to a file
4. A parser function that parses the html and stores the urls in the queue
---This is where tidy comes in. We can use tidy to parse the html and store the urls in the queue
5. Add to main function that initializes the queue and threads and calls the crawler function

**************************************************************************************************************
*/

// main function
int main(int argc, char ** argv) {
    // Welcome message
    printf("Welcome to our Web Crawler!\n");
    printf("Please enter the number of threads you would like to use: ");
    // Get number of threads from user
    int num_threads;
    scanf("%d", &num_threads);
    // Check if number of threads is valid
    if (num_threads > MAX_THREADS) {
        printf("Number of threads is too large. Please enter a number less than %d", MAX_THREADS);
        return 1;
    }
    // Get starting URL from text file
    FILE *fp;
    char *line = NULL; // line from file
    size_t len = 0; // length of line
    ssize_t read; // result of getline()
    fp = fopen("start_url.txt", "r");
    if (fp == NULL) {
        printf("Error: fopen() failed\n");
        return 1;
    }
    // Read first line of file
    read = getline(&line, &len, fp);
    // Check if line is valid
    if (read == -1) {
        printf("Error: getline() failed\n");
        return 1;
    }
    // Check if line is too long
    if (strlen(line) > MAX_URL_LENGTH) {
        printf("Error: URL is too long\n");
        return 1;
    }
    // Remove newline character from end of line
    line[strcspn(line, " \t")] = 0;
    // Print starting URL
    printf("Starting URL: %s\n", line);
    // Close file
    fclose(fp);
}

// crawler function
// will retrieve details from a url and store it in a file
// changed to void* to be used with pthread_create
// arg is a pointer to a crawler arguments struct
void *crawler(void *arg) {

    // we need to cast arg to a crawler arguments struct
    CrawlerArgs *args = (CrawlerArgs *) arg; // cast arg to crawler arguments struct
    // initialize curl variables
    CURL *curl; // curl handle
    CURLcode res; // curl result
    curl = curl_easy_init(); // initialize curl

    int result; // result of curl request

    // if curl is initialized correctly then continue
    // this is to prevent a segfault if curl is not initialized correctly
    if (curl) {
        // set curl options
        curl_easy_setopt(curl, CURLOPT_URL, crawler->url); // set url
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // set write function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, crawler->file); // set write data

        // perform curl request and return result code
        result = curl_easy_perform(curl);

        // if curl request was successful then continue
        // we can open the file and parse the html
        if (result == CURLE_OK) {
            // success message
            printf("Crawler %d: Successfully crawled %s", crawler->id, crawler->url); // crawler id and url
        } else {
            // error message
            printf("Crawler %d: Error: curl_easy_perform() failed: %s", crawler->id, curl_easy_strerror(result)); // crawler id and error message
        }
        // cleanup curl
        // this is to prevent a memory leak
        curl_easy_cleanup(curl);
    } else {
        // error message
        printf("Crawler %d: Error: curl_easy_init() failed", crawler->id);
    }
    // return result code
    return result;

}

// queue struct
// this is a circular queue
// front is the index of the front of the queue
// rear is the index of the rear of the queue
// size is the number of items in the queue
typedef struct {
    int front, rear, size;
    unsigned capacity; // maximum number of items in the queue
    char** array; // array of items in the queue
} Queue;

// threadpool struct
// this is a threadpool
// num_threads is the number of threads in the threadpool
// threads is an array of threads in the threadpool
typedef struct {
    pthread_t threads[MAX_THREADS]; // array of threads
    int num_threads; // number of threads
} Threadpool;

// crawler arguments struct
// this is used to pass arguments to the crawler function
// id is the id of the crawler
// url is the url of the crawler
// file is the file of the crawler
typedef struct {
    int id; // crawler id
    char url[MAX_URL_LENGTH]; // url for crawler
    FILE *file; // ouput file
} Crawler;

// crawler results struct
// this is used to store the seults of the crawler thread
typedef struct {
    int result; // result of curl request
    char *url; // url of crawler that was crawled
} CrawlerResults;

// queue functions
// these functions are used to manipulate the queue
Queue* createQueue(unsigned capacity); // creates a queue with a given capacity
int isFull(Queue* queue); // returns 1 if queue is full, 0 otherwise
int isEmpty(Queue* queue); // returns 1 if queue is empty, 0 otherwise
void enqueue(Queue* queue, char* item); // adds an item to the queue
char* dequeue(Queue* queue); // removes an item from the queue
char* front(Queue* queue); // returns the item at the front of the queue
char* rear(Queue* queue); // returns the item at the rear of the queue

// createQueue function
// creates a queue with a given capacity
Queue* createQueue(unsigned capacity) {
    // allocate memory for queue
    Queue* queue = (Queue*) malloc(sizeof(Queue)); // allocate memory for queue
    // set queue properties
    queue->capacity = capacity; // set capacity
    queue->front = queue->size = 0; // set front and size to 0
    queue->rear = capacity - 1; // set rear to capacity - 1
    // allocate memory for queue array
    queue->array = (char**) malloc(queue->capacity * sizeof(char*)); // allocate memory for array
    // return queue
    return queue;
}
// isFull function
// returns 1 if queue is full, 0 otherwise
int isFull(Queue* queue) {
    // return 1 if queue is full, 0 otherwise
    return (queue->size == queue->capacity); 
}
// isEmpty function
// returns 1 if queue is empty, 0 otherwise
int isEmpty(Queue* queue) {
    // return 1 if queue is empty, 0 otherwise
    return (queue->size == 0);
}
// enqueue function
// adds an item to the queue
void enqueue(Queue* queue, char* item) { // item is a pointer to a string
    // check if queue is full
    if (isFull(queue)) {
        // error message
        printf("Error: Queue is full\n");
        return; // exit function
    }
    // increment rear index
    queue->rear = (queue->rear + 1) % queue->capacity;
    // add item to queue
    queue->array[queue->rear] = item;
    // increment size
    queue->size = queue->size + 1;
}
// dequeue function
// removes an item from the queue
char* dequeue(Queue* queue) { // returns a pointer to a string
    // check if queue is empty
    if (isEmpty(queue)) {
        // error message
        printf("Error: Queue is empty\n");
        return NULL;
    }
    // get item at front of queue
    char* item = queue->array[queue->front];
    // increment front index
    queue->front = (queue->front + 1) % queue->capacity;
    // decrement size
    queue->size = queue->size - 1;
    // return item
    return item;
}
// front function
// returns the item at the front of the queue
char* front(Queue* queue) { // returns a pointer to a string
    // check if queue is empty
    if (isEmpty(queue)) {
        // error message
        printf("Error: Queue is empty\n");
        return NULL;
    }

    // return item at front of queue
    return queue->array[queue->front];
}
// rear function
// returns the item at the rear of the queue
char* rear(Queue* queue) { // returns a pointer to a string
    // check if queue is empty
    if (isEmpty(queue)) {
        // error message
        printf("Error: Queue is empty\n");
        return NULL;
    }

    // return item at rear of queue
    return queue->array[queue->rear];
}
// writedata function
// writes data to a file
// takes a pointer to a string and a file pointer
// we do size * nmemb because curl will call this function multiple times
// and we want to write all the data to the file
// we use size_t because it is the same size as a pointer
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // get the size of the data
    size_t written = fwrite(ptr, size, nmemb, stream);
    // return the size of the data
    return written;
}
// parser function
// will parse a file and add urls to a queue
void parser(char* htmlCont, Queue* queue) {
    // initialize variables for parsing
    // will keep track of the start and end of a url
    char* start = htmlCont;
    char* end = htmlCont;
    char* url_start;
    char* url_end;
    char* url;
    char* c;
    // loop through each character in the string
    while ((c = *end++) != '\0') {
        // if the character is a '<' then continue
        if (c == '<') {
            if (strncmp(end, "a href=\"", 8) == 0) {
                // if the characters match, record the start and end of the url
                url_start = end + 8;
                url_end = strchr(url_start, '\"');
                // if the url is found, allocate memory for the url and copy it
                if (url_end != NULL) {
                    url = malloc(url_end - url_start + 1);
                    strncpy(url, url_start, url_end - url_start);
                    url[url_end - url_start] = '\0';
                    // add the url to the queue
                    enqueue(queue, url);
                    // update the end pointer to point to the end of the url
                    end = url_end + 1;
                    }
                }
            }
            
        }
}

// initialize threadpool function
// this function will initialize the threadpool
void init_threadpool(Threadpool *pool,  int num_threads) {
    // set number of threads
    pool->num_threads = num_threads;
    // loop through each thread and create it
    for (int i = 0; i < num_threads; i++) {
        // create threads
        pthread_create(&(pool->threads[i]), NULL, crawler, NULL); // create threads and pass them to crawler function
    }
}

// thread crawler function
// this function will be called by each thread
void *thread_crawler(void *arg) {
    // cast arg to crawler arguments struct
    CrawlerArgs *args = (CrawlerArgs *) arg;
    // initiatlized curl variables
    CURL *curl; // curl handle
    CURLcode res; // curl result
    curl = curl_easy_init(); // initialize curl
    CrawlerResults *results = malloc(sizeof(CrawlerResults)); // allocate memory for results
    results->results = -1; //default results to -1
    // check if curl was initialized
    // if it was, then continue
    if (curl) {
        // setting curl options
        curl_easy_setopt(curl, CURLOPT_URL, args->url); // set url
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // set write function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, args->file); // set write data
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // set follow location
        // do curl request and return result code
        results->results = curl_easy_perform(curl);
        results->url = args->url;
        // if curl request was successful, then continue
        // we can open the file and parse it
        if (results->results == CURLE_OK) {
            // success
            printf("Crawler %d: Successfully crawled %s", args->id, args->url);
        } else {
            // error
            printf("Crawler %d: Error crawling %s", args->id, args->url);
        }
        // cleanup curl
        // this is to prevent a memory leak
        curl_easy_cleanup(curl);
    } else {
        // error
        printf("Crawler %d: Error initializing curl", args->id);
    }
    // return result code
    return results;
}