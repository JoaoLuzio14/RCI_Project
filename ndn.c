/* Comment Header */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

#define IP "139.136.138.142"
#define PORT "59000"

int check_ip(char *full_ip);
int get_cmd(char *cmd);
void msg_build(char* msg, char* net, char* ndIP, char* TCP);

int main(int argc, char **argv){

	/* Common Variables */
	int i, errcode;
	char argvector[5][8] = {"Name", "IP", "TCP", "regIP", "regUDP"}; // Desired Arguments Description
	char nodeIP[20], nodeTCP[20];

	/* User Interface Variables */
	char user_str[64], cmd[64], net[64], nodeID[64], bootIP[64], bootTCP[64];
	int cmd_code, joined = 0;

	/* UDP Server Variables */
	struct addrinfo hints, *res;
	struct sockaddr addr;
	int fd;
	ssize_t n;
	socklen_t addrlen;
	char buffer[128+1];

	/* TCP Server Variables */	

	/* Argument Process */	
	/*
	if(argc < 3){
		printf("Invalid number of arguments. Very few arguments inserted.\nThe ideal executable command is: 'ndn IP TCP regIP regUDP'\n");
		exit(1);
	}
	else if(argc > 5){
		printf("Invalid number of arguments. Too many arguments inserted.\nThe ideal executable command is: 'ndn IP TCP regIP regUDP'\n ");
	}
	else if(argc == 3){
		printf("Valid number of arguments. Some arguments might have been ommited and some values will be set by deafault.\n");
	}
	else if(argc == 4){
		printf("Valid number of arguments. Some arguments might have been ommited and some values will be set by deafault.\n");
	}
	else if(argc == 5){
		printf("Valid number of arguments.\n");
		printf("Arguments are valid.\n");
	}
	*/

	strcpy(nodeIP, argv[1]);
	strcpy(nodeTCP, argv[2]);

	/* UDP Node Server Connection Setup */
	fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	if(fd == -1) exit(1);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP socket

	errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", "59000", &hints, &res);
	if(errcode != 0) exit(1);
	
	/* User Interface */
	while(1){
		printf(">>>");
		if(fgets(user_str, 64, stdin)!= NULL){
			errcode = sscanf(user_str, "%s", cmd);
			if(errcode != 1) continue;
			cmd_code = get_cmd(cmd); // Get command code
		}

   		switch(cmd_code){
   			case 1:
   				if(joined != 0){ // Already joined?
   					printf("Node already joined a net!\n");
   					break;
   				}

   				errcode = sscanf(user_str, "%s %s %s %s %s", cmd, net, nodeID, bootIP, bootTCP);
   				if((errcode != 3) && (errcode != 5)){
   					printf("Invalid command syntax.\n");
   					break;
   				}
   				else{

   					strcpy(user_str, "REG ");
   					cmd[0] = '\0';
   					msg_build(cmd, net, nodeIP, nodeTCP);
   					strcat(user_str, cmd);
   					//printf("%s\n", user_str);
   					n = sendto(fd, user_str, strlen(user_str), 0, res->ai_addr, res->ai_addrlen);
					if(n == -1) exit(1);
					
					addrlen = sizeof(addr);
					memset(buffer, '0', sizeof(buffer));
					n = recvfrom(fd, buffer, strlen(buffer)-1, 0, &addr, &addrlen);
					if(n == -1) exit(1);
					buffer[n] = '\0';

					if(strcmp(buffer, "OKREG") == 0) joined = 1;
					else{
						printf("The connection to the net was not successful.\n");
						break;
					} 

   				}

   				if(errcode == 3){ // Indirect Join

   				}
   				else if(errcode == 5){ // Direct Join

   				}

   				break;

   			case 2:
   				if(joined != 1){ // Not joined yet?
   					printf("Node does not have joined any net!\n");
   					break;
   				}

   				memset(user_str, '0', strlen(user_str));

				strcpy(user_str, "UNREG ");
   				cmd[0] = '\0';
   				msg_build(cmd, net, nodeIP, nodeTCP);
   				strcat(user_str, cmd);	
   				//printf("%s\n", user_str);

   				n=sendto(fd,user_str, strlen(user_str),0,res->ai_addr, res->ai_addrlen);
   					if(n==-1) exit(1);

   				addrlen = sizeof(addr);
   				memset(buffer, '0', sizeof(buffer));
   				n = recvfrom(fd, buffer, strlen(buffer)-1, 0, &addr, &addrlen);
   				if(n==-1) exit(1);
   				buffer[n]='\0';
   				//printf("%s\n", buffer);

   				if((strcmp(buffer, "OKUNREG"))==0){
   					joined=0;
   					memset(net, '0', sizeof(net));
					memset(user_str, '0', sizeof(user_str));
   				}
   				else{
   					printf("You are still logged in! ERROR");	
   				}

   				break;

   			case 3:
   				exit(0);
   				break;

   			default:
   				printf("Invalid command.\n");
   				break;
   		}

	}

	/* UDP Node Server Connection Close */
	freeaddrinfo(res);
	close(fd);
	exit(0);
}

int check_ip(char *full_ip){

	return 0;
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