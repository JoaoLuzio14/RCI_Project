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
	char node_tcp[8];
}contact;

typedef struct Expedition{
	char id[10];
	int fd;
	struct nodeinfo *next;
}nodeinfo;

/* auxfunctions */
int check_ip(char *full_ip);
int get_cmd(char *cmd);
int get_msg(char *msg);
int val_number(char *str);
int table_in(nodeinfo *head, nodeinfo *new);
int table_out(nodeinfo *head, char *node_id);
int table_free(nodeinfo *head);

/* connectivity */
int tcp_connection(char* bootIP, char* bootTCP);
int regNODE(int regFLAG, char* net, char* nodeIP, char* nodeTCP, char* regIP, char* regUDP);
int getEXT(char* net, char* regIP, char* regUDP, char* bootIP, char* bootTCP);

#endif // NDN_H_INCLUDED
