CC = gcc
CFLAGS = -Wall -Wextra -Werror
LIBS = -lmosquitto -lcjson

all:
	$(CC) $(CFLAGS) main.c -o iot_monitor $(LIBS)

clean:
	rm -f iot_monitor
