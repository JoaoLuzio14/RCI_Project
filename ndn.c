/******************************************************************************
 *
 * File Name: ndn.c
 * Autor:  G19 (RCI 20/21) - João Luzio (IST193096) & José Reis (IST193105)
 * Last Review: 31 Mar 2021
 *
 *****************************************************************************/
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
#include "ndn.h"

#define max(A,B) ((A)>=(B)?(A):(B))
#define DEFAULT_HOST "tejo.tecnico.ulisboa.pt"
#define DEFAULT_IP "193.136.138.142"
#define DEFAULT_PORT "59000"
#define BUFFERSIZE 1024

int main(int argc, char **argv){

	/* Common Variables */
	int i, errcode, endFLAG = 0;
	char nodeIP[20], nodeTCP[20], regIP[20], regUDP[20], *token;
	char matrix[BUFFERSIZE][BUFFERSIZE];
	fd_set ready_sockets;
	enum {unreg, reg, busy, getout, handle} state;
	int maxfd, cntr;

	/* User Interface Variables */
	char user_str[64], cmd[64], net[64], nodeID[64], bootIP[64], bootTCP[64], except_id[64];
	int fd, except_fd, aux_fd, cmd_code, counter, joined = 0;
	struct sockaddr addr_tcp;
	socklen_t addrlen_tcp;
	fd_set rfds;
	struct timeval tv;
	struct sigaction act;

	/* Node Topology Variables */
	contact extern_node, backup_node, intern[5];

	/* Expedition Tables Variables */
	nodeinfo *head_table, *new_table, *aux_table;

	/* TCP Server Variables */	
	struct addrinfo hints, *res;
	ssize_t n;
	char buffer[128+1];
	int fd_server;

	/* Argument Process */	
	printf("\n");
	if(argc < 3){
		printf("Invalid number of arguments. Very few arguments inserted.\nTypical usage: 'ndn IP TCP regIP regUDP'.\n");
		exit(1);
	}
	else if(argc > 5){
		printf("Invalid number of arguments. Too many arguments inserted.\nTypical usage: 'ndn IP TCP regIP regUDP'.\n ");
		exit(1);
	}
	else if(argc == 3){
		printf("Valid number of arguments. Some arguments might have been ommited and some values will be set by deafault.\n");
		strcpy(regIP, DEFAULT_IP);
		strcpy(regUDP, DEFAULT_PORT);
	}
	else if(argc == 4){
		printf("Valid number of arguments. Some arguments might have been ommited and some values will be set by deafault.\n");
		strcpy(regIP, argv[3]);
		strcpy(regUDP, DEFAULT_PORT);
	}
	else if(argc == 5){
		printf("Valid number of arguments.\n");
		strcpy(regIP, argv[3]);
		strcpy(regUDP, argv[4]);
	}

	strcpy(nodeIP, argv[1]);
	strcpy(nodeTCP, argv[2]);
	if((check_ip(nodeIP) == 0) || (check_ip(regIP) == 0)){
		printf("Error in IP format verification.\n");
		exit(1);
	}
	if((atoi(regUDP) < 0) || (atoi(regUDP) > 65535)){
		printf("Error specifying UDP port.\n");
		exit(1);
	}

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	printf("Arguments are valid.\n\n\n\n\n");

	for(i = 0; i < 5; i++) intern[i].fd = 0;

	/* TCP Server Connection */

	if((fd_server=socket(AF_INET,SOCK_STREAM,0)) == -1) exit(1); //TCP type of socket

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; //IPv4
	hints.ai_socktype = SOCK_STREAM; //TCP socket
	hints.ai_flags = AI_PASSIVE;

	if((errcode=getaddrinfo(NULL, nodeTCP, &hints, &res))!=0){
		fprintf(stderr,"TCP error: getaddrinfo: %s\n",gai_strerror(errcode));
	}
 
	if (bind(fd_server, res->ai_addr, res->ai_addrlen)==1){
		printf("Error registering server address with the system (bind)");
		exit(1);
	}
	if (listen(fd_server, 10) == -1){
		printf("Error in instructing the kernal to accept incoming connections (listen)");
	}

	/* User Interface */
	state = unreg; // Inicial State Defined
	// SIGPIPE Ignore
	memset(&act,0,sizeof act);
	act.sa_handler=SIG_IGN;
	if(sigaction(SIGPIPE,&act,NULL)==-1) exit(1);
	// Command Line Prompt
	printf("Node Interface:\n");
	printf(">>> ");
	fflush(stdout);

	while(1){
		FD_ZERO(&ready_sockets);
		switch(state){
			case unreg: // Node not registered
				FD_SET(0, &ready_sockets);
				maxfd = 0;
				break;
			case busy: // Waiting to receive information from new connection
				FD_SET(fd, &ready_sockets);
				maxfd = fd;
				break;
			case reg: // Node fully operational
				FD_SET(0, &ready_sockets);
				FD_SET(fd_server, &ready_sockets);
				maxfd = max(0, fd_server);
				if(head_table != (nodeinfo *)NULL){
					aux_table = head_table;
					while(aux_table != NULL){
						if(aux_table->fd != 0){
							FD_SET(aux_table->fd, &ready_sockets);
							maxfd = max(aux_table->fd, maxfd);
						}
						aux_table = (nodeinfo *)aux_table->next;
					}
				}
				break;
			case getout: // Exit from the Application
				endFLAG = 1;
				break;
			case handle:
				head_table = table_out(head_table, except_id); // Remove from expedition table
				for(i = 0; i < 5; i++){
					if(intern[i].fd == except_fd){
						intern[i].fd = 0;
						break;
					}
				}
				// Remove all nodes related with except_fd
				aux_table  = head_table;
				while(aux_table != NULL){
					if(aux_table->fd == except_fd){
						head_table = table_out(head_table, aux_table->id); // Remove from expedition table
					}
					aux_table = (nodeinfo *)aux_table->next; 
				}
				// Propagate WITHDRAW
				bzero(buffer, sizeof(buffer));
				sprintf(buffer, "WITHDRAW %s\n", except_id);
				new_table = (nodeinfo*)head_table->next;
				while(new_table != NULL){
					n = write(new_table->fd, buffer, sizeof(buffer));
					if(n <= 0){
						printf("\tError sending node info!\n");
						break;
					}					
					new_table = (nodeinfo*)new_table->next;
				}
				if(except_fd == extern_node.fd){ // Node that left was external neighbour
					// Connect to backup
					cmd_code = 0;
					if((strcmp(backup_node.node_ip, nodeIP) == 0) && (strcmp(backup_node.node_tcp, nodeTCP) == 0)){ // Back Node is the Node
						bootIP[0] = '\0';
						bootTCP[0] = '\0';
						cmd_code = getEXT(net, regIP, regUDP, bootIP, bootTCP, nodeIP, nodeTCP, 1);
						extern_node.node_ip[0] = '\0';
						extern_node.node_tcp[0] = '\0';
						if(cmd_code == -1) break;
						else if(cmd_code == 2){
							backup_node.node_ip[0] = '\0';
							backup_node.node_tcp[0] = '\0';
							extern_node.fd = 0;	
						}
						else{
							for(i = 0; i < 5; i++){
								if(intern[i].fd != 0){
									strcpy(extern_node.node_ip, intern[i].node_ip);
									strcpy(extern_node.node_tcp, intern[i].node_tcp);
									extern_node.fd = intern[i].fd;	
									// Propagate EXTERN
									bzero(buffer, sizeof(buffer));
									sprintf(buffer, "EXTERN %s %s\n", intern[i].node_ip, intern[i].node_tcp);
									new_table = (nodeinfo*)head_table->next;
									while(new_table != NULL){
										n = write(new_table->fd, buffer, sizeof(buffer));
										if(n <= 0){
											printf("\tError sending node info!\n");
											break;
										}					
										new_table = (nodeinfo*)new_table->next;
									}			
									break;
								}
							}
							if(i == 5){
								backup_node.node_ip[0] = '\0';
								backup_node.node_tcp[0] = '\0';
								extern_node.fd = 0;
							}
						}
						state = reg;
						endFLAG = 2;
						break;
					}
					else{
						strcpy(extern_node.node_ip, backup_node.node_ip);
						strcpy(extern_node.node_tcp, backup_node.node_tcp);
					}
					// Establish Connection
					fd = tcp_connection(extern_node.node_ip, extern_node.node_tcp);
					extern_node.fd = fd;
					// Send Self Info to External
					bzero(buffer, sizeof(buffer));
					sprintf(buffer, "NEW %s %s\n", nodeIP, nodeTCP);
					n = write(fd, buffer, sizeof(buffer));
					if(n <= 0){
						printf("\tError sending node info!\n");
						break;
					}

					FD_ZERO(&rfds);
					FD_SET(fd, &rfds);

					counter = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
					if(counter <= 0){
						printf("\tError establishing connection with new external neighbour node!\n");
						close(fd);
						exit(1);
						break;
					}
					bzero(buffer, sizeof(buffer));
					n = read(fd, buffer, sizeof(buffer));
					if(n <= 0){
						printf("%s\n", strerror(errno));
						printf("\tError receiving node info!\n");
						break;
					}
					// Get External Neighbour Info
					if((sscanf(buffer, "%s %s %s", user_str, backup_node.node_ip, backup_node.node_tcp) != 3) || (strcmp(user_str, "EXTERN") != 0) || (n <= 0)){
						printf("\tError getting backup node information.\n");
						close(fd);
						exit(1);
						break;
					}
					else backup_node.fd = 0;	
					// Propagate EXTERN
					bzero(buffer, sizeof(buffer));
					sprintf(buffer, "EXTERN %s %s\n", extern_node.node_ip, extern_node.node_tcp);
					new_table = (nodeinfo*)head_table->next;
					while(new_table != NULL){
						n = write(new_table->fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError sending node info!\n");
							break;
						}					
						new_table = (nodeinfo*)new_table->next;
					}								
					// Propagate ADVERTISE
					new_table = head_table;
					while(new_table != NULL){
						bzero(buffer, sizeof(buffer));
						sprintf(buffer, "ADVERTISE %s\n", new_table->id);
						n = write(fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError sending node info!\n");
							break;
						}								
						new_table = (nodeinfo*)new_table->next;
					}
					state = busy;
				}
				if(state != busy) state = reg;
				endFLAG = 2;
				break;
			default:
				printf("\treached an Unknown State!\n");
				exit(1);
				break;
		}
		if(endFLAG == 1) break;
		else if(endFLAG == 2){
			endFLAG = 0;
			continue;
		}
		/* Select Phase */
		if(state == busy){
			cntr = select(maxfd + 1, &ready_sockets, (fd_set *)NULL, (fd_set *)NULL, &tv);
			if(cntr <= 0){
				printf("\tError: Did not received advertise form internal neighbour!\n");
				state = reg;
				continue;
			}
		}
		else{
			cntr = select(maxfd + 1, &ready_sockets, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
			if(cntr <= 0){
				printf("\tError: Unexpected (%d): %s\n", cntr, strerror(errno));
				aux_table = head_table;
				while(aux_table != NULL){
					printf("\t%s %d\n", aux_table->id, aux_table->fd);
					aux_table = (nodeinfo *)aux_table->next;
				}
				exit(1);
			}
		}
		/* Nodes Connected Messages Processing */	
		if((state == reg) && (head_table != (nodeinfo *)NULL)){
			aux_table = head_table;
			while(aux_table != NULL){
				if((aux_table->fd != 0) && (FD_ISSET(aux_table->fd, &ready_sockets))){
					FD_CLR(aux_table->fd, &ready_sockets);
					cntr--;

					bzero(buffer, sizeof(buffer));
					n = read(aux_table->fd, buffer, sizeof(buffer));
					if(n <= 0){
						except_fd = aux_table->fd;
						bzero(except_id, sizeof(except_id));
						strcpy(except_id, aux_table->id);
						state = handle;
						break;
					}
					
					i=0;
				    token = strtok(buffer, "\n");
					while(token != NULL){
						strcpy(matrix[i], token);
						token = strtok(NULL, "\n");
						i++;
					}					
					i--;

					while(i>=0){
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, matrix[i]);
						bzero(cmd, sizeof(cmd));
						if(sscanf(buffer, "%s", cmd) == 1){
							cmd_code = get_msg(cmd); // Get command code
						}
						else continue;

						switch(cmd_code){
							case 1: // ADVERTISE
								// Update Expedition Table
								new_table = (nodeinfo*)calloc(1, sizeof(nodeinfo));
								sscanf(buffer, "%s %s", user_str, new_table->id);
								new_table->fd = aux_table->fd;
								new_table->next = NULL;
								head_table = table_in(head_table, new_table);
								// Propagate the Message
								new_table = head_table;
								while(new_table != NULL){
									if((new_table->fd != 0) && (new_table->fd != aux_table->fd)){
										n = write(new_table->fd, buffer, sizeof(buffer));
										if(n <= 0){
											printf("\tError sending message!\n");
											break;
										}
									}
									new_table = (nodeinfo*)new_table->next;
								}
								break;
							case 2: // WITHDRAW
								sscanf(buffer, "%s %s", user_str, except_id);
								new_table = head_table;
								while(new_table != NULL){
									if(strcmp(new_table->id, except_id) == 0){
										head_table = table_out(head_table, new_table->id);
										break;
									}					
									new_table = (nodeinfo*)new_table->next;
								}
								// Propagate the Message
								new_table = head_table;
								while(new_table != NULL){
									if((new_table->fd != 0) && (new_table->fd != aux_table->fd)){
										n = write(new_table->fd, buffer, sizeof(buffer));
										if(n <= 0){
											printf("\tError sending message!\n");
											break;
										}
									}
									new_table = (nodeinfo*)new_table->next;
								}
								break;
							case 3: // EXTERN
								if(sscanf(buffer, "%s %s %s", user_str, backup_node.node_ip, backup_node.node_tcp) != 3){
									printf("\t%s\n", buffer);
									printf("\tError getting backup node information.\n");
								}
								break;

							default: // Unknown Message Received
								break;
						}
						i--;	
					}
				}
				aux_table = (nodeinfo *)aux_table->next;
			}		
		}
		/* Server and User Messages Processing */
		for(; cntr; --cntr){
			switch(state){
				case busy:
					if(FD_ISSET(fd, &ready_sockets)){
						FD_CLR(fd, &ready_sockets);

						bzero(buffer, sizeof(buffer));
						n = read(fd, buffer, sizeof(buffer));
						if(n <= 0){
							// printf("\tError receiving a message from a neighbour node!\n");
							bzero(except_id, sizeof(except_id));
							strcpy(except_id, aux_table->id);
							except_fd = fd;
							state = handle;
							break;
						}
						
						i=0;
				    	token = strtok(buffer, "\n");
						while(token != NULL){
							strcpy(matrix[i], token);
							strcat(matrix[i], "\n");
							token = strtok(NULL, "\n");
							i++;
					 	}
					 	i--;

					 	while(i>=0){
					 		bzero(buffer, sizeof(buffer));
					 		strcpy(buffer, matrix[i]);

							bzero(cmd, sizeof(cmd));
							sscanf(buffer, "%s", cmd);
							if(strcmp(cmd, "ADVERTISE") == 0){							
								// Update Expedition Table
								new_table = (nodeinfo*)calloc(1, sizeof(nodeinfo));
								sscanf(buffer, "%s %s", user_str, new_table->id);
								new_table->fd = fd;
								new_table->next = NULL;
								head_table = table_in(head_table, new_table);
								// Propagate the Message
								new_table = head_table;
								while(new_table != NULL){
									if((new_table->fd != 0) && (new_table->fd != fd)){
										n = write(new_table->fd, buffer, sizeof(buffer));
										if(n <= 0){
											printf("\tError sending message!\n");
											break;
										}
									}
									new_table = (nodeinfo*)new_table->next;
								}
							}
							else if(strcmp(cmd, "WITHDRAW") == 0){
								aux_fd = -1;
								sscanf(buffer, "%s %s", user_str, except_id);
								new_table = head_table;
								while(new_table != NULL){
									if(strcmp(new_table->id, except_id) == 0){
										aux_fd = new_table->fd;
										head_table = table_out(head_table, new_table->id);
										new_table = head_table;
									}
									else if(new_table->fd == aux_fd) new_table->fd = fd;						
									new_table = (nodeinfo*)new_table->next;
								}
								// Propagate the Message
								new_table = head_table;
								while(new_table != NULL){
									if((new_table->fd != 0) && (new_table->fd != fd)){
										n = write(fd, buffer, sizeof(buffer));
										if(n <= 0){
											printf("\tError sending message!\n");
											break;
										}
									}
									new_table = (nodeinfo*)new_table->next;
								}
							}
							else{
								printf("\tInvalid message received when node was updating!\n");
								break;
							}
							i--;
						}
						state = reg;
					}
					break;
				case unreg:
					if(FD_ISSET(0, &ready_sockets)){
						FD_CLR(0, &ready_sockets);
						if(fgets(user_str, 64, stdin)!= NULL){
							bzero(cmd, sizeof(cmd));
							if(sscanf(user_str, "%s", cmd) == 1){
								cmd_code = get_cmd(cmd); // Get command code
							}
							else break;

							switch(cmd_code){
								case 0: // unknown
									printf("\tInvalid or unknown command: %s\n", cmd);
					   				break;

					   			case 1: // join
					   				if(joined != 0){ // Already joined?
					   					printf("\tNode already joined a net!\n");
					   					break;
					   				}

					   				errcode = sscanf(user_str, "%s %s %s %s %s", cmd, net, nodeID, bootIP, bootTCP);
					   				if((errcode != 3) && (errcode != 5)){
					   					printf("\tInvalid command syntax. The ideal executable command is: 'join net id bootIP bootTCP'.\n");
					   					break;
					   				}
					   				else{
						   				if(errcode == 3){ // Indirect Join
						   					bootIP[0] = '\0';
						   					bootTCP[0] = '\0';
						   					if(getEXT(net, regIP, regUDP, bootIP, bootTCP, nodeIP, nodeTCP, 0) == -1) break;
						   				}
						   				if(errcode == 5){ // Direct Join
						   					if(check_ip(bootIP) == 0){
												printf("Error in IP format verification.\n");
												break;
											}
											if((atoi(bootTCP) < 0) || (atoi(bootTCP) > 65535)){
												printf("Error specifying TCP port.\n");
												break;
											}
						   				}
										strcpy(extern_node.node_ip, bootIP);
										strcpy(extern_node.node_tcp, bootTCP);

										if((extern_node.node_ip[0] != '\0') && (extern_node.node_tcp[0] != '\0')){ // Connect to external neighbour
											fd = tcp_connection(extern_node.node_ip, extern_node.node_tcp);
											extern_node.fd = fd;
											// Send Self Info to External
											bzero(buffer, sizeof(buffer));
											sprintf(buffer, "NEW %s %s\n", nodeIP, nodeTCP);
											n = write(fd, buffer, sizeof(buffer));
											if(n <= 0){
												printf("\tError sending node info!\n");
												break;
											}

											FD_ZERO(&rfds);
											FD_SET(fd, &rfds);

											counter = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
											if(counter <= 0){
												printf("\tError establishing connection with external neighbour node!\n");
												close(fd);
												break;
											}
											bzero(buffer, sizeof(buffer));
											n = read(fd, buffer, sizeof(buffer));
											if(n <= 0){
												printf("\tError receiving node info!\n");
												break;
											}
											// Get External Neighbour Info
											if((sscanf(buffer, "%s %s %s", user_str, backup_node.node_ip, backup_node.node_tcp) != 3) || (strcmp(user_str, "EXTERN") != 0) || (n <= 0)){
												printf("\t%s\n", buffer);
												printf("\tError getting backup node information.\n");
												close(fd);
												break;
											}
											else backup_node.fd = 0;
											// Self Advertise
											bzero(buffer, sizeof(buffer));
											sprintf(buffer, "ADVERTISE %s\n", nodeID);
											n = write(fd, buffer, sizeof(buffer));
											if(n <= 0){
												printf("\tError sending node info!\n");
												break;
											}
										}

										// Start Expedition Table
										head_table = (nodeinfo*)calloc(1, sizeof(nodeinfo));
										strcpy(head_table->id, nodeID);
										head_table->fd = 0;
										head_table->next = NULL;

										// Register Node in Node Server
					   					joined = regNODE(1, net, nodeIP, nodeTCP, regIP, regUDP);
					   					if(joined == 0) break;
					   					else if(joined == 1){
					   						if(extern_node.node_ip[0] != '\0') state = busy;
					   						else state = reg;
					   					}
					   				}
					   				break;

					   			case 3: // exit
					   				state = getout;
					   				if((state == getout) && (joined == 0)) printf("\tSucess! Node shut down.\n");
					   				break;

					   			default:
					   				printf("\tNode does not have joined any net yet!\n");
					   				break;
					   		}	
							if(state != getout){
					   			printf(">>> "); // Command Line Prompt
								fflush(stdout);			
					   		}		
						}
					}
					break;

				case reg:
					if(FD_ISSET(0, &ready_sockets)){				
						FD_CLR(0, &ready_sockets);
						if(fgets(user_str, 64, stdin)!= NULL){

							if(sscanf(user_str, "%s", cmd) == 1){
								cmd_code = get_cmd(cmd); // Get command code
							}
							else break;

							switch(cmd_code){
								case 0: // unknown
									printf("\tInvalid or unknown command: %s\n", cmd);
					   				break;

					   			case 2: // leave
					   				if(joined != 1){ // Not joined yet?
					   					printf("\tNode does not have joined any net!\n");
					   					break;
					   				}
					   				// shut down all connection with other nodes here
					   				joined = regNODE(0, net, nodeIP, nodeTCP, regIP, regUDP);
					   				if(joined == 1) break;
					   				else if(joined == 0) state = unreg;

					   				aux_table=head_table;
					   				while(aux_table!=NULL){
					   					if(aux_table->fd != 0) close(aux_table->fd);
					   					aux_table = (nodeinfo *) aux_table->next;
					   				}

					   				table_free(head_table); // free expedition table
					   				head_table = (nodeinfo *)NULL;
					   				for(i = 0; i < 5; i++) intern[i].fd = 0; // reset ineternal neighbour vector
					   				extern_node.node_ip[0] = '\0';
									extern_node.node_tcp[0] = '\0';
									extern_node.fd = 0;
									backup_node.node_ip[0] = '\0';
									backup_node.node_tcp[0] = '\0';

					   				memset(net, '\0', sizeof(net));
					   				break;

					   			case 3: // exit
					   				printf("\tShutting down all connections and closing the node...\n");
					   				// shut down all connection with other nodes here
					   				if(joined == 1){
					   					joined = regNODE(0, net, nodeIP, nodeTCP, regIP, regUDP);
					   				}
					   				state = getout;

					   				while(aux_table!=NULL){
						   				if(aux_table->fd != 0) close(aux_table->fd);
						   				aux_table = (nodeinfo *) aux_table->next;
					   				}
					   				table_free(head_table); // free expedition table
					   				head_table = (nodeinfo *)NULL;

					   				if((state == getout) && (joined == 0)) printf("\tSucess! Node shut down.\n");
					   				break;

					   			case 4: // show topology
					   				if(extern_node.node_ip[0] == '\0'){
					   					printf("\tThe node is alone in the net.\n");
					   					break;
					   				}
					   				else{
						   				printf("\tEXTERNAL NEIGHBOUR: %s %s\n", extern_node.node_ip, extern_node.node_tcp);
						   				printf("\tBACKUP NEIGHBOUR: %s %s\n", backup_node.node_ip, backup_node.node_tcp);
					   				}
					   				break;

					   			case 5: // show routing
					   				printf("\tEXPEDITION TABLE:\n");
					   				aux_table = head_table;
									while(aux_table != NULL){
										printf("\tID:%s\tfd:%d\n", aux_table->id, aux_table->fd);
										aux_table = (nodeinfo *)aux_table->next;
									}		
					   				break;

					   			default:
					   				printf("\tNode already joined a net!\n");
					   				break;
					   		}	
					   		if(state != getout){
					   			printf(">>> "); // Command Line Prompt
								fflush(stdout);			
					   		}	
						}
					}
					else if(FD_ISSET(fd_server, &ready_sockets)){
						FD_CLR(fd_server, &ready_sockets);

						addrlen_tcp = sizeof(addr_tcp);
						if((fd = accept(fd_server, &addr_tcp, &addrlen_tcp)) == -1) exit(1);

						FD_ZERO(&rfds);
						FD_SET(fd, &rfds);

						counter = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
						if(counter <= 0){
							printf("\tError establishing connection with internal neighbour node!\n");
							close(fd);
							break;
						}

						bzero(buffer, sizeof(buffer));
						n = read(fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError receiving node info!\n");
							break;
						}

						bootIP[0] = '\0';
						bootTCP[0] = '\0';
						if((sscanf(buffer, "%s %s %s", user_str, bootIP, bootTCP) != 3) || (strcmp(user_str, "NEW") != 0) || (n <= 0)){
							printf("\t%s\n", buffer);
							printf("\tError getting external neighbour node information.\n");
							close(fd);
							break;
						}
						else if(extern_node.node_ip[0] == '\0'){
							strcpy(extern_node.node_ip, bootIP);
							strcpy(extern_node.node_tcp, bootTCP);
							strcpy(backup_node.node_ip, nodeIP);
							strcpy(backup_node.node_tcp, nodeTCP);
							extern_node.fd = fd;
						}

						for(i = 0; i < 5; i++){
							if(intern[i].fd == 0){
								strcpy(intern[i].node_ip, bootIP);
								strcpy(intern[i].node_tcp, bootTCP); 
								intern[i].fd = fd;
								break;
							}
						}

						bzero(buffer, sizeof(buffer));
						sprintf(buffer, "EXTERN %s %s\n", extern_node.node_ip, extern_node.node_tcp);
						n = write(fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError sending node info!\n");
							break;
						}
						// send new connection my expedition table
						aux_table = head_table;
						while(aux_table != NULL){
								bzero(buffer, sizeof(buffer));
								sprintf(buffer, "ADVERTISE %s\n", aux_table->id);
								n = write(fd, buffer, sizeof(buffer));
								if(n <= 0){
									printf("\tError sending expedition table!\n");
									break;
								}
							aux_table = (nodeinfo*)aux_table->next;
						}
						state = busy;
					}
					break;

				case getout: break;

				default: // Do nothing
   					break;
			}
		}
	}


	freeaddrinfo(res);
	close(fd_server);
	exit(0);
}