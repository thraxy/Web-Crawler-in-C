//example of multithread function
//assigning each thread to crawl the urls 
//from the queue concurrently 

//pretty much I'm taking the number of threads as arguements 
//and assigning each thread to call the crawler function

//could be called from the main function 
//after parsing the urls

void run_crawler(int num_threads, queue* q) {
    pthread_t threads[num_threads];
    int i;

    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, crawler, q);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}