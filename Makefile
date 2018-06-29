CC := gcc

interface: interface.o lib.o util_sgio.o
	$(CC) -o interface interface.o lib.o util_sgio.o -I/root/scsi/jsmn -ljsmn

interface.o: interface.c
	$(CC) -c interface.c -I/root/scsi/jsmn -ljsmn

lib.o: lib.c
	$(CC) -c lib.c

sigo: sgio.o util_sgio.o
	$(CC) -o sgio sgio.o util_sgio.o

sigo.o: sgio.c
	$(CC) -c sgio.c

util_sgio.o: util_sgio.c
	$(CC) -c util_sgio.c
