CC=gcc
SRC=main.c
CFLAGS=-Wall -Werror
LDLIBS=-lX11
OUT=evilsc

default:
	$(CC) -o $(OUT) $(SRC) $(CFLAGS) $(LDLIBS)
