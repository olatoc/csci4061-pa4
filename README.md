# This was a project for the class CSCI 4061: Introduction to Operating Systems
 
 /*test machine: csel-kh1250-01.cselabs.umn.edu * date: 12/06/19
* name: Oliver Latocki
* x500: latoc004 */


PROGRAMMING ASSIGNMENT 4: SOCKET PROGRAMMING
CSCI 4061

Server/client program to demonstrate TCP socket programming. Uses a multithreaded server which handles several client connections to solve a map/reduce problem: counting letter frequencies of thousands of words.

HOW TO COMPILE


server.c


>make
    (which calls gcc -std=gnu99 -Wall -pthread -Iinclud -o server src/server.c)
>./server <Server Port>


where <Server Port> is any unsigned integer to be used as a port number


client.c


>make
    (which calls gcc -Wall -std=gnu99 -pthread -Iinclude -o client src/client.c src/phase1.c)
>./client <Folder Name> <# of Mappers> <Server IP> <Server Port>


where,
* <Folder Name> is the name of the directory to read files from
   * i.e. “Testcases/TestCase4”
* <# of Mappers> is between 1 and 32
* Server IP and Server Port are the respective server’s IP and port


INDIVIDUAL CONTRIBUTIONS


* I did everything


ASSUMPTIONS


* Number of mappers is between 1 and 32, inclusive


EXTRA CREDIT


* No
