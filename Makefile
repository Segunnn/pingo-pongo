CC = g++
CFLAGS = -Wall -Wextra

all: program

program: main.cpp
	$(CC) $(CFLAGS) main.cpp -o main $(LIBS)

