/*test machine: CSELAB_machine_name * date: 12/06/19
* name: Oliver Latocki
* x500: latoc004 */


#include "../include/protocol.h"
#include "../include/phase1.h"

void recursiveTraverseFS(int mappers, char *basePath, FILE *fp[], int *toInsert, int *nFiles){
	//check if the directory exists
	DIR *dir = opendir(basePath);
	if(dir == DIRNULL){
		printf("Unable to read directory %s\n", basePath);
		exit(1);
	}

	char path[1000];
	struct dirent *dirContentPtr;

	//use https://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html
	while((dirContentPtr = readdir(dir)) != DIRNULL){
		if (strcmp(dirContentPtr->d_name, ".") != 0 &&
			strcmp(dirContentPtr->d_name, "..") != 0){
			struct stat buf;
			lstat(path, &buf);
			if (S_ISLNK(buf.st_mode))
				continue;
			if (dirContentPtr->d_type == 8){
				//file
				char filePath[1000];
				strcpy(filePath, basePath);
				strcat(filePath, "/");
				strcat(filePath, dirContentPtr->d_name);
				
				//insert into the required mapper file
				fputs(filePath, fp[*toInsert]);
				fputs("\n", fp[*toInsert]);

				*nFiles = *nFiles + 1;

				//change the toInsert to the next mapper file
				*toInsert = (*toInsert + 1) % mappers;
			}else if (dirContentPtr->d_type == 4){ //FIX THIS
				//directory
				// basePath creation - Linux platform
				strcpy(path, basePath);
				strcat(path, "/");
				strcat(path, dirContentPtr->d_name);
				recursiveTraverseFS(mappers, path, fp, toInsert, nFiles);
			}
		}
	}
}

void traverseFS(int mappers, char *path){
    FILE *fp[mappers];

    pid_t p = fork();
    if (p==0)
        execl("/bin/rm", "rm", "-rf", "MapperInput", NULL);

    wait(NULL);
    //Create a folder 'MapperInput' to store Mapper Input Files
    mkdir("MapperInput", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // open mapper input files to store paths of files to be processed by each mapper
    int i;
    for (i = 0; i < mappers; i++){
        // create the mapper file name
        char mapperCount[10];
        sprintf(mapperCount, "%d", i + 1);
        char mapInFileName[100] = "MapperInput/Mapper_";
        strcat(mapInFileName, mapperCount);
        strcat(mapInFileName, ".txt");
        fp[i] = fopen(mapInFileName, "w");

    }
    //refers to the File to which the current file path should be inserted
    int toInsert = 0;
    int nFiles = 0;
    recursiveTraverseFS(mappers, path, fp, &toInsert, &nFiles);

    // close all the file pointers
    for(i = 0; i < mappers; i++)
        fclose(fp[i]);

    if(nFiles == 0){
        pid_t p1 = fork();
        if (p1==0)
            execl("/bin/rm", "rm", "-rf", "MapperInput", NULL);
    }

    if (nFiles == 0){
        printf("The %s folder is empty\n", path);
        exit(0);
    }
}


//PHASE 2, MY CODE


//called by each client process
//connects each client to server and makes each request
int phase_2(int id, int port, char *ip){

    int sockfd = socket(AF_INET , SOCK_STREAM , 0);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip);

	if (connect(sockfd, (struct sockaddr *) &address, sizeof(address)) != 0){
		perror("Connection failed!\n");
	} else{
		fprintf(logfp, "[%d] open connection\n", id);
	}

	//send sequential requests to server
    send_CHECKIN(sockfd, id);
	int total_messages = 0;
	read_mapper_file(sockfd, id, &total_messages);
	fprintf(logfp, "[%d] UPDATE_AZLIST: %d\n", id, total_messages);
	send_GET_AZLIST(sockfd, id);
	send_GET_MAPPER_UPDATES(sockfd, id);
	send_GET_ALL_UPDATES(sockfd, id);
	send_CHECKOUT(sockfd, id);

	exit(0);
	return 0;
}

//send CHECKIN request
void send_CHECKIN(int sockfd, int id){
    int request_buf[28];
	int response_buf[28];
    request_buf[0] = CHECKIN;
	request_buf[1] = id;
    write(sockfd, request_buf, sizeof(request_buf));
    read(sockfd, response_buf, sizeof(response_buf));
	fprintf(logfp, "[%d] CHECKIN: %d %d\n", id, response_buf[1], response_buf[2]);
}

//send CHECKOUT request
void send_CHECKOUT(int sockfd, int id){
    int request_buf[28];
	int response_buf[28];
    request_buf[0] = CHECKOUT;
	request_buf[1] = id;
    write(sockfd, request_buf, sizeof(request_buf));
    read(sockfd, response_buf, sizeof(response_buf));
	fprintf(logfp, "[%d] CHECKOUT: %d %d\n", id, response_buf[1], response_buf[2]);
	if (response_buf[1] == RSP_OK){
		//close
		fprintf(logfp, "[%d] close connection\n", id);
		close(sockfd);
	}
}

//send GET_AZLIST request
void send_GET_AZLIST(int sockfd, int id){
	char log_buf[256];
	char let_count[128];
	int request_buf[28];
	int response_buf[28];
    request_buf[0] = GET_AZLIST;
	request_buf[1] = id;
	
    write(sockfd, request_buf, sizeof(request_buf));
	read(sockfd, response_buf, sizeof(response_buf));
	sprintf(log_buf, "[%d] GET_AZLIST: %d ", id, response_buf[1]);
	for (int i = 0; i < 26; i++){
		if (i < 25){
			sprintf(let_count, "%d ", response_buf[i + 2]);
		} else{
			sprintf(let_count, "%d", response_buf[i + 2]);
		}
		strcat(log_buf, let_count);
	}
	strcat(log_buf, "\n");
	fprintf(logfp, "%s", log_buf);
}

//send UPDATE_AZLIST request
void send_UPDATE_AZLIST(int sockfd, int id, int *requset_buf){
    int response_buf[28];
	write(sockfd, requset_buf, sizeof(int)*29);
	read(sockfd, response_buf, sizeof(response_buf));
}

//send GET_MAPPER_UPDATES request
void send_GET_MAPPER_UPDATES(int sockfd, int id){
	int request_buf[28];
	int response_buf[28];
    request_buf[0] = GET_MAPPER_UPDATES;
	request_buf[1] = id;
	
    write(sockfd, request_buf, sizeof(request_buf));
	read(sockfd, response_buf, sizeof(response_buf));
	fprintf(logfp, "[%d] GET_MAPPER_UPDATES: %d %d\n", id, response_buf[1], response_buf[2]);
	//printf("updates %d\n", response_buf[2]);
}

//send GET_ALL_UPDATES request
void send_GET_ALL_UPDATES(int sockfd, int id){
	int request_buf[28];
	int response_buf[28];
    request_buf[0] = GET_ALL_UPDATES;
	request_buf[1] = id;
	
    write(sockfd, request_buf, sizeof(request_buf));
	read(sockfd, response_buf, sizeof(response_buf));
	fprintf(logfp, "[%d] GET_ALL_UPDATES: %d %d\n", id, response_buf[1], response_buf[2]);
}

//this function reads and parses all the files stored in Mapper_i.txt
//this calls send_UPDATE_AZLIST()
void read_mapper_file(int sockfd, int id, int *total_messages){
	//open file
	int mapper_file_count = 0;
	char mapper_filename[128];
	char *filename = malloc(sizeof(char)*128);
	sprintf(mapper_filename, "MapperInput/Mapper_%d.txt", id);
	FILE *fp = fopen(mapper_filename, "r");
	mapper_file_count = count_files(mapper_filename);

	//parse each file inside Mapper_i.txt

	while ((fgets(filename, sizeof(char)*128, fp)) != NULL) {
		int *request_buf = (int*)malloc(sizeof(int)*29);
		for (int i = 0; i < 29; i++){
			request_buf[i] = 0;
		}
		request_buf[0] = UPDATE_AZLIST;
		request_buf[1] = id;
		request_buf[28] = mapper_file_count;
		parse_file(filename, request_buf);

		//send request
		send_UPDATE_AZLIST(sockfd, id, request_buf);
		free(request_buf);
		*total_messages += 1;
	}
	
	free(filename);
	fclose(fp);
}

//do counting statistics on each file before sending UPDATE_AZLIST
void parse_file(char *filename, int *az_list_buf){
	//remove \n from end of filename; necessary to open properly
	filename[strcspn(filename, "\n")] = 0;
	FILE *fp;
    char *line;
	char c;
    size_t len = 0;
    fp = fopen(filename, "r");
    if (fp == NULL){
        exit(1);
	}
	//count how many occurances of a word's starting with each letter within the file
    while ((getline(&line, &len, fp)) != -1) {
		c = line[0];
		if (c >= 65 && c <= 90){
			/* handle capital letters */
			az_list_buf[c - 65 + 2] += 1;
		} else if(c >= 97 && c <= 122){
			/* handle lowercase letters */
			az_list_buf[c - 97 + 2] += 1;
		}
    }
	if (line){
		free(line);
	}
	fclose(fp);
}

//count number of files in each Mapper_i.txt
int count_files(char *filename){
	FILE *fp = fopen(filename, "r");
	char c;
	int count = 0;
  
    // Check if file exists 
  
    while(!feof(fp)){
		c = fgetc(fp);
		if(c == '\n'){
			count++;
		}
	}
	return count;
}