/*test machine: CSELAB_machine_name * date: 12/06/19
* name: Oliver Latocki
* x500: latoc004 */


#include "../include/phase1.h"
#include "../include/protocol.h"


void createLogFile(void) {
    pid_t p = fork();
    if (p==0)
        execl("/bin/rm", "rm", "-rf", "log", NULL);

    wait(NULL);
    mkdir("log", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    logfp = fopen("log/log_client.txt", "w");
}

int main(int argc, char *argv[]) {
    int mappers;
    char folderName[100] = {'\0'};
    char *server_ip;
    int server_port;

    if (argc == 5) { // 4 arguments
        strcpy(folderName, argv[1]);
        mappers = atoi(argv[2]);
        server_ip = argv[3];
        server_port = atoi(argv[4]);
        if (mappers > MAX_MAPPER_PER_MASTER) {
            printf("Maximum number of mappers is %d.\n", MAX_MAPPER_PER_MASTER);
            printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
            exit(1);
        } else if(mappers < 1){
            printf("Minimum number of mappers is %d.\n", 1);
            printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
            exit(1);
        }

    } else {
        printf("Invalid or less number of arguments provided\n");
        printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
        exit(1);
    }

    // create log file
    createLogFile();

    // phase1 - File Path Partitioning
    traverseFS(mappers, folderName);

    // Phase2 - Mapper Clients's Deterministic Request Handling
    
    //spawn client processes
    int pid;
	for (int id = 1; id < mappers + 1; id++){
		pid = fork();
		if (pid < 0){
			fprintf(stderr, "error creating process");
		} else if (pid == 0){
			phase_2(id, server_port, server_ip);
		}
	}

    //join processes
    for (int i = 0; i < mappers; i++){
        wait(NULL);
    }

    fclose(logfp);
    return 0;

}
