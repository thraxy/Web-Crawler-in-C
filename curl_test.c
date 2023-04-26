/***************************************************************************
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
char *start_page = "https://www.rutgers.edu/"; // start page
 

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
    memory *mem = (memory*) ctx;
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
    memory *mem = malloc(sizeof(memory));
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
size_t follow_links(CURLM *multi_handle, memory *mem, char *url)
{
    // int opts defines the options for the HTML parser
    int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    // parse the HTML
    // the HTML is stored in the buffer
    // the HTML parser will parse the HTML and store the result in the doc
    htmlDocPtr doc = htmlReadMemory(mem->buf, mem->size, url, NULL, opts);
    // If doc is NULL, htmlReadMemory failed to parse the document.
    if(!doc)
        return 0;
    // xpath is the query language for XML and HTML
    // //a/@href is the xpath to get all the href attributes of the a tags
    // the a tags are the links
    xmlChar *xpath = (xmlChar*) "//a/@href";
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if(!result)
        return 0;
    xmlNodeSetPtr nodeset = result->nodesetval;
    if(xmlXPathNodeSetIsEmpty(nodeset)) {
        xmlXPathFreeObject(result);
        return 0;
    }
    size_t count = 0;
    int i;
    for(i = 0; i < nodeset->nodeNr; i++) {
        // randomly choose a link
        // this is important because we do not want to crawl all the links
        double r = rand();
        // we use the random number to choose a link
        int x = r * nodeset->nodeNr / RAND_MAX;
        // get the href attribute of the link node
        const xmlNode *node = nodeset->nodeTab[x]->xmlChildrenNode;
        xmlChar *href = xmlNodeListGetString(doc, node, 1);
        // if the href is relative, we build the absolute URL using the base URL
        if(follow_relative_links) {
            xmlChar *orig = href;
            // build the absolute URL
            // XMLbUILDURI is a function in libxml2 that builds an absolute URL
            href = xmlBuildURI(href, (xmlChar *) url);
            xmlFree(orig);
        }
        char *link = (char *) href;
        if(!link || strlen(link) < 20)
            continue;
        if(!strncmp(link, "http://", 7) || !strncmp(link, "https://", 8)) {
            curl_multi_add_handle(multi_handle, make_handle(link));
        if(count++ == max_link_per_page)
            break;
        }
        xmlFree(link);
    }
  xmlXPathFreeObject(result);
  return count;
}
 
int is_html(char *ctype)
{
    // check if the content type is text/html or not
    // if the content type is text/html, return 1

    return ctype != NULL && strlen(ctype) > 10 && strstr(ctype, "text/html");
}
 
int main(void)
{
    // signal handler for Ctrl-C
    // when Ctrl-C is pressed, the program will exit
    // 
    signal(SIGINT, sighandler);
    LIBXML_TEST_VERSION;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURLM *multi_handle = curl_multi_init();
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_con);
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 6L);
 
  /* enables http/2 if available */
#ifdef CURLPIPE_MULTIPLEX
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
#endif
 
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
    int pending = 0; // number of pending transfers to process
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
        CURLMsg *m = NULL;
        while((m = curl_multi_info_read(multi_handle, &msgs_left))) {
            if(m->msg == CURLMSG_DONE) {
                CURL *handle = m->easy_handle;
                char *url;
                memory *mem;
                curl_easy_getinfo(handle, CURLINFO_PRIVATE, &mem);
                curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);
            if(m->data.result == CURLE_OK) {
                long res_status;
            curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res_status);
            // check if the http status is 200 or not (200 means HTTP OK)
            if(res_status == 200) {
            //
            char *ctype;
            curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &ctype);
            printf("[%d] HTTP 200 (%s): %s\n", complete, ctype, url);
            fprintf(outputFile, "%s\n", url);
            if(is_html(ctype) && mem->size > 100) {
              if(pending < max_requests && (complete + pending) < max_total) {
                pending += follow_links(multi_handle, mem, url);
                still_running = 1;
              }
            }
          }
          else {
            printf("[%d] HTTP %d: %s\n", complete, (int) res_status, url);
            fprintf(outputFile, "%s\n", url);
          }
        }
        else {
          printf("[%d] Connection failure: %s\n", complete, url);
          fprintf(outputFile, "%s\n", url);
        }
        curl_multi_remove_handle(multi_handle, handle);
        curl_easy_cleanup(handle);
        free(mem->buf);
        free(mem);
        complete++;
        pending--;
                }
            }
        }
 
    }
fclose(urlListFile); // close the file pointer
fclose(outputFile); // close the url file
  
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
    return 0;
}