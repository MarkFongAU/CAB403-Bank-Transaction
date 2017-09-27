#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>   /* standard I/O routines  */
#include <pthread.h>  /* pthread functions and data structures */
#include <stdlib.h> /* rand() and srand() functions   */
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>


/* --------- Defines ----------- */
#define MY_PORT "12345"   /* the port users will be connecting to */
#define BACKLOG 10     /* how many pending connections queue will hold */

// From txt
#define MAX_AUTHENTICATION 10   /* Maximum username in the authentication.txt and client_details.txt */
#define MAX_ACCOUNTS 25         /* Maximum accounts in the accounts.txt */
#define DEPOSIT 3       /* Deposit type */
#define WITHDRAWAL 2    /* Withdrawal type */
#define TRANSFER 4      /* Transfer type */

// Size of array
#define ARRAYSIZE(x)  (sizeof(x) / sizeof((x)[0]))

#define MAX_USERS 10 /* Maximum amount of clients can connect to server */
#define MAX_CONNECTIONS 11 /* 10 client 1 server */
#define NUM_OF_USERS
#define RETURNED_ERROR -1
#define OK 1
#define SCREEN 2000
#define NUM_HANDLER_THREADS 10 /* number of threads used to service requests */


/* --------- Typedef Structure ------------- */
typedef struct user user_t;                   // Store the registered users from text file
typedef struct clientDetails clientDetails_t; // Store the client details
typedef struct account account_t;             // Store the account details
typedef struct transaction transaction_t;     // Store transaction details
//typedef struct node node_t;                   // Nodes

// Registered users from authentication.txt file
struct user {
    char username[20];
    int pin;
    int client_no;
};

struct account {
    int account_no;
    double opening_bal;
    double closing_bal;
    transaction_t *transaction;
};

//// A linked list of accounts to clientNo
//struct node {
//    account_t *account_struct;
//    node_t *next;
//};

struct clientDetails {
    char firstname[20];
    char lastname[20];
    user_t user_struct;
    account_t saving_acc;
    account_t loan_acc;
    account_t credit_card;
};

struct transaction {
    int fromAccount;
    int toAccount;
    int tranType;
    double amount;
};

// Format of a single request.
struct request {
    int number;             /* number of the request                  */
    struct request* next;   /* pointer to next request, NULL if none. */
};


/* --------- Connection Variables --------- */
int sockfd, new_fd;            /* listen on sock_fd, new connection on new_fd */
struct sockaddr_in my_addr;    /* my address information */
struct sockaddr_in their_addr; /* connector's address information */
socklen_t sin_size;
int socklist[MAX_USERS];
int exit_client[MAX_USERS];

/* ---------- Variables ------------------- */
/* Global mutex for our program. assignment initializes it. */
/* note that we use a RECURSIVE mutex, since a handler      */
/* thread might try to lock it twice consecutively. --Refer from Tutorial 5 */
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* Global condition variable for our program. assignment initializes it. --Refer from Tutorial 5*/
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

int num_requests = 0;   /* Threadpool: number of pending requests, initially none */
int num_authentication = 0;
int num_accounts = 0;
int num_transaction = 0;

struct request* requests = NULL;     /* head of linked list of requests. */
struct request* last_request = NULL; /* pointer to last request. */

user_t *user_list;
account_t *account_list;
clientDetails_t *clientDetails_list;
transaction_t *transaction_list;


/* ---------- Function Call --------------- */
void setup_server(char *port);
int get_client_address(int num);
void quit_Handler();
void Main_ATM_Handler(int socket_id);
void Send_Data(int socket_id, char *data);
void add_request(int request_num, pthread_mutex_t* p_mutex, pthread_cond_t*  p_cond_var);
struct request* get_request(pthread_mutex_t* p_mutex);
void *handle_requests_loop(void* data);
void handle_request(struct request* a_request, int thread_id);
char *Receive_Data(int socket_identifier, int size);


// Get data from txt file
user_t *get_user_struct();
clientDetails_t *get_clientDetails_struct(user_t *user, account_t *account);
transaction_t *get_transaction_struct();
account_t *get_account_struct(transaction_t *transaction);

//Update struct when a transaction is done
clientDetails_t *update_clientDetails_struct(clientDetails_t *clientDetails, account_t *account);
transaction_t *update_transaction_struct(transaction_t *transaction, int fromAccount, int toAccount, int tranType, double amount);
account_t *update_account_struct(account_t *account, transaction_t *transaction);

// Set data to txt file
void store_transaction(transaction_t *transaction, int transaction_array_size);

// Initialise struct to zero
user_t *initialise_user (size_t n);
account_t *initialise_account (size_t n);
clientDetails_t *initialise_clientDetails (size_t n);
transaction_t *initialise_transaction (size_t n);

// Update struct
transaction_t *update_transaction_struct_size (transaction_t *transaction, size_t n);

/* ---------- Main Function --------------- */
int main(int argc, char *argv[])
{
    signal(SIGINT, quit_Handler);

    printf("%s\n", "------------------Start server---------------");

    if (argc == 1) {
        setup_server(MY_PORT);
    } else {
        setup_server(argv[1]);
    }


    /* Thread variables and attributes */
    int        i;                                /* loop counter          */
    int        thr_id[NUM_HANDLER_THREADS];      /* thread IDs            */
    pthread_t  p_threads[NUM_HANDLER_THREADS];   /* thread's structures   */
    struct timespec delay;                       /* used for wasting time */

    /* Quit option array initialisation*/
    for (int i = 0; i < MAX_USERS; ++i) {
        exit_client[i] = 0;
    }

    /* create the request-handling threads */
    for (i=0; i<NUM_HANDLER_THREADS; i++) {
        thr_id[i] = i;
        pthread_create(&p_threads[i], NULL, handle_requests_loop, (void*)&thr_id[i]);
    }

    /* run a loop that generates requests */
    int counter = 0;
    while(1) {
        if (get_client_address(counter) == OK) {
            sleep(2);
            add_request(counter, &request_mutex, &got_request);
            /* pause execution for a little bit, to allow      */
            /* other threads to run and handle some requests.  */
            if (rand() > 3 * (RAND_MAX / 4)) { /* this is done about 25% of the time */
                delay.tv_sec = 0;
                delay.tv_nsec = 10;
                nanosleep(&delay, NULL);
            }
            counter++;
        }
    }
}

void Main_ATM_Handler(int socket_id){
    // CONTINUE Check username and password function HERE


    const int MAX_BUF = SCREEN;
    char* Main_screen_info = malloc(MAX_BUF);
    int length = 0;

    length += snprintf(Main_screen_info+length, MAX_BUF-length, "Welcome to the ATM System\n\n");
    length += snprintf(Main_screen_info+length, MAX_BUF-length, "You are currently logged in as ");
    length += snprintf(Main_screen_info+length, MAX_BUF-length, "Client Number - ");
    length += snprintf(Main_screen_info+length, MAX_BUF-length, "\n\n\n");
    length += snprintf(Main_screen_info+length, MAX_BUF-length, "Please enter a selection\n<1> Account Balance\n<2> Withdrawal\n<3> Deposit\n<4> Transfer\n<5> Transaction Listing\n<6> EXIT\n\nSelection option 1-6 -> ");

    Send_Data(socket_id, Main_screen_info);
}

// Main Send data function via socket to Client -- Adapt from Tutorial 7
void Send_Data(int socket_id, char *data){
    int i=0;
    uint16_t statistics;
    for (i = 0; i < 10; i++) {
        statistics = htons(data[i]);
        send(socket_id, &statistics, sizeof(uint16_t), 0);
    }
}

// Main Receive data function via specific socket from Client -- Adapt from Tutorial 7
char *Receive_Data(int socket_identifier, int size) {
    int number_of_bytes, i=0;
    uint16_t statistics;

    char *results = malloc(sizeof(char)*size);
    for (i=0; i < size; i++) {
        if ((number_of_bytes=recv(socket_identifier, &statistics, sizeof(uint16_t), 0))
            == RETURNED_ERROR) {
            perror("recv");
        }
        if (ntohs(statistics) != '\n'){
            results[i] = ntohs(statistics);
        }
    }
    return results;
}


// Setup the server, start listening to any clients
void setup_server(char *port){
    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
    }

    /* generate the end point */
    my_addr.sin_family = AF_INET;         /* host byte order */
    my_addr.sin_port = htons(atoi(port)); /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
    /* bzero(&(my_addr.sin_zero), 8);   ZJL*/     /* zero the rest of the struct */

    /* bind the socket to the end point */
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
    == -1) {
        perror("Bind");
        exit(1);
    }

    /* start listnening */
    if (listen(sockfd, BACKLOG) == -1) {
        perror("Listen");
        exit(1);
    }

    printf("Server starts listening ...\n");
}

// Get Client Address
int get_client_address(int num){
    //Prevent 11th client connection
    if(num > MAX_USERS){
        return 0;
    }
    else{
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
        &sin_size)) == -1) {
            perror("Accept");
        } else {
            socklist[num] = new_fd;
            printf("Server: got connection from %s\n", \
            inet_ntoa(their_addr.sin_addr));
            return 1;
        }
    }
    return 0;
}



// Get the user struct from Authentication.txt
user_t *get_user_struct(){
    // Initialize user struct to 0 and allocates the requested memory
    user_t *user = initialise_user(MAX_AUTHENTICATION);
    if(!user){
        return NULL;
    }

    FILE *input_file = fopen("Authentication.txt", "r");
    if (input_file == NULL)
    {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open Authentication.txt\n");
        exit(-1);
    }
    else{
        // Count the actual number of username in the Authentication.txt -- reference from Stack Overflow
        int ch = 0;
        int line = 0;
        while(!feof(input_file)) {
            ch = fgetc(input_file);
            if(ch == '\n'){
                line++;
            }
        }
        num_authentication = line - 1;

        rewind(input_file);
        char buff[255];
        // Token counter
        int counter=0;
        int register_user_counter=0;
        const char *term = "\t\n\r "; // word terminators
        char *state; // Invocation state of strtok_r

        //Ignore first line of the files
        fgets(buff, 255, input_file);

        while(1)
        {
            // Read the file line by line
            fgets(buff, 255, input_file);
            //Token
            char *tok = strtok_r(buff, term, &state); // First invocation is different
            while (tok) {
                //Pass token(data) into struct
                counter++;
                if(counter == 1){
                    strcpy(user[register_user_counter].username, tok);
                } else if(counter == 2){
                    user[register_user_counter].pin = atoi(tok);
                } else if(counter == 3){
                    user[register_user_counter].client_no = atoi(tok);
                }
                tok = strtok_r(NULL, term, &state); // subsequent invocations call with NULL
            }
            counter = 0;
            register_user_counter++;

            // Check if End_Of_File
            if( feof(input_file) )
            {
                break ;
            }
        }
    }
    fclose(input_file);
    return user;
}

// Get the account struct from Accounts.txt
account_t *get_account_struct(transaction_t *transaction){
    // Initialize account struct to 0 and allocates the requested memory
    account_t *account = initialise_account(MAX_ACCOUNTS);
    if(!account){
        return NULL;
    }

    FILE *input_file = fopen("Accounts.txt", "r");
    if (input_file == NULL)
    {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open Accounts.txt\n");
        exit(-1);
    }
    else{
        // Count the actual number of username in the Accounts.txt -- reference from Stack Overflow
        int ch = 0;
        int line = 0;
        while(!feof(input_file)) {
            ch = fgetc(input_file);
            if(ch == '\n'){
                line++;
            }
        }
        num_accounts = line - 1;

        rewind(input_file);
        char buff[255];
        // Token counter
        int counter=0;
        int register_account_counter = 0;
        int transaction_per_account = 0;
        const char *term = "\t\n\r "; // word terminators
        char *state; // Invocation state of strtok_r

        //Ignore first line of the files
        fgets(buff, 255, input_file);

        while(1)
        {
            // Read the file line by line
            fgets(buff, 255, input_file);
            //Token
            char *tok = strtok_r(buff, term, &state); // First invocation is different
            while (tok) {
                //Pass token(data) into struct
                counter++;
                if(counter == 1){
                    account[register_account_counter].account_no = atoi(tok);
                } else if(counter == 2){
                    account[register_account_counter].opening_bal = atof(tok);
                } else if(counter == 3){
                    account[register_account_counter].closing_bal = atof(tok);
                }
                tok = strtok_r(NULL, term, &state); // subsequent invocations call with NULL
            }
            counter = 0;

            //Loop through transaction_t
            for(int i=0; i < num_transaction; i++){
                // Check the type of transaction,
                // if deposit and withdraw, limited to own account,
                // else involves 2 accounts
                if((transaction[i].tranType == DEPOSIT ||
                   transaction[i].tranType == WITHDRAWAL) &&
                        transaction[i].fromAccount == account[register_account_counter].account_no &&
                        transaction[i].toAccount == account[register_account_counter].account_no){
                    account[register_account_counter].transaction[transaction_per_account] = transaction[i];
                    transaction_per_account++;
                } else if(transaction[i].tranType == TRANSFER &&
                        (transaction[i].fromAccount == account[register_account_counter].account_no ||
                        transaction[i].toAccount == account[register_account_counter].account_no)){
                    account[register_account_counter].transaction[transaction_per_account] = transaction[i];
                    transaction_per_account++;
                }
            }
            transaction_per_account=0;

            register_account_counter++;

            // Check if End_Of_File
            if( feof(input_file) )
            {
                break ;
            }
        }
    }
    fclose(input_file);
    return account;
}

// Get the clientDetails struct from Client_Details and from user, account struct.txt
clientDetails_t *get_clientDetails_struct(user_t *user, account_t *account){
    // Initialize clientDetails struct to 0 and allocates the requested memory
    clientDetails_t *client_Details = initialise_clientDetails(MAX_AUTHENTICATION);
    if(!client_Details){
        return NULL;
    }

    FILE *input_file = fopen("Client_Details.txt", "r");
    if (input_file == NULL)
    {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open Client_Details.txt\n");
        exit(-1);
    }
    else{
        // NOT NEEDED: Count the actual number of username in the Client_Details.txt -- reference from Stack Overflow

        rewind(input_file);
        char buff[255];
        // Token counter
        int counter=0;
        int register_client_details_counter=0;
        const char *term = "\t\n\r, "; // word terminators
        char *state; // Invocation state of strtok_r

        //Ignore first line of the files
        fgets(buff, 255, input_file);

        while(1)
        {
            // Read the file line by line
            fgets(buff, 255, input_file);
            //Token
            char *tok = strtok_r(buff, term, &state); // First invocation is different
            while (tok) {
                //Pass token(data) into struct
                counter++;
                if(counter == 1){
                    strcpy(client_Details[register_client_details_counter].firstname, tok);
                } else if(counter == 2){
                    strcpy(client_Details[register_client_details_counter].lastname, tok);
                } else if(counter == 3){
                    // Loop through user_t
                    for(int i=0; i < MAX_AUTHENTICATION; i++){
                        // Check user_t client number,
                        // if match pass the struct into clientDetails_t
                        if(user[i].client_no == atoi(tok)){
                            client_Details[register_client_details_counter].user_struct = user[i];
                        }
                    }
                } else if(counter >= 4){
                    // Loop through account_t
                    for(int i=0; i < MAX_ACCOUNTS; i++){
                        // Check account_t AccountNo, and type of account,
                        // 11- Saving, 12- Loan, 13- Credit card,
                        // if match pass the struct into clientDetails_t
                        if(account[i].account_no == atoi(tok) && atoi(tok)%11 ==0){
                            client_Details[register_client_details_counter].saving_acc = account[i];
                        } else if(account[i].account_no == atoi(tok) && atoi(tok)%12 ==0){
                            client_Details[register_client_details_counter].loan_acc = account[i];
                        } else if(account[i].account_no == atoi(tok) && atoi(tok)%13 ==0){
                            client_Details[register_client_details_counter].credit_card = account[i];
                        }
                    }
                }
                tok = strtok_r(NULL, term, &state); // subsequent invocations call with NULL
            }
            counter = 0;
            register_client_details_counter++;

            // Check if End_Of_File
            if( feof(input_file) )
            {
                break ;
            }
        }
    }
    fclose(input_file);
    return client_Details;
}

// Get the transaction struct from Transaction.txt
transaction_t *get_transaction_struct(){
    transaction_t *transaction;

    FILE *input_file = fopen("Transaction.txt", "r");
    if (input_file == NULL)
    {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open Transaction.txt\n");
        exit(-1);
    }
    else{
        // Count the actual number of transaction in the Transaction.txt -- reference from Stack Overflow
        int ch = 0;
        int line = 0;
        while(!feof(input_file)) {
            ch = fgetc(input_file);
            if(ch == '\n'){
                line++;
            }
        }
        num_transaction = line - 1;

        // Initialize transaction struct to 0 and allocates the requested memory
        transaction = initialise_transaction(num_transaction);

        rewind(input_file);
        char buff[255];
        // Token counter
        int counter=0;
        int register_transaction_counter=0;
        const char *term = "\t\n\r, "; // word terminators
        char *state; // Invocation state of strtok_r

        //Ignore first line of the files
        fgets(buff, 255, input_file);

        while(1)
        {
            // Read the file line by line
            fgets(buff, 255, input_file);
            //Token
            char *tok = strtok_r(buff, term, &state); // First invocation is different
            while (tok) {
                //Pass token(data) into struct
                counter++;
                if(counter == 1){
                    transaction[register_transaction_counter].fromAccount = atoi(tok);
                } else if(counter == 2){
                    transaction[register_transaction_counter].toAccount = atoi(tok);
                } else if(counter == 3){
                    transaction[register_transaction_counter].tranType = atoi(tok);
                } else if(counter == 4){
                    transaction[register_transaction_counter].amount = atof(tok);
                }
                tok = strtok_r(NULL, term, &state); // subsequent invocations call with NULL
            }
            counter = 0;
            register_transaction_counter++;

            // Check if End_Of_File
            if( feof(input_file) )
            {
                break ;
            }
        }
    }
    fclose(input_file);
    if(!transaction){
        return NULL;
    }
    return transaction;
}

// Set the transaction struct into the Transaction.txt
void store_transaction(transaction_t *transaction, int transaction_array_size){
    FILE* output_file = fopen("Transaction.txt", "w");

    char buff[255];
    //Ignore first line of the files
    fgets(buff, 255, output_file);

    for (int j = 0; j < transaction_array_size; j++)
    {
        fprintf(output_file, "%d\t\t", transaction[j].fromAccount);
        fprintf(output_file, "%d\t\t", transaction[j].toAccount);
        fprintf(output_file, "%d\t\t", transaction[j].tranType);
        fprintf(output_file, "%f\n", transaction[j].amount);
    }
    fclose(output_file);
}

// Update clientDetails struct after doing a transaction
clientDetails_t *update_clientDetails_struct(clientDetails_t *clientDetails, account_t *account){
    // Loop through account_t
    for(int i=0; i < MAX_ACCOUNTS; i++){
        // Check account_t AccountNo, and type of account,
        // 11- Saving, 12- Loan, 13- Credit card,
        // if match pass the struct into clientDetails_t
        for(int j=0; j< MAX_AUTHENTICATION;j++){
            if(clientDetails[j].saving_acc.account_no == account[i].account_no){
                clientDetails[j].saving_acc = account[i];
            } else if(clientDetails[j].loan_acc.account_no == account[i].account_no){
                clientDetails[j].loan_acc = account[i];
            } else if(clientDetails[j].credit_card.account_no == account[i].account_no){
                clientDetails[j].credit_card = account[i];
            }
        }
    }
}

// Update account struct after doing a transaction
account_t *update_account_struct(account_t *account, transaction_t *transaction){
    //Loop through account_t
    for(int i=0; i < num_accounts; i++){
        // Check the type of the latest transaction,
        // if deposit and withdraw, limited to own account,
        // else involves 2 accounts
        if((transaction[num_transaction-1].tranType == DEPOSIT ||
            transaction[num_transaction-1].tranType == WITHDRAWAL) &&
           transaction[num_transaction-1].fromAccount == account[num_accounts].account_no &&
           transaction[num_transaction-1].toAccount == account[num_accounts].account_no){
            // Append behind the last index of the transaction struct in the account struct
            int j=0;
            // Check if reached out of bound index
            while(account[num_accounts].transaction[j].fromAccount != NULL){
                j++;
            }
            account[num_accounts].transaction[j] = transaction[num_transaction-1];
        } else if(transaction[num_transaction-1].tranType == TRANSFER &&
                  (transaction[num_transaction-1].fromAccount == account[num_accounts].account_no ||
                   transaction[num_transaction-1].toAccount == account[num_accounts].account_no)){
            // Append behind the last index of the transaction struct in the account struct
            int j=0;
            // Check if reached out of bound index
            while(account[num_accounts].transaction[j].fromAccount != NULL){
                j++;
            }
            account[num_accounts].transaction[j] = transaction[num_transaction-1];
        }
    }
}

// Update transaction struct after doing a transaction
transaction_t *update_transaction_struct(transaction_t *transaction, int fromAccount, int toAccount, int tranType, double amount){
    num_transaction++;
    update_transaction_struct_size(transaction, num_transaction);
    transaction[num_transaction+1].fromAccount= fromAccount;
    transaction[num_transaction+1].toAccount= toAccount;
    transaction[num_transaction+1].tranType= tranType;
    transaction[num_transaction+1].amount= amount;
}

// Initialise struct to NULL/0
user_t *initialise_user (size_t n) {
    return calloc(n, sizeof(user_t));
}

account_t *initialise_account (size_t n) {
    return calloc(n, sizeof(account_t));
}

clientDetails_t *initialise_clientDetails (size_t n){
    return calloc(n, sizeof(clientDetails_t));
}

transaction_t *initialise_transaction (size_t n) {
    return calloc(n, sizeof(transaction_t));
}

// Update struct size
transaction_t *update_transaction_struct_size (transaction_t *transaction, size_t n){
    return realloc(transaction, n);
}

// Check if the input is a Char
int isChar(char input){
    char letters[53] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMDOPQRSTUVWXYZ";
    for (int i = 0; i < 52; i++) {
        if (letters[i] == input){
            return 1;
        }
    }
    return 0;
}

// Check if the input is a Integer
int isInteger(char input){
    char numbers[11] = "0123456789";
    for (int i = 0; i < 10; i++) {
        if (numbers[i] == input){
            return 1;
        }
    }
    return 0;
}

// Check if the input is a double (includes negative doubles)
int isDouble(char input){
    char double_number[13] = "0123456789.-";
    for (int i = 0; i < 12; i++) {
        if (double_number[i] == input){
            return 1;
        }
    }
    return 0;
}

// Handler to handle the action of Ctrl + C
void quit_Handler(){
    // Prevent server log out if their are client still using


    printf("\nYou quit\n");
    int rc;
    // Free any request
    while (num_requests > 0) {
        struct request* a_request = get_request(&request_mutex);
        if (a_request) { /* got a request - handle it and free it */
            rc = pthread_mutex_unlock(&request_mutex);
            free(a_request);
            printf("Request %d freed\n", a_request->number);
            /* and lock the mutex again. */
            rc = pthread_mutex_lock(&request_mutex);
        }
    }

    // Close all Client sockets
    // Free allocated memory
    // Clear transaction listing memory
    // Free customer accounts dynamic memory
    // Destroy Synchronisation Primitives
    // Server close all sockets and dynamic memory deallocated
    // Transaction file and account file updated
    exit(1);
}


/* -- Functions for add/handle the threads from Tutorial Practical 5-- */
/*
 * function add_request(): add a request to the requests list
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    none.
 */
void add_request(int request_num,
                 pthread_mutex_t* p_mutex,
                 pthread_cond_t*  p_cond_var)
{
    int rc;                         /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to newly added request.     */

    /* create structure with new request */
    a_request = (struct request*)malloc(sizeof(struct request));
    if (!a_request) { /* malloc failed?? */
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }
    a_request->number = request_num;
    a_request->next = NULL;

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    /* add new request to the end of the list, updating list */
    /* pointers as required */
    if (num_requests == 0) { /* special case - list is empty */
        requests = a_request;
        last_request = a_request;
    }
    else {
        last_request->next = a_request;
        last_request = a_request;
    }

    /* increase total number of pending requests by one. */
    num_requests++;

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* signal the condition variable - there's a new request to handle */
    rc = pthread_cond_signal(p_cond_var);
}

/*
 * function get_request(): gets the first pending request from the requests list
 *                         removing it from the list.
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    pointer to the removed request, or NULL if none.
 * memory:    the returned request need to be freed by the caller.
 */
struct request* get_request(pthread_mutex_t* p_mutex)
{
    int rc;                         /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to request.                 */

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    if (num_requests > 0) {
        a_request = requests;
        requests = a_request->next;
        if (requests == NULL) { /* this was the last request on the list */
            last_request = NULL;
        }
        /* decrease the total number of pending requests */
        num_requests--;
    }
    else { /* requests list is empty */
        a_request = NULL;
    }

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* return the request to the caller. */
    return a_request;
}

/*
 * function handle_requests_loop(): infinite loop of requests handling
 * algorithm: forever, if there are requests to handle, take the first
 *            and handle it. Then wait on the given condition variable,
 *            and when it is signaled, re-do the loop.
 *            increases number of pending requests by one.
 * input:     id of thread, for printing purposes.
 * output:    none.
 */
void* handle_requests_loop(void* data)
{
    int rc;                         /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to a request.               */
    int thread_id = *((int*)data);  /* thread identifying number           */


    /* lock the mutex, to access the requests list exclusively. */
    rc = pthread_mutex_lock(&request_mutex);

    /* do forever.... */
    while (1) {

        if (num_requests > 0) { /* a request is pending */
            a_request = get_request(&request_mutex);
            if (a_request) { /* got a request - handle it and free it */
                /* unlock mutex - so other threads would be able to handle */
                /* other reqeusts waiting in the queue paralelly.          */
                rc = pthread_mutex_unlock(&request_mutex);
                handle_request(a_request, thread_id);
                free(a_request);
                /* and lock the mutex again. */
                rc = pthread_mutex_lock(&request_mutex);
            }
        }
        else {
            /* wait for a request to arrive. note the mutex will be */
            /* unlocked here, thus allowing other threads access to */
            /* requests list.                                       */

            rc = pthread_cond_wait(&got_request, &request_mutex);
            /* and after we return from pthread_cond_wait, the mutex  */
            /* is locked again, so we don't need to lock it ourselves */

        }
    }
}

/*
 * function handle_request(): handle a single given request.
 * algorithm: prints a message stating that the given thread handled
 *            the given request.
 * input:     request pointer, id of calling thread.
 * output:    none.
 */
void handle_request(struct request* a_request, int thread_id)
{
    if (a_request) {
        printf("Thread '%d' handled request '%d'\n",
               thread_id, a_request->number);


        // DO SOMETHING



        fflush(stdout);
    }
}