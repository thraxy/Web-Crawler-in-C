//example of an output file
//to store info from crawled webpages
//pretty much creating a file with a name 
//from the domain of the webpage; urls will be on this file
//and any other relevant info on the webpage 

vvoid *crawler(void *arg) {

    // initialize curl
    CURL *curl;
    CURLcode res;

    // initialize variables
    url_info* info;
    char filename[MAX_URL_LENGTH];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i;

    //checking for errors
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: curl_easy_init() failed\n");
        exit(1);
    }

    // while loop to crawl through urls
    while (1) {
        info = dequeue(&queue);
        char* url = info->url;
        char* domain = info->domain;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Error: curl_easy_per
