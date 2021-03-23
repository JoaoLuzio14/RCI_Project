/******************************************************************************
 *
 * File Name: auxfunctions.c
 * Autor:  G19 (RCI 20/21) - João Luzio (IST193096) & José Reis (IST193105)
 * Last Review: 20 Mar 2021
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include "ndn.h"

int check_ip(char *full_ip){
	int i, num, dots = 0;
  char *ptr;

  if(full_ip == NULL) return 0;
  ptr = strtok(full_ip, ".");
  if(ptr == NULL) return 0;
  while(ptr){
    if(!val_number(ptr)) return 0;
    num = atoi(ptr);
    if(num >= 0 && num <= 255){
      ptr = strtok(NULL, ".");
      if(ptr != NULL) dots++;
    } 
    else return 0;
  }
  
  if(dots != 3) return 0;
  return 1;
}

int get_cmd(char *cmd){
	if(strcmp(cmd, "join") == 0) return 1;
	if(strcmp(cmd, "leave") == 0) return 2;
	if(strcmp(cmd, "exit") == 0) return 3;
	else return 0;
}

void msg_build(char* msg, char* net, char* ndIP, char* TCP){
	strcat(msg, net);
	strcat(msg, " ");
	strcat(msg, ndIP);
	strcat(msg, " ");
	strcat(msg, TCP);
  strcat(msg, "\0");
}

int val_number(char *str){
  while(*str){
    if(!isdigit(*str)) return 0;
    str++;
  }
  return 1;
}