CC = gcc
CFLAGS = -std=c99 -O3 -s -Wall -Wextra
VERSION = \"0.1.0\"

random: random.c
	$(CC) $(CFLAGS) $< -o $@ -DVERSION=$(VERSION)
