
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
// I did not implement tidy yet but after some research I found that it is a good library for parsing html
#include <tidy/tidy.h> 
#include <tidy/buffio.h>

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
int crawler(Crawler *crawler) {

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




