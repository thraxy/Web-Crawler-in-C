
/*
***************************************************************************************
Few things about this code, there are syntax errors, and it is not complete.
After looking at the crawler template on libcurl's site it looks like libxml2 is a good
library to use for parsing the html. That way we do not have to make our own parser.
The issue now is that the code is segfaulting when it tries to parse the html.
It starts at rutgers and does output 1 link, but then it segfaults.
I am not sure if it is because of the html parser or if it is because of the way I am
using the html parser.

I am sure we can figure that out. 


***************************************************************************************
*/








/* Parameters */
int max_con = 200; // max connections
int max_total = 100; // max total requests
int max_requests = 500; // max requests per connection
int max_link_per_page = 5; // max links per page
int follow_relative_links = 1; // follow relative links
int max_threads = 10; // max threads
char *start_page = "https://www.rutgers.edu"; // start page

// libraries
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>

// pending interrupt we can set from signal handler
// to stop the program
int pending_interrupt = 0;
// signal handler
// set pending_interrupt to 1
void sighandler(int dummy)
{
 pending_interrupt = 1;
}

/* resizable buffer */
// memory struct
// we use this to store the body of the response
typedef struct {
 char *buf; // buffer
 size_t size; // size of buffer
 FILE *fp; // file pointer
} memory; // memory struct

// grow buffer function
// we use this to grow the buffer
// when we receive the body of the response
size_t grow_buffer(void *contents, size_t sz, size_t nmemb, void *ctx) 
{
 size_t realsize = sz * nmemb; // real size
 memory *mem = (memory*) ctx; // memory struct
 char *ptr = realloc(mem->buf, mem->size + realsize); // reallocate memory
 if(!ptr) { // out of memory
   /* out of memory */
   printf("not enough memory (realloc returned NULL)\n");
   return 0;
 }
 mem->buf = ptr; // set buffer to ptr (new memory)
 memcpy(&(mem->buf[mem->size]), contents, realsize); // copy contents to buffer at size position 
 mem->size += realsize; // increase size by realsize (size of contents)
 return realsize; // return realsize (size of contents)
}
// make handle function 
// we use this to make a handle
CURL *make_handle(char *url)
{
// initialize handle
 CURL *handle = curl_easy_init();
 /* Important: use HTTP2 over HTTPS */
 curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
 curl_easy_setopt(handle, CURLOPT_URL, url); // set url to handle url
 /* buffer body */
 memory *mem = malloc(sizeof(memory)); // allocate memory for memory struct
 mem->size = 0; // set size to 0
 mem->buf = malloc(4096); // allocate memory for buffer (4096 bytes) 
 curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, grow_buffer); // set write function to grow buffer function
 curl_easy_setopt(handle, CURLOPT_WRITEDATA, mem);  // set write data to memory struct 
 curl_easy_setopt(handle, CURLOPT_PRIVATE, mem); // set private data to memory struct 
 /* For completeness */
 curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, ""); // set accept encoding to empty string
 curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5L); // set timeout to 5 seconds
 curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L); // set follow location to 1 (true)
 curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L); // set max redirects to 10
 curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 2L); // set connect timeout to 2 seconds
 curl_easy_setopt(handle, CURLOPT_COOKIEFILE, ""); // set cookie file to empty string
 curl_easy_setopt(handle, CURLOPT_FILETIME, 1L); // set file time to 1 (true) 
 curl_easy_setopt(handle, CURLOPT_USERAGENT, "mini crawler"); // set user agent to mini crawler 
 curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY); // set http auth to any 
 curl_easy_setopt(handle, CURLOPT_UNRESTRICTED_AUTH, 1L); // set unrestricted auth to 1 (true)
 curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY); // set proxy auth to any
 curl_easy_setopt(handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L); // set expect 100 timeout to 0
 return handle;
}
/* HREF finder implemented in libxml2 but could be any HTML parser */
size_t follow_links(CURLM *multi_handle, memory *mem, char *url, FILE *fp) // follow links function
{
 int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR |
            HTML_PARSE_NOWARNING | HTML_PARSE_NONET; // set options for html parser
 htmlDocPtr doc = htmlReadMemory(mem->buf, mem->size, url, NULL, opts); // read memory from buffer
 if(!doc) // if doc is null
   return 0; // return 0
 xmlChar *xpath = (xmlChar*) "//a/@href"; // set xpath to href
 xmlXPathContextPtr context = xmlXPathNewContext(doc); // create new context with doc
 xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context); // evaluate expression with context
 xmlXPathFreeContext(context); // free context 
 if(!result) // if result is null
   return 0; // return 0
 xmlNodeSetPtr nodeset = result->nodesetval; // set nodeset to result nodeset
 if(xmlXPathNodeSetIsEmpty(nodeset)) { // if nodeset is empty
   xmlXPathFreeObject(result); // free result
   return 0; // return 0
 }
 size_t count = 0; // set count to 0
 int i; // for loop variable
 if(nodeset->nodeNr > 0) { // if nodeset node number is greater than 0 
   double r = rand(); // set r to random number between 0 and 1
   int x = r * nodeset->nodeNr / RAND_MAX; // set x to r * nodeset node number / RAND_MAX
   const xmlNode *node = nodeset->nodeTab[x]->xmlChildrenNode; // set node to nodeset node tab at x xml children node
   xmlChar *href = xmlNodeListGetString(doc, node, 1); // set href to node list string with doc, node, and 1
   if(follow_relative_links) { // if follow relative links is true
     xmlChar *orig = href; // set orig to href
     href = xmlBuildURI(href, (xmlChar *) url); // set href to build uri with href and url
     xmlFree(orig); // free orig
   } else { 
   char *link = (char *) href; // set link to href
   if(!link || strlen(link) < 20) // if link is null or link length is less than 20
     return 0; // return 0
   if(!strncmp(link, "http://", 7) || !strncmp(link, "https://", 8)) { // if link starts with http:// or https://
     fprintf(fp, "%s\n", link); // print link to file    
     curl_multi_add_handle(multi_handle, make_handle(link)); // add handle to multi handle with link
     count++; // increase count by 1 because we added a link
     if(count == max_link_per_page) { // if count is equal to max link per page we stop
       xmlFree(href); // free href because we are done with it
       xmlXPathFreeObject(result); // free result because we are done with it
       xmlFreeDoc(doc); // free doc because we are done with it
       return count; // return count 
   }
   }
   xmlFree(href); // free href because it is not a link
   href = NULL; // set href to null to avoid double free
 }
xmlXPathFreeObject(result); 
xmlFreeDoc(doc);
return count;
}
}
// is html function
// returns 1 if content type is html or xhtml
int is_html(char *ctype) { 
    return ctype && (strstr(ctype, "text/html") || strstr(ctype, "application/xhtml+xml")); //returning content type if it is html or xhtml
}

// main function
// this is where we start the program
// this is where we create the multi handle and add the handles to the multi handle
// this is where we set the options for the multi handle
// this is where we read the urls from the file
// this is where we create the memory struct
// this is where we create the file pointer
// this is where we open the files
// this is where we check if the files is null
// this is where we crawl the urls
int main(void)
{
// set the signal handler because we want to stop the program with ctrl+c
signal(SIGINT, sighandler);
 // libxml init and curl init
LIBXML_TEST_VERSION;
curl_global_init(CURL_GLOBAL_DEFAULT);
// create the multi handle
CURLM *multi_handle = curl_multi_init();
 // set the options for the multi handle
curl_multi_setopt(multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_con); // set max total connections to 1000
curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 6L); // set max host connections to 6
// file pointers
// we need to open the files
FILE *fp; // file pointer for output file
FILE *urlFile; // file pointer for url list file
char *urlListFileName = "urls.txt"; 
char *outputFileName = "output.txt";

// open the files
urlFile = fopen(urlListFileName, "r"); // open the url list file
if (urlFile == NULL) { // if the file is null
    printf("Error opening file.\n"); // print error message
    return(1); // return 1
}

// open the output file
 fp = fopen(outputFileName, "w"); // open the output file for writing
 if (fp == NULL) { // if the file is null
   printf("Error opening file.\n"); // print error message
   return(1); // return 1
 }
// create the memory struct for the curl write function
memory *mem = malloc(sizeof(memory)); // allocate memory for the memory struct

//read urls from file
char url[1000]; // create a char array for the url with a size of 1000
while (fgets(url, sizeof(url), urlFile) != NULL) { // while we are not at the end of the file 
   //remove newline character from url
    int len = strlen(url); // set len to the length of the url
    if (len > 0 && url[len-1] == '\n') { // if the length is greater than 0 and the last character is a newline
        url[len-1] = '\0'; // set the last character to null
}

    //add url to curl multi handle
    curl_multi_add_handle(multi_handle, make_handle(url));


/* enables http/2 if available */
#ifdef CURLPIPE_MULTIPLEX // if curl pipe multiplex is defined
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX); // set curl multi option to curl pipe multiplex
#endif // end if curl pipe multiplex is defined

/* sets html start page */
curl_multi_add_handle(multi_handle, make_handle(start_page));
int msgs_left; // create an int for the messages left
int pending = 0; // create an int for pending
int complete = 0; // create an int for complete
int still_running = 1; // create an int for still running
while(still_running && !pending_interrupt) { // while still running and pending interrupt is false
   curl_multi_perform(multi_handle, &still_running); // perform the multi handle   
    int numfds; // create an int for the number of file descriptors
    int rc = curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds); // set rc to curl multi wait with multi handle, null, 0, 1000, and numfds because we are still running
    curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds); // wait for the multi handle because we are still running
    if(rc != CURLM_OK) { // if rc is not equal to curl multi ok
        fprintf(stderr, "curl_multi_wait() failed, code %s\n", curl_multi_strerror(rc)); // print error message
        break;
    }
    if(numfds) { // if numfds is true
        rc = curl_multi_perform(multi_handle, &still_running); // set rc to curl multi perform with multi handle and still running
        if(rc != CURLM_OK) { // if rc is not equal to curl multi ok
            fprintf(stderr, "curl_multi_perform() failed, code %s.\n", curl_multi_strerror(rc)); // print error message
        break; 
    }    
}

    CURLMsg *msgg; // create a curl message
    do {
        int msgq = 0; // create an int for the message queue
        msgg = curl_multi_info_read(multi_handle, &msgq); // set msgg to curl multi info read with multi handle and message queue
        if(msgg && (msgg->msg == CURLMSG_DONE)) { // if msgg is true and the message is done
            complete++; // increment complete
        CURL *e = msgg->easy_handle; // set e to the easy handle
        int http_code; // create an int for the http code
        curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &http_code); // get the response code
        if(http_code == 200) { // if the http code is 200
            memory *mem; // create a memory struct for mem
            curl_easy_getinfo(e, CURLINFO_PRIVATE, &mem); // get the private info
            char *url; // create a char array for the url
            curl_easy_getinfo(e, CURLINFO_EFFECTIVE_URL, &url); // get the effective url
            char *ctype; // create a char array for the content type
            curl_easy_getinfo(e, CURLINFO_CONTENT_TYPE, &ctype); // get the content type
            if(is_html(ctype)) { // if the content type is html
                follow_links(multi_handle, mem, url, fp); // follow the links
            } else { 
           printf("[%d] HTTP %d (%s): %s\n", complete, http_code, ctype, url); // print the http code, content type, and url
         }
       }
       curl_multi_remove_handle(multi_handle, e); // remove the handle from the multi handle
       curl_easy_cleanup(e); // clean up the easy handle
       free(mem); // free the memory struct
       mem = NULL; // set mem to null because we are done with it
     }
   } while(msgg); // while msgg is true do the above
 }


   /* See how the transfers went */
   CURLMsg *m = NULL; // create a curl message for m and set it to null
   while((m = curl_multi_info_read(multi_handle, &msgs_left))) { // while m is not null and there are messages left to read
     if(m->msg == CURLMSG_DONE) { // if the message is done 
       CURL *handle = m->easy_handle; // set handle to the easy handle
       char *url; // create a char array for the url
       memory *mem; // create a memory struct for mem
       curl_easy_getinfo(handle, CURLINFO_PRIVATE, &mem); // get the private info for the handle and set it to mem
       curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url); // get the effective url for the handle and set it to url
       if(m->data.result == CURLE_OK) { // if the result is ok 
         long res_status; // create a long for the response status
         curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res_status); // get the response code for the handle and set it to res_status
         if(res_status == 200) { // if the response status is 200
           char *ctype; // create a char array for the content type 
           curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &ctype); // get the content type for the handle and set it to ctype
           printf("[%d] HTTP 200 (%s): %s\n", complete, ctype, url); // print the http code, content type, and url
           if(is_html(ctype) && mem->size > 100) { // if the content type is html and the size is greater than 100
             if(pending < max_requests && (complete + pending) < max_total) { // if pending is less than max requests and complete plus pending is less than max total
               pending += follow_links(multi_handle, mem, url, fp); // increment pending by the number of links followed
               still_running = 1; // set still running to 1 because we are still running
            }
            }
        }
        else {
           printf("[%d] HTTP %d: %s\n", complete, (int) res_status, url); // print the http code and url 
        }
       }
       else { // if the result is not ok 
         printf("[%d] Connection failure: %s\n", complete, url); // print the connection failure and url  
       }
       curl_multi_remove_handle(multi_handle, handle); // remove the handle from the multi handle
       curl_easy_cleanup(handle); // clean up the easy handle 
       fclose(mem->fp); // close the file pointer
       free(mem); // free the memory struct 
       mem = NULL; // set mem to null because we are done with it
       complete++; // increment complete because we are done with this url
       pending--; // decrement pending because we are done with this url
     }
   }
 }
 curl_multi_cleanup(multi_handle); // clean up the multi handle 
 fclose(fp); // close the file pointer
 fclose(urlFile); // close the url file
 return 0; // return 0 because we are done
}



