/******************************************************************************
 *
 * File Name: ndn.h
 * Autor:  G19 (RCI 20/21) - João Luzio (IST193096) & José Reis (IST193105)
 * Last Review: 31 Mar 2021
 *
 *****************************************************************************/
#ifndef NDN_H_INCLUDED
#define NDN_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

typedef struct Node{
	char node_ip[20];
	char node_TCP[8];
}node_info;

/* auxfunctions */
int check_ip(char *full_ip);
int get_cmd(char *cmd);
int val_number(char *str);
void msg_build(char* msg, char* net, char* ndIP, char* TCP);

/* connectivity */
int regNODE(int regFLAG, char* net, char* nodeIP, char* nodeTCP, char* regIP, char* regUDP);

#endif // NDN_H_INCLUDED