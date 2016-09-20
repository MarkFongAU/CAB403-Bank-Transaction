#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>

#define PORT 12345    /* The port client will be connecting to */

#define MAXDATASIZE 100 /* Max number of bytes we can get at once */

void check_Socket(char *port);
void check_Connection(int argc, char *hostname);
void print_welcome_info();
void send_data_receive_reply(char *data);
void send_data_receive_board(char *data);
void send_data_no_reply(char *data);
int check_input(char input);
void quitHandler();

//Referred from Practical 7
int sockfd, numbytes;
char buf[MAXDATASIZE];
struct hostent *he;
struct sockaddr_in their_addr; /* Connector's address information */

int quit = 0;

//Main Function
int main(int argc, char *argv[]) {
    signal(SIGNIT, quitHandler);
    // Check connection details(hostname/address)
    check_Connection(argc, argv[1]);
    // Print welcome information
    print_welcome_info();
    check_Socket(argv[2]);

    checkLogin(argv[2]);


}

// get connection to the server by the port number
void check_Socket(char *port){
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(atoi(port));    /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if (connect(sockfd, (struct sockaddr *)&their_addr, \
    sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
}

void check_Connection(int argc, char *hostname){
    if (argc != 3) {
        fprintf(stderr,"usage: client_hostname port_number\n");
        exit(1);
    }

    if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info */
        herror("gethostbyname");
        exit(1);
    }
}

// Print welcome information on client end
void print_welcome_info(){
    for (int i = 0; i < 50; ++i){
        printf("%s", "=");
    }
    printf("\n\n\n");
    printf("%s\n", "Welcome to the ATM.");
    printf("\n\n");
    for (int i = 0; i < 50; ++i){
        printf("%s", "=");
    }
    printf("\n\n");
    printf("%s\n\n", "You are required to logon with your registered Username and Password.");
}


// send data to server
void send_data_receive_reply(char *data){
    Send_Data(sockfd, data);
    if ((numbytes=recv(sockfd, buf, 40, 0)) == -1) {
        perror("recv");
    }
}

void send_data_receive_board(char *data){
    if ((numbytes=recv(sockfd, board_buf, 2000, 0)) == -1) {
        perror("recv");
    } else {
        printf("%s", buf);
        printf("%s\n",board_buf);
    }
}

void send_data_no_reply(char *data){
    Send_Data(sockfd, data);
}

void Send_Data(int socket_id, char *data) {
    int i=0;
    uint16_t statistics;
    for (i = 0; i < 10; i++) {
        statistics = htons(data[i]);
        send(socket_id, &statistics, sizeof(uint16_t), 0);
    }
}

// Check if the char is a lower case letter
int check_input(char input){
    char letters[27] = "abcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 26; i++) {
        if (letters[i] == input){
            return 0;
        }
    }
    return 1;
}

// Handler to handle the interrupt of Ctrl + C, let server know that the client has quit
void quitHandler(){
    printf("\nYou quit\n");
    send_data_no_reply("******");
    exit(1);
}