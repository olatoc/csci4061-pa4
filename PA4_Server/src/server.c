/*test machine: CSELAB_machine_name * date: 12/06/19
* name: Oliver Latocki
* x500: latoc004 */


#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <zconf.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include "../include/protocol.h"

//mutex for handling critical sections, accessing global buffers
pthread_mutex_t currentConn_lock;
int updateStatus[MAX_MAPPER_PER_MASTER][3];
int azlist[26];


//arg struct for thread
typedef struct thread_arg {
    int clientfd;
    char *clientip;
    int clientport;
} thread_arg;

void *thread_function(thread_arg *args){
    //local variables for handling requests
    int all_updates = 0;
    int mapper_file_count;
    int mapper_file_i = 0;
    
    //request and response buffers
    int request_buf[29];
    int response_buf[28];
    for (int i = 0; i < 28; i++){
        request_buf[i] = 0;
        response_buf[i] = 0;
    }
    request_buf[28] = 0;
    int old_signal = -1;
    int new_signal = -1;
    //this loop is to repeatedly read the socket for new requests made by client processes
    while (1){
        read(args->clientfd, request_buf, sizeof(int)*29);
        //error handle bad requests
        if (request_buf[0] != CHECKIN){
            if (updateStatus[request_buf[1] - 1][2] != 1){
                //no corresponding entry
                fprintf(stderr, "ERROR: no corresponding entry in updateStauts table\n");
                response_buf[1] = RSP_NOK;
                write(args->clientfd, response_buf, sizeof(response_buf));
            }
        }
        new_signal = request_buf[0];
        if (new_signal == UPDATE_AZLIST){
            mapper_file_count = request_buf[28];
        }
        //handle all requests by designated request codes
        if (old_signal != new_signal || new_signal == UPDATE_AZLIST){
            switch (request_buf[0]){
                case CHECKIN:
                    //error handle bad requests
                    if ((request_buf[1] < 0) || (updateStatus[request_buf[1] - 1][2] != 0)){
                        response_buf[1] = RSP_NOK;
                        if (request_buf[1] < 0){
                            fprintf(stderr, "ERROR: client ID less than 0\n");
                        } else if (updateStatus[request_buf[1] - 1][2] != 0){
                            fprintf(stderr, "ERROR: client already checked in\n");
                        }
                        write(args->clientfd, response_buf, sizeof(response_buf));
                        break;
                    }
                    printf("[%d] CHECKIN\n", request_buf[1]);
                    pthread_mutex_lock(&currentConn_lock);
                    //update updateStatus
                    updateStatus[request_buf[1] - 1][0] = request_buf[1]; //id
                    updateStatus[request_buf[1] - 1][2] = 1; //checked in

                    //send response
                    response_buf[0] = CHECKIN;
                    response_buf[1] = RSP_OK;
                    response_buf[2] = request_buf[1]; //id
                    write(args->clientfd, response_buf, sizeof(response_buf));
                    pthread_mutex_unlock(&currentConn_lock);
                    break;
                case UPDATE_AZLIST:
                    if (mapper_file_i < mapper_file_count){
                        pthread_mutex_lock(&currentConn_lock);
                        for (int i = 0; i < 26; i++){
                            azlist[i] += request_buf[i + 2];
                        }
                        updateStatus[request_buf[1] - 1][1] += 1;

                        //send response
                        response_buf[0] = UPDATE_AZLIST;
                        response_buf[1] = RSP_OK;
                        response_buf[2] = request_buf[1]; //id
                        write(args->clientfd, response_buf, sizeof(response_buf));
                        mapper_file_i += 1;
                        pthread_mutex_unlock(&currentConn_lock);
                        break;
                    } else{
                        response_buf[0] = -1;
                    }
                case GET_AZLIST:
                    printf("[%d] GET_AZLIST\n", request_buf[1]);
                    pthread_mutex_lock(&currentConn_lock);

                    //send response
                    response_buf[0] = GET_AZLIST;
                    response_buf[1] = RSP_OK;
                    response_buf[2] = request_buf[1]; //id
                    for (int i = 0; i < 26; i++){
                        response_buf[i + 2] = azlist[i];
                    }
                    write(args->clientfd, response_buf, sizeof(response_buf));
                    //printf("finished azlist\n");
                    pthread_mutex_unlock(&currentConn_lock);
                    break; 
                case GET_MAPPER_UPDATES: 
                    printf("[%d] GET_MAPPER_UPDATES\n", request_buf[1]);
                    pthread_mutex_lock(&currentConn_lock);

                    //send response
                    response_buf[0] = GET_MAPPER_UPDATES;
                    response_buf[1] = RSP_OK;
                    response_buf[2] = updateStatus[request_buf[1] - 1][1];
                    write(args->clientfd, response_buf, sizeof(response_buf));
                    pthread_mutex_unlock(&currentConn_lock);
                    break;
                case GET_ALL_UPDATES: 
                    printf("[%d] GET_ALL_UPDATES\n", request_buf[1]); 
                    pthread_mutex_lock(&currentConn_lock);

                    //send response
                    response_buf[0] = GET_ALL_UPDATES;
                    response_buf[1] = RSP_OK;
                    for (int i = 0; i < MAX_MAPPER_PER_MASTER; i++){
                        all_updates += updateStatus[i][1];
                    }
                    response_buf[2] = all_updates;
                    write(args->clientfd, response_buf, sizeof(response_buf));
                    pthread_mutex_unlock(&currentConn_lock);
                    break;
                case CHECKOUT:
                    //error handle bad requests
                    if (updateStatus[request_buf[1] - 1][2] != 1){
                        response_buf[1] = RSP_NOK;
                        fprintf(stderr, "ERROR: client already checked out\n");
                        write(args->clientfd, response_buf, sizeof(response_buf));
                        break;
                    }
                    printf("[%d] CHECKOUT\n", request_buf[1]);
                    pthread_mutex_lock(&currentConn_lock);
                    updateStatus[request_buf[1] - 1][2] = 0; //checked in

                    //send response
                    response_buf[0] = CHECKOUT;
                    response_buf[1] = RSP_OK;
                    response_buf[2] = request_buf[1]; //id
                    write(args->clientfd, response_buf, sizeof(response_buf));
                    pthread_mutex_unlock(&currentConn_lock);
                    printf("close connection from %s:%d\n", args->clientip, args->clientport);
                    close(args->clientfd);
                    pthread_exit(NULL);
                    break;
                default:
                    //error handle bad request codes
                    response_buf[1] = RSP_NOK;
                    write(args->clientfd, response_buf, sizeof(response_buf));
            }
        }
        old_signal = new_signal;
    }
}

int main(int argc, char *argv[]) {

    //globals; zero out junk values
    for (int i = 0; i < 26; i++){
        azlist[i] = 0;
    }
    for (int i = 0; i < MAX_MAPPER_PER_MASTER; i++){
        for (int j = 0; j < 3; j++){
            updateStatus[i][j] = 0;
        }
    }

    int server_port;

    if (argc == 2) { // 1 arguments
        server_port = atoi(argv[1]);
    } else {
        printf("Invalid or less number of arguments provided\n");
        printf("./server <server Port>\n");
        exit(0);
    }

    // Server (Reducer) code

    int sock = socket(AF_INET , SOCK_STREAM , 0);

	// Bind it to a local address.
	struct sockaddr_in servAddress;
	servAddress.sin_family = AF_INET;
	servAddress.sin_port = htons(server_port);
	servAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(sock, (struct sockaddr *) &servAddress, sizeof(servAddress));

	// We must now listen on this port.
	listen(sock, MAX_MAPPER_PER_MASTER);
    printf("server is listening\n");

    pthread_t threads[MAX_MAPPER_PER_MASTER];

    int thread_index = 0;

    while (1){
        //accept new connections
        struct sockaddr_in clientAddress;
		socklen_t size = sizeof(struct sockaddr_in);
		int clientfd = accept(sock, (struct sockaddr*) &clientAddress, &size);

        thread_arg *arg = (thread_arg*)malloc(sizeof(thread_arg));
		
        //fill thread argument struct
		arg->clientfd = clientfd;
		arg->clientip = inet_ntoa(clientAddress.sin_addr);
		arg->clientport = clientAddress.sin_port;

        printf("open connection from %s:%d\n", arg->clientip, arg->clientport);

        //spawn threads
        pthread_create(&threads[thread_index], NULL, &thread_function, (void*)arg);
        thread_index += 1;
		
    }
    close(sock);
    return 0;
}

