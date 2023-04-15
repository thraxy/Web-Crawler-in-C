//My example code for parser function
//that parses the url from the input
//This can be called from the main function after queue is initialized

void parse_file(const char* filename, queue* q) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: failed to open file %s\n", filename);
        exit(1);
    }

    char url[MAX_URL_LENGTH];
    while (fgets(url, MAX_URL_LENGTH, fp)) {

        // remove newline character at the end of the url
        url[strcspn(url, "\n")] = 0;

        // create a new url_info object and enqueue it
        url_info* info = malloc(sizeof(url_info));
        strcpy(info->url, url);

        // extract domain from url
        strcpy(info->domain, domain);
        enqueue(q, info);
    }

    fclose(fp);
}