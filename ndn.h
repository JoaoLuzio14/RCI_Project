/******************************************************************************
 *
 * File Name: ndn.h
 * Autor:  G19 (RCI 20/21) - João Luzio (IST193096) & José Reis (IST193105)
 * Last Review: 20 Mar 2021
 *
 *****************************************************************************/
#ifndef NDN_H_INCLUDED
#define NDN_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

typedef struct Node{
	char node_ip[64];
	char node_TCP[64];
}node_info;


int check_ip(char *full_ip);
int get_cmd(char *cmd);
int val_number(char *str);
void msg_build(char* msg, char* net, char* ndIP, char* TCP);

#endif // NDN_H_INCLUDED