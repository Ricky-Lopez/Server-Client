CC = gcc
CCFLAGS = -std=c99

serve: DUMBserver.c
	$(CC) $(CCFLAGS) -o DUMBserve DUMBserver.c -lpthread

clean:
	rm DUMBserve
