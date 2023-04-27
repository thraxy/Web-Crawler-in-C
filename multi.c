***************************************************************************
 * 
 * 
 * 
 * 
 * 
 * To compile:
 *      gcc crawler.c $(pkg-config --cflags --libs libxml-2.0 libcurl lpthread)
 *                          or
 *      gcc crawler.c `pkg-config --cflags --libs libxml-2.0 libcurl lpthread`
 */
 
// parameters for the crawler
int max_con = 200; // max concurrent connections
int max_total = 100; // max total connections
int max_requests = 50; // max requests per connection
int max_link_per_page = 5; // max links to follow per page
int follow_relative_links = 0; // follow relative links
// commented out because we are using 
//char *start_page = "https://www.rutgers.edu/"; // start page
 

// libraries
#include <libxml/HTMLparser.h> // html parser for html parsing
#include <libxml/xpath.h> // xpath for html parsing 
#include <libxml/uri.h>  // uri parser for relative links 
#include <curl/curl.h> //  curl library for HTTP requests and responses 
#include <stdlib.h> // standard library
#include <string.h> // string library for string operations
#include <math.h> // math library for random number generation
#include <signal.h> // signal library for interrupt handling'
#include <pthread.h> // pthread library for multithreading
 

// pending interrupt flag for interrupt handling
int pending_interrupt = 0;
// function to handle interrupt signals
// dummy parameter is required for signal handler function
// but not used in the function
void sighandler(int dummy)
{
    // set pending interrupt flag to 1 to indicate interrupt
    pending_interrupt = 1;
}
 
/* resizable buffer */
// struct to store the buffer and its size 
// this is important for the curl library to store the response
// the buffer is resized as needed
typedef struct {
  char *buf; // buffer to store the response
  size_t size; // size of the buffer
} responsememory; // struct name
 
// function to resize the buffer
// this is important for the curl library to store the response
// the buffer is resized as needed
// this helps to store the response in the buffer
size_t grow_buffer(void *contents, size_t sz, size_t nmemb, void *ctx)
{
    // calculate the size of the buffer
    // nmemb is the number of elements
    // sz is the size of each element
    // realsize is the total size of the buffer
    // ctx is the context of the buffer
    // mem is the memory struct
    size_t realsize = sz * nmemb;
    responsememory *mem = (responsememory*) ctx;
    // reallocate the buffer to the new size 
    // if the buffer is not large enough
    // if the buffer is not large enough, the buffer is reallocated
    // to the new size
    // if the buffer is large enough, the buffer is not reallocated
    // and the new data is copied to the buffer
    char *ptr = realloc(mem->buf, mem->size + realsize);
    if(!ptr) {
        /* out of memory */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    // copy the new data to the buffer
    mem->buf = ptr;
    memcpy(&(mem->buf[mem->size]), contents, realsize);
    // update the size of the buffer
    mem->size += realsize;
    // return the size of the buffer
    return realsize;
}
 
/* make a new handle for a URL */
// function to make a new handle for a URL
// this is important for the curl library to make a new handle
// for a URL
// the reason why it is important is that the handle is used to
// make the HTTP request
CURL *make_handle(char *url)
{
    // initialize the curl handle
    CURL *handle = curl_easy_init();
 
    /* Important: use HTTP2 over HTTPS */
    // set the HTTP version to HTTP2 over HTTPS
    // this is important for the curl library to use HTTP2 over HTTPS
    // the reason why it is important is that HTTP2 is a newer version
    // of HTTP and it is faster than HTTP1
    curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(handle, CURLOPT_URL, url);
 
    /* buffer body */
    // initialize the memory struct
    responsememory *mem = malloc(sizeof(responsememory));
    // set the size of the buffer to 0 
    mem->size = 0;
    // allocate the buffer to 1
    mem->buf = malloc(1);
    // set the write function to grow_buffer function 
    // we do this because we want to store the response in the buffer
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, grow_buffer);
    // set the write data to the memory struct
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, mem);
    // set the private data to the memory struct
    curl_easy_setopt(handle, CURLOPT_PRIVATE, mem);
 
    /* For completeness */
    // we do not need all these options for our crawler but we set them
    // for completeness.
    // these options are important for the curl library to make a HTTP
    // request
    // accept_encoding is set to empty string because we do not want to
    // accept any encoding
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");
    // set the timeout to 5 seconds
    // after 5 seconds, the request will timeout
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5L);
    // set the follow location to 1
    // after 1 redirect, the request will follow the redirect
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    // set the max redirects to 10
    // after 10 redirects, the request will stop
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L);
    // set the connect timeout to 2 seconds
    // after 2 seconds, the request will timeout
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 2L);
    // set the cookie file to empty string
    // we do not want to use cookies
    curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
    // set the filetime to 1
    // we want to get the file time
    curl_easy_setopt(handle, CURLOPT_FILETIME, 1L);
    // set the user agent to OS Crawler
    // a user agent is a string that identifies
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "OS Crawler");
    // set the HTTP authentication to any
    // we want to use any authentication
    curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    // set the unrestricted authentication to 1
    // we want to use unrestricted authentication
    // unrestricted authentication is a way to authenticate
    // without using a username and password
    curl_easy_setopt(handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    // set the proxy authentication to any
    // we want to use any authentication
    // we do not need this because we are not using a proxy
    // but we set it for completeness
    curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    // set the expect 100 timeout to 0
    // we do not want to wait for 100 timeout
    // 100 timeout is a timeout that is used to wait for a 100 response
    // from the server
    curl_easy_setopt(handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);
    // return the handle
    // the handle is used to make the HTTP request
    return handle;
}
 
/* HREF finder implemented in libxml2 but could be any HTML parser */
// curlm is the multi handle for the curl library
// mem is the memory struct
// url is the URL
// this function is used to find all the links in the HTML
size_t follow_links(CURLM *multi_handle, responsememory *mem, char *url)
{
    // int opts defines the options for the HTML parser
    int htmlopts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    // parse the HTML
    // the HTML is stored in the buffer
    // the HTML parser will parse the HTML and store the result in the doc
    htmlDocPtr doc = htmlReadMemory(mem->buf, mem->size, url, NULL, htmlopts);
    // If doc is NULL, htmlReadMemory failed to parse the document.
    if(!doc)
        return 0;
    // xpath is the query language for XML and HTML
    // //a/@href is the xpath to get all the href attributes of the a tags
    // the a tags are the links
    xmlChar *xpath = (xmlChar*) "//a/@href";
    // context is the context for the xpath
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    // if the context is NULL, xmlXPathNewContext failed to create the context
    xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
    // if the result is NULL, xmlXPathEvalExpression failed to evaluate the xpath
    // free the context
    xmlXPathFreeContext(context);
    if(!result)
        return 0;
    // nodeset is the set of nodes that match the xpath
    xmlNodeSetPtr nodeset = result->nodesetval;
    // if the nodeset is empty, xmlXPathNodeSetIsEmpty failed to check if the nodeset is empty
    if(xmlXPathNodeSetIsEmpty(nodeset)) {
        xmlXPathFreeObject(result);
        return 0;
    }
    // count is the number of links
    size_t count = 0;
    // i is the index of the link
    int i;
    for(i = 0; i < nodeset->nodeNr; i++) {
        // randomly choose a link
        // this is important because we do not want to crawl all the links
        double ranNum = rand();
        // we use the random number to choose a link
        int ranLink = ranNum * nodeset->nodeNr / RAND_MAX;
        // get the href attribute of the link node
        const xmlNode *node = nodeset->nodeTab[ranLink]->xmlChildrenNode;
        xmlChar *href = xmlNodeListGetString(doc, node, 1);
        // if the href is relative, we build the absolute URL using the base URL
        if(follow_relative_links) {
            xmlChar *orig = href;
            // build the absolute URL
            // XMLbUILDURI is a function in libxml2 that builds an absolute URL
            href = xmlBuildURI(href, (xmlChar *) url);
            xmlFree(orig);
        }
        // if the href is NULL, we skip the link
        char *link = (char *) href;
        if(!link || strlen(link) < 20)
            continue;
        // if the link is not a HTTP or HTTPS link, we skip the link
        if(!strncmp(link, "http://", 7) || !strncmp(link, "https://", 8)) {
            // add the link to the multi handle
            curl_multi_add_handle(multi_handle, make_handle(link));
        // if count is greater than the max link per page, we stop
        if(count++ == max_link_per_page)
            break;
        }
        // free the link
        xmlFree(link);
    }
    // free the result
  xmlXPathFreeObject(result);
  // free the doc
  return count;
}
 
int htmlVerif(char *ctype)
{
    // check if the content type is text/html or not
    // if the content type is text/html, return 1
    return ctype != NULL && strlen(ctype) > 10 && strstr(ctype, "text/html");
}
 
int main(void)
{
    // signal handler for Ctrl-C
    // when Ctrl-C is pressed, the program will exit
    signal(SIGINT, sighandler);
    // libxml test version is used to check if the libxml2 version is compatible
    LIBXML_TEST_VERSION;
    // initialize the curl library globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    // multi handle is the handle for the curl library for multiple handles
    CURLM *multi_handle = curl_multi_init();
    // max_con is the maximum number of connections
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_con);
    // max_host_con is the maximum number of connections per host
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 6L);
 
  /* enables http/2 if available */
#ifdef CURLPIPE_MULTIPLEX
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
#endif
    // lock thread  
    pthread_mutex_lock(&lock); 
    FILE *urlListFile; // file pointer for output file
    FILE *outputFile; // file pointer for url list file
    char *urlListFileName = "urls.txt"; 
    char *outputFileName = "output.txt";
  

  // open the files
    urlListFile = fopen(urlListFileName, "r"); // open the url list file
    if (urlListFile == NULL) { // if the file is null
        printf("Error opening file.\n"); // print error message
        return(1); // return 1
    }

  // open the output file
    outputFile = fopen(outputFileName, "w"); // open the output file for writing
    if (outputFile == NULL) { // if the file is null
        printf("Error opening file.\n"); // print error message
        return(1); // return 1
    }
  
  //fprintf(outputFile, "TEST\n"); 
   
    char url[1000]; // create a char array for the url with a size of 1000
    while (fgets(url, sizeof(url), urlListFile) != NULL) { // while we are not at the end of   the file 
    //remove newline character from url
  	    int len = strlen(url); // set len to the length of the url
  	    if (len > 0 && url[len-1] == '\n') { // if the length is greater than 0 and the last character is a newline
  		    url[len-1] = '\0'; // set the last character to null
        }

    /* sets html start page */
    curl_multi_add_handle(multi_handle, make_handle(url));
    
    int msgs_left; // number of messages left to process
    int inprogress = 0; // number of pending transfers to process
    int complete = 0; // number of transfers completed so far
    int still_running = 1; // number of transfers still in progress
    while(still_running && !pending_interrupt) {
        // numfds will be set to the maximum file descriptor value + 1
        int numfds;
        // we start some action by calling perform right away
        // wait for activity, timeout or "nothing" (ie libcurl timers) 
        // timeout is set to 1 second 
        // after this, numfds is set to the maximum file descriptor value + 1
        // numfds is an input argument for select function
        // file descriptors are used for input/output operations
        curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
        curl_multi_perform(multi_handle, &still_running);
 
        /* See how the transfers went */
        CURLMsg *multir = NULL;
        // curl_multi_info_read returns CURLMsg structure pointer if there is a message to read
        while((multir = curl_multi_info_read(multi_handle, &msgs_left))) {
            // if the message is done
            if(multir->msg == CURLMSG_DONE) {
                // get the handle of the message
                CURL *handle = multir->easy_handle;
                char *url;
                responsememory *mem;
                // get the private data of the handle
                curl_easy_getinfo(handle, CURLINFO_PRIVATE, &mem);
                // get the url of the handle
                curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);
            // get the result of the message (CURLE_OK means no error)
            if(multir->data.result == CURLE_OK) {
                long res_status;
                // get the http status code of the message (200 means HTTP OK)
            curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res_status);
            // check if the http status is 200 or not (200 means HTTP OK)
            if(res_status == 200) {
            //
            char *ctype;
            // get the content type of the message
            curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &ctype);
            // print the url, content type, and the size of the content
            printf("[%d] HTTP 200 (%s): %s\n", complete, ctype, url);
            // print the url to the output file
            fprintf(outputFile, "%s\n", url);
            // if the content type is text/html and the size of the content is greater than 100
            if(htmlVerif(ctype) && mem->size > 100) {
                // inprogress is the number of pending transfers to process
                // if the number of pending transfers to process is less than the maximum number of requests and the 
                // number of completed transfers plus the number of pending transfers to process is less than the maximum number of total transfers
              if(inprogress < max_requests && (complete + inprogress) < max_total) {
                inprogress += follow_links(multi_handle, mem, url);
                still_running = 1;
              }
            }
          }
          // else if the http status is not 200
          else {
            // print the url and the http status code to the console
            printf("[%d] HTTP %d: %s\n", complete, (int) res_status, url);
               // print the url to the output file 
            fprintf(outputFile, "%s\n", url);
          }
        }
        // else if there is an error with the message
        else {
            // print the url and the error message to the console
          printf("[%d] Connection failure: %s\n", complete, url);
          fprintf(outputFile, "%s\n", url);
        }
        // remove the handle from the multi handle
        curl_multi_remove_handle(multi_handle, handle);
        // free the memory of the handle
        curl_easy_cleanup(handle);
        // free the memory of the response
        free(mem->buf);
        // free the memory of the response
        free(mem);
        // increment the number of completed transfers
        complete++;
        inprogress--;
                }
            }
        }
 
    }
    fclose(urlListFile); // close the file pointer
    fclose(outputFile); // close the url file
    // free the multi handle and the curl library
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
    pthread_mutex_unlock(&lock); // unlock thread
    pthread_mutex_destroy(&lock); // destroy thread
    return 0;
}