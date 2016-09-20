#define _GNU_SOURCE
#include <stdio.h>   /* standard I/O routines  */
#include <pthread.h>  /* pthread functions and data structures */
#include <stdlib.h> /* rand() and srand() functions   */
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

/* number of threads used to service requests */
#define NUM_HANDLER_THREADS 10

#define DEFAULT_PORT "12345" // default port if user didn't input one

#define MAX_USERS 100

#define NUM_PLAYERS 15

#define min(a, b) (((a) < (b)) ? (a) : (b))   // define the max() function

#define BACKLOG 10     /* how many pending connections queue will hold */

#define RETURNED_ERROR -1

typedef struct user user_t; // Store the registered users from text file
typedef struct clientDetails clientDetails_t; // Store the client details
typedef struct account account_t; // Store the account details
typedef struct transaction transaction_t; // Store transaction details
typedef struct node node_t; // Nodes


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
};

struct clientDetails {
    char firstname[20];
    char lastname[20];
    int client_no;
    struct account_node;
};

struct transaction {
    int fromAccount;
    int toAccount;
    int tranType;
    double amount;
};

// A node in a linked list of accounts to clientNo
struct account_node {
    account_t *accountno;
    node_t *next;
};

/* ----------------------------------------- Function Call ----------------------------------------------------------*/
int isLetter(char input);
int isNumber(char input);
void add_request(int request_num, pthread_mutex_t* p_mutex, pthread_cond_t*  p_cond_var);




struct sockaddr_in server_addr; // Server Address information
struct sockaddr_in client_addr; // Client Address information


/* ------------------------- Functions for add/handle the threads from Tutorial Practical ------------------------------- */

/*
 * Function add_request(): Add a request to the requests list
 * Algorithm: Creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * Input:     Request number, linked list mutex.
 * Output:    None.
 */
void add_request(int request_num, pthread_mutex_t* p_mutex, pthread_cond_t*  p_cond_var)
{
    int rc;                         /* Return code of pthreads functions.  */
    struct request* a_request;      /* Pointer to newly added request.     */

    /* Create structure with new request */
    a_request = (struct request*)malloc(sizeof(struct request));
    if (!a_request) { /* Malloc failed?? */
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }
    a_request->number = request_num;
    a_request->next = NULL;

    /* Lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    /* Add new request to the end of the list, updating list */
    /* Pointers as required */
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

// Check if a char is a letter
int isLetter(char input){
    char letters[52] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMDOPQRSTUVWXYZ";
    for (int i = 0; i < 52; i++) {
        if (letters[i] == input){
            return 0;
        }
    }
    return 1;
}
// Check if a char is a number
int isNumber(char input){
    char numbers[10] = "0123456789";
    for (int i = 0; i < 26; i++) {
        if (numbers[i] == input){
            return 0;
        }
    }
    return 1;
}




/* ----------------------------------------------- Main Function ---------------------------------------------------- */
int main(int argc, char* argv[]) {


}
}