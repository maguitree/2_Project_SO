CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -lrt -pthread

SRC = main.c shared_mem.c semaphores.c master.c worker.c
OBJ = $(SRC:.c=.o)

all: webserver

webserver: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o webserver
