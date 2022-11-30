TEAM = NOBODY
VERSION = 1
HANDINDIR = ~/handin

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -pthread

OBJS = proxy.o csapp.o

all: a.out

a.out: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c csapp.c

proxy.o: proxy.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c proxy.c

handin:
	cp proxy.c $(HANDINDIR)/$(TEAM)-$(VERSION)-proxy.c

clean:
	rm -f *~ *.o proxy core

