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
  char *ptr, aux[20];

  if((full_ip[0] == '.') || (full_ip[strlen(full_ip)-1] == '.')) return 0;

  strcpy(aux, full_ip);
  if(aux == NULL) return 0;
  ptr = strtok(aux, ".");
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
  if(strcmp(cmd, "st") == 0) return 4;
  if(strcmp(cmd, "sr") == 0) return 5;
	else return 0;
}

int get_msg(char *msg){
  if(strcmp(msg, "ADVERTISE") == 0) return 1;
  else return 0;
}

int val_number(char *str){
  while(*str){
    if(!isdigit(*str)) return 0;
    str++;
  }
  return 1;
}

int table_in(nodeinfo *head, nodeinfo *new){
  nodeinfo *aux1;

  aux1 = (nodeinfo *)head;
  while(aux1->next != NULL){
    if(strcmp(aux1->id , new->id) == 0){
      aux1->fd = new->fd;
      free(new);
      return 1; // new fd related with the same node
    }
    aux1 = (nodeinfo*) aux1->next;
  } 
  aux1->next = (struct nodeinfo *) new;

  return 0;
}

int table_out(nodeinfo *head, char *node_id){
  nodeinfo *aux1, *aux2;

  aux1 = (nodeinfo *)head;
  aux2 = (nodeinfo *)aux1->next;
  while(aux2->next != NULL){
    if(strcmp(aux2->id, node_id) == 0){
      aux1->next = aux2->next;
      free(aux2);
      return 0;
    }
    else{
      aux1 = (nodeinfo *)aux1->next;
      aux2 = (nodeinfo *)aux2->next;
    }
  }

  return -1;
}

int table_free(nodeinfo *head){
  nodeinfo *aux1, *aux2;
 
  aux1 = (nodeinfo *)head;
  if(aux1 != NULL) aux2 = (nodeinfo *)head->next;

  while(aux1->next!=NULL){
    if(aux1->fd != 0) close(aux1->fd);
    free(aux1);
    aux1 = aux2;
    aux2 = (nodeinfo*)aux2->next;
  }

  free(aux1);
  return -1;
}