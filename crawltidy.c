//web crawler 
#include <stdio.h>
#include <string.h>

//was able to use curl & tidy libraries 
//since I have them installed on my linux machine

//used for the http 
#include <curl/curl.h>

//used tidy for cleaning the html pages
//also to parse the html and extract the links 
#include <tidy/tidy.h>  
#include <tidy/buffio.h>

//implementing the crawler function 
#include "crawler.h"

//implementing input/out function
#include "io.c"

//definig the max link and url lengths 
#define MAX_LINKS 10
#define MAX_URL_LEN 500

// initializing index variable
int currtIn = 0;

//created the bufferCallback function 
//to output webpage contents 
//via buffer 
s_Tidy bufferCallback(
  char * buffer,
  s_Tidy size,
  s_Tidy new_Memb,
  TidyBuffer * tidyBuffer) {

    // append response to the buffer
    s_Tidy uptSize = size * new_Memb;
    tidyBufAppend(tidyBuffer, buffer, uptSize);

    // returning the updated size
    return uptSize;
};

// output to file
int write(char ** output) {
  for (int i = 0; i < MAX_LINKS; i ++) {
    if (output[i]) {
      printf("This is: %d: %s\n", i, output[i]);
      openFileAndWrite(OUTPUT_PATH, output[i]);
    }
  }
}

//parse content in Tidy 
//pretty much creating a tree structure in memory
//to store the website contents

void parse(TidyNode node, char ** output) {
  TidyNode child;

//for each child, 
//recursively parse all of their children

  for (child = tidy_getChild(node); child != NULL; child = tidy_Next(child)) {

    //incase href exists

    TidyAttr hrefAttr = tidy_getId(child, TidyAttr_HREF);
    if (hrefAttr) {

      // I dont remember why I did this...lol
     // but I beliebe its an additional if for outputting 
      if (currtIn < 10) {
        if (strlen(tidyAttrValue(hrefAttr)) < MAX_URL_LEN) {
          strcpy(output[currtIn], tidyAttrValue(hrefAttr));
          currtIn ++;
        }
      }
      if (tidyAttrValue(hrefAttr)) printf("The URL: %s\n", tidyAttrValue(hrefAttr));
    }

    //traversing the tree recursively
    parse(child, output);
  }
}

// storing webpage content into the buffer 
int web_Cont(Crawler crawler) {

  
  if (crawler.url) {

    // intitializing curl variables
    CURL * handle;
    handle = curl_easy_init();
    char errBuff[CURL_ERROR_SIZE];
    int res;
    TidyDoc parseDoc;
    TidyBuffer tidyBuffer = {0};
    TidyBuffer tidyErrBuff = {0};

    // when initialized 
    if (handle) {

      // different options 

      //setting the url
      curl_easy_setopt(handle, CURLOPT_URL, crawler.url);
      
      // output from the callback
      curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, bufferCallback); 
      curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuff);

      // prepping the Tidy buffer & document 
      parseDoc = tidyCreate();
      tidyBufInit(&tidyBuffer);
      

      //setting the maximum length 
      tidyOptSetInt(parseDoc, TidyWrapLen, 2048); 

     //forcing an output
      tidyOptSetBool(parseDoc, TidyForceOutput, yes);

     //finding the buffer to store the data 
      curl_easy_setopt(handle, CURLOPT_WRITEDATA, &tidyBuffer); 

      //execute 
      res = curl_easy_perform(handle);

      // checking if it was successful
      if (res == CURLE_OK) {
        printf("crawl success: %s\n", crawler.url);

        //making the content tidy friendly
        tidyParseBuffer(parseDoc, &tidyBuffer);

        //output array
        for (int i = 0; i < MAX_LINKS; i ++) {
          crawler.parsedUrls[i] = (char *) malloc(MAX_URL_LEN * sizeof(char *));
        }

	//parsing the results 
        parse(tidyGetBody(parseDoc), crawler.parsedUrls);
        crawler.parsedUrls = crawler.parsedUrls;
      } 
	//if it was unsuccessful
	
	else {
        printf("crawl failure: %s\n", crawler.url);
        return 0; 
      }

      // cleaning up
      // and closing the connections
      curl_easy_cleanup(handle);
      tidyBufFree(&tidyBuffer);
      tidyBufFree(&tidyErrBuff);
      tidyRelease(parseDoc);

//successful!
      return 1;

    }

//not successful!
    return 0; 

  }
  return 0; 

}