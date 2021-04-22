# makefile
ndn: ndn.c connectivity.c auxfunctions.c ndn.h
	gcc -Wall ndn.c connectivity.c auxfunctions.c ndn.h -o ndn