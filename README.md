# Web-Crawler-in-C

A web crawler is a program that automatically navigates the internet and it's pages and extracts useful information about them.

## Goal

The goal of this project is to develop a multithreaded web crawler that can efficiently crawl and extract information from multiple web pages simultaneously

## Requirements

1. The crawler should be able to crawl multiple web pages concurrently.
2. The crawler shoud be able to extract and store relevant information such as any links present on the page.
3. The crawler should be able to follow links on the page to other pages and continue the crawling process.
4. The crawler should be able to handle errors and exceptions:
      * Invalid URLs
      * Unavailable pages
5. The extracted information should be stored in an appropriate data structure, such as a database or file.
6. The input must be in the form of a text file containing multiple links/URLs which will be parsed as input to the program.

## How we are going to approach the project

1. We believe it would be good for us to begin with a single threaded design before attempting to create a multithreaded crawler.
2. We will be using a queue data structure for the list of URLs.
      * The first URL will lead to more URLs that were found on the page
3. We will need to implement locks for when we approach the multithreaded design. 
      * Locks are usually used to give exclusive access to a shared resource.
      * Some locks allow concurrent access to a shared resource
4. Once we have a functional and working project, if we have time we will implement a ranking algorithm.
      * Since the ranking algorithm is a procedure that ranks items in a set of data according to some criteria, we will need to make the decision what criteria is important and worth ranking.

