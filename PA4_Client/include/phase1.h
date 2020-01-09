#ifndef PHASE1_H

//phase 1
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include<sys/wait.h> 

//phase 2
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER_PORT 4061
/*test machine: CSELAB_machine_name * date: 12/06/19
* name: Oliver Latocki
* x500: latoc004 */

#define DIRNULL NULL
#define FILENULL NULL

//phase 1
void recursiveTraverseFS(int mappers, char *basePath, FILE *fp[], int *toInsert, int *nFiles);
void traverseFS(int mappers, char *path);

FILE *logfp;

int phase_2(int id, int port, char *ip);
void send_CHECKIN(int sockfd, int id);
void read_mapper_file(int sockfd, int id, int *total_messages);
void send_GET_AZLIST(int sockfd, int id);
void send_GET_MAPPER_UPDATES(int sockfd, int id);
void send_GET_ALL_UPDATES(int sockfd, int id);
void send_CHECKOUT(int sockfd, int id);

void parse_file(char *filename, int *az_list_buf);
int count_files(char *filename);

#endif