CC=gcc
CFLAGS=-I.

all: wavfile.c
	$(CC) -shared -o libwavfile.so -fPIC wavfile.c

clean:
	rm *.so