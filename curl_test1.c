/***************************************************************************

 * To compile:

 *   gcc crawler.c $(pkg-config --cflags --libs libxml-2.0 libcurl)

 */

 

/* Parameters */

int max_con = 200;

int max_total = 1000;

int max_requests = 500;

int max_link_per_page = 5;

int follow_relative_links = 0;

char *start_page = "https://www.rutgers.edu/";



//addded pthread 

#include <pthread.h>





#include <unistd.h> 

#include <libxml/HTMLparser.h>

#include <libxml/xpath.h>

#include <libxml/uri.h>

#include <curl/curl.h>

#include <stdlib.h>

#include <string.h>

#include <math.h>

#include <signal.h>

 

 //max number of threads 

 #define NUM_THREADS 10

 

 

int pending_interrupt = 0;

void sighandler(int dummy)

{

  pending_interrupt = 1;

}

 

/* resizable buffer */

typedef struct {

  char *buf;

  size_t size;

} memory;

 

size_t grow_buffer(void *contents, size_t sz, size_t nmemb, void *ctx)

{

  size_t realsize = sz * nmemb;

  memory *mem = (memory*) ctx;

  char *ptr = realloc(mem->buf, mem->size + realsize);

  if(!ptr) {

    /* out of memory */

    printf("not enough memory (realloc returned NULL)\n");

    return 0;

  }

  mem->buf = ptr;

  memcpy(&(mem->buf[mem->size]), contents, realsize);

  mem->size += realsize;

  return realsize;

}

 

CURL *make_handle(char *url)

{

  CURL *handle = curl_easy_init();

 

  /* Important: use HTTP2 over HTTPS */

  curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);

  curl_easy_setopt(handle, CURLOPT_URL, url);

 

  /* buffer body */

  memory *mem = malloc(sizeof(memory));

  mem->size = 0;

  mem->buf = malloc(1);

  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, grow_buffer);

  curl_easy_setopt(handle, CURLOPT_WRITEDATA, mem);

  curl_easy_setopt(handle, CURLOPT_PRIVATE, mem);

 

  /* For completeness */

  curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");

  curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5L);

  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

  curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L);

  curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 2L);

  curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");

  curl_easy_setopt(handle, CURLOPT_FILETIME, 1L);

  curl_easy_setopt(handle, CURLOPT_USERAGENT, "mini crawler");

  curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

  curl_easy_setopt(handle, CURLOPT_UNRESTRICTED_AUTH, 1L);

  curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);

  curl_easy_setopt(handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);

  return handle;

}

 

/* HREF finder implemented in libxml2 but could be any HTML parser */

size_t follow_links(CURLM *multi_handle, memory *mem, char *url)

{

  int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | 

             HTML_PARSE_NOWARNING | HTML_PARSE_NONET;

  htmlDocPtr doc = htmlReadMemory(mem->buf, mem->size, url, NULL, opts);

  if(!doc)

    return 0;

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

    double r = rand();

    int x = r * nodeset->nodeNr / RAND_MAX;

    const xmlNode *node = nodeset->nodeTab[x]->xmlChildrenNode;

    xmlChar *href = xmlNodeListGetString(doc, node, 1);

    if(follow_relative_links) {

      xmlChar *orig = href;

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

  return ctype != NULL && strlen(ctype) > 10 && strstr(ctype, "text/html");

}





//adding pthreads 

pthread_t threads[NUM_THREADS];



int main(void)

{

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



/*pretty much what Im doing here is initializing a thread pool and assigning each thread a handle. Each thread will then be added to the multihandle and fetching the page. The created fetch_page function obviously is there for fetching pages, adding new links to the multihandle. Also i did add the pthread.h library, define max amount of threads which was 10*/



void *fetch_page(void *arg) {

	CURLM *multi_handle = (CURLM *)arg;

	int still_running;

	do {

		curl_multi_perform(multi_handle, &still_running);

		

		int msgs_left;

		CURLMsg *msg;

		while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {

		if (msg->msg == CURLMSG_DONE) {

			CURL *handle = msg->easy_handle;

			long response_code;

			curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);

			

			//when handling a good request

			if (response_code = 200) {

				memory *mem;

				curl_easy_getinfo(handle, CURLINFO_PRIVATE, &mem);

				char *url;

				curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);

				

				//adding new links for multihandling 

				if (is_html(NULL)) {

					size_t count = follow_links(multi_handle, mem, url);

					printf("%s - %zu added new links\n", url, count);

				}

			}

			

			curl_multi_remove_handle(multi_handle, handle);

			curl_easy_cleanup(handle);

		}

		}

		

		if (pending_interrupt) {

			printf("Interrupted...\n");

			exit(0);

		}

		

		struct timeval timeout;

		timeout.tv_sec = 1;

		timeout.tv_usec = 0;

		

		int fd;

		fd_set fdset;

	}

}

 

 //the thread pool 

 for (int i = 0; i < NUM_THREADS; i++) {

 	int rc = pthread_create(&threads[i], NULL, fetch_page, multi_handle);

 	if (rc) {

 		printf("Unable to create thread %d\n", i);

 		exit(-1);

 	}

 }

 

 //While threads complete 

 for (int i = 0; i < NUM_THREADS; i++) {

 	pthread_join(threads[i], NULL);

 }

 

 curl_multi_cleanup(multi_handle);

 curl_global_cleanup();

 xmlCleanupParser();

 

 return 0;

 

 }

 

 

 

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

 

  int msgs_left;

  int pending = 0;

  int complete = 0;

  int still_running = 1;

  while(still_running && !pending_interrupt) {

    int numfds;

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

          if(res_status == 200) {

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
