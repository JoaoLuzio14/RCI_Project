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
	int num, dots = 0;
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
  if(strcmp(cmd, "sc") == 0) return 6;
  if(strcmp(cmd, "create") == 0) return 7;
  if(strcmp(cmd, "get") == 0) return 8;
  if(strcmp(cmd, "so") == 0) return 9;
  if(strcmp(cmd, "remove") == 0) return 10;
	else return 0;
}

int get_msg(char *msg){
  if(strcmp(msg, "ADVERTISE") == 0) return 1;
  if(strcmp(msg, "WITHDRAW") == 0) return 2;
  if(strcmp(msg, "EXTERN") == 0) return 3; 
  if(strcmp(msg, "INTEREST") == 0) return 4;
  else return 0;
}

int val_number(char *str){
  while(*str){
    if(!isdigit(*str)) return 0;
    str++;
  }
  return 1;
}

nodeinfo *table_in(nodeinfo *head, nodeinfo *new){
  nodeinfo *aux1, *aux2;

  aux2 = (nodeinfo *)head;
  aux1 = (nodeinfo *)head->next;
  if(strcmp(aux2->id, new->id) == 0){
    free(new);
    return head; 
  }
  while(aux1 != NULL){
    if(strcmp(aux1->id , new->id) == 0){
      aux1->fd = new->fd;
      free(new);
      return head;
    }
    aux1 = (nodeinfo*) aux1->next;
    aux2 = (nodeinfo*) aux2->next;
  } 
  aux2->next = (struct nodeinfo *)new;

  return head;
}

nodeinfo *table_out(nodeinfo *head, char *node_id){
  nodeinfo *aux1, *aux2;

  aux1 = (nodeinfo *)head;
  aux2 = (nodeinfo *)aux1->next;
  if(aux2 == NULL) return head;
  while(aux2 != NULL){
    if(strcmp(aux2->id, node_id) == 0){
      aux1->next = aux2->next;
      free(aux2);
      return head;
    }
    else{
      aux1 = (nodeinfo *)aux1->next;
      aux2 = (nodeinfo *)aux2->next;
    }
  }

  return head;
}

void table_free(nodeinfo *head){
  nodeinfo* tmp;
  while (head != NULL){
    tmp = head;
    head = (nodeinfo *)head->next;
    free(tmp);
  }
}

void cache_in(char cache[CACHESIZE][64], char *object){ // LIFO Regime
  int i;
  for(i = 0; i < CACHESIZE; i++){
    if((cache[i][0] == '\0') || (strcmp(cache[i], object) == 0)){
      strcpy(cache[i], object);
      break;
    }
  }
  if(i == CACHESIZE){
    for(i = 0; i < (CACHESIZE-1); i++) strcpy(cache[i], cache[i+1]);
    strcpy(cache[CACHESIZE - 1], object);
  }
}

int name_split(char *name, char *id, char *subname){

  char *token, *ptr, aux[64];

  strcpy(aux, name);
  ptr = &name[0];
  while(*ptr != '\0'){
    if(*ptr == '.') break;
    ptr++;
  }
  if(*ptr == '\0'){
    printf("\tInvalid command syntax. The object name must be specified as: 'id.subname'.\n");
    return 1;
  }
  token = strtok(aux, ".");
  strcpy(id, token);
  token = strtok(NULL, "\0");
  strcpy(subname, token);

  return 0;
}

int writeTCP(int fd, char *buffer){ // Writes to TCP connection socket

  int l, res;
  char *ptr;

  ptr = &buffer[0];
  l = strlen(buffer);
  while(l > 0){
    res = write(fd, ptr, l);
    if(res <= 0) return res;
    l-=res;
    ptr += res;
  }
  return 1;
}

int readTCP(int fd, char *buffer){ // Reads from TCP connection socket

  int res;
  char *ptr;

  ptr = &buffer[0];
  while(1){
    res = read(fd, ptr, sizeof(buffer));
    ptr += res-1;
    if(res <= 0) return res;
    else if(*ptr == '\n') break;
    ptr++;
  }
  return 1;
}