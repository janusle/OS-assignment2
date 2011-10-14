CC=gcc
FLAG=-g -Wall -ansi

All: os2

os2: semaphore.o main.o
	$(CC) -o os2 semaphore.o main.o $(FLAG)

semaphore.o: 
	$(CC) -c semaphore.c $(FLAG)

main.o:
	$(CC) -c main.c $(FLAG)

clean:
	-rm *.o
	-rm os2
	-rm core
