CC = gcc
CFLAGS = -Wall -Wextra -Werror
LIBS = 'pkg-config --cflags --libs libxml-2.0 libcurl'

crawler: crawler.o
    $(CC) $(CFLAGS) -o crawler crawler.o $(LIBS)

clean:
    rm -f crawler.o crawler
