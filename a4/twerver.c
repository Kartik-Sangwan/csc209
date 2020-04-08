#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 50031
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);

// These are some of the function prototypes that we used in our solution 
// You are not required to write functions that match these prototypes, but
// you may find them helpful when thinking about operations in your program.

// Send the message in s to all clients in active_clients. 
void announce(struct client **active_clients, char *s);

// Move client c from new_clients list to active_clients list. 
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr);

//Handle the input and output for the client who has not yet entered a name.
int handle_new_client(struct client *p, struct client **active_clients, struct client **new_clients);

//Handle the commands form the current active user.
void handle_commands(struct client *p, struct client **active_clients_ptr);

// Find the network newline in the input from users.
int find_network_newline(const char *buf, int n);

//Read the input form a client.
int read_from_any_client(struct client *p);

//Partial_remove is a helper function that removes a client p from both lists.
void partial_remove(struct client *p, struct client *current_following, struct client *current_follower);

//Client1 unfollows client2.
int unfollow(struct client *client1, struct client *client2, struct client **active_clients_ptr);

//Client1 follows client2.
int follow(struct client *client1, struct client *client2, struct client **active_clients_ptr);

// Broadcast the message to all the followers.
int send_message(struct client *client, char *msg, struct client **active_clients);

//Displays the previously sent messages of those users this user is following.
void show_messages(struct client *p, struct client **active_clients_ptr);
// Indicate to the user that the command entered is invalid.
void invalid_command(struct client *p, struct client **active_clients_ptr);


// Announce to all clients except the client with username as he is the one exiting.
void exit_announce(struct client **active_clients_ptr, char *username);

// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {

    // Create all the fields of the new_client and add it to the head of clients list.
    struct client *p = calloc(1 ,sizeof(struct client));
    if (!p) {
        perror("calloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    memset(p->username, '\0', sizeof(p->username));
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next);

    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        // TODO: Remove the client from other clients' following/followers
        // lists

        for (int i = 0; i < FOLLOW_LIMIT; i++){
            // These are the temporary pointers to both lists in p.
            struct client *following = (*p)->following[i];
            struct client *follower = (*p)->followers[i];
            partial_remove(*p, following, follower);
            (*p)->following[i] = NULL;
            (*p)->followers[i] = NULL;

        }

        // Remove the client
        struct client *t = (*p)->next;
        printf("Removed client. %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;

    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);

    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, 
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        // Handle input from a new client who has not yet
                        // entered an acceptable name.
                    
                        if (read_from_any_client(p) == 1) {

                            // Invalid read from client p.
                            fprintf(stderr, "Read from the client failed\n");
                            remove_client(&new_clients, p->fd);
                        } else {

                            // Handle the new user who is yet to enter username.
                            handled = handle_new_client(p, &active_clients, &new_clients) + 1;
                            // If handeled is 1 then correct usename has been entered.
                            // Thus activate client.
                            if (handled == 1) {
                                activate_client(p, &active_clients, &new_clients);
                            } else {
                                 fprintf(stderr, "Invalid username entered by user:%d\n", p->fd);
                            }
                            break;
                        }
                        
                    }
                }
                // This is for previously added users (i.e. active users).
                // User that was just activated doesn't satisfy this if condition.
                if (handled == 0) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
                            // Handle input from an active client.
                            if(read_from_any_client(p) == 1) {
                                fprintf(stderr, "Read from the client failed\n");
                                exit_announce(&active_clients, p->username);
                                remove_client(&active_clients, p->fd);
                            } else {
                                // Handle any command issues by this of the active users.
                                handle_commands(p, &active_clients);
                                break;
                            }
                            
                        } 
                    }
                }
            }
        }
    }
    return 0;
}


int handle_new_client(struct client *p, struct client **active_clients, struct client **new_clients) {

    // p->inbuf refers to the username of the client as its a new client.
    
    // Checking whether username is valid or not.
    int flag = 0;
    if (p->inbuf[0] != '\0'){
        struct client *temp_client = (*active_clients);
        while (temp_client  != NULL){
            if (strcmp(temp_client->username, p->inbuf) == 0) flag = -1;
            temp_client = temp_client->next;
        }
    }

    // Username entered is not valid.

    if (p->inbuf[0] == '\0' || flag == -1) {
        char *msg = "INVALID USERNAME PLEASE ENTER AGAIN: ";
        if (write(p->fd, msg, strlen(msg)) == -1) {
            fprintf(stderr, "Write to client failed. A\n");
            exit_announce(active_clients, p->username);
            remove_client(new_clients, p->fd);
        }
        return 1;
    } else {
        return 0;
    }
}

void handle_commands(struct client *p, struct client **active_clients){

    // p->inbuf refers to the current message/command of the client p.
    printf("%s: %s\n", p->username, p->inbuf);


    char *space = strchr(p->inbuf, ' ');
    // Check if command has a space in it.
    if (space == NULL) {
        // For the command to be valid it has to be show or a quit command.
        if (strcmp(p->inbuf, "show") == 0){
           show_messages(p, active_clients);
        } else if (strcmp(p->inbuf, "quit") == 0){
            exit_announce(active_clients, p->username);
            remove_client(active_clients, p->fd);
        } else {
            // Otherwise, the user entered an invalid command.
            invalid_command(p, active_clients);
            return;
        }
    } else {
        // Parse the command that conatins a space.
        // Split it into two parts the command and 
        // the rest_command that remains after the space.
        int size = strlen(p->inbuf) - strlen(space) + 1;
        char command[size];
        char rest_command[strlen(space)];
        strncpy(command, p->inbuf, size - 1);
        strncpy(rest_command, space + 1, strlen(space) - 1);
        command[size - 1] = '\0';
        rest_command[strlen(space) - 1] = '\0';

        // If its a follow command.
        if (strcmp(command, "follow") == 0){
            int flag = 1;
            // Compare usernames that are in the active clients list.
            // If username matches the rest_command then follow that user.
            struct client *temp = *active_clients;
            while (temp != NULL) {
                if (strcmp(temp->username, rest_command) == 0){
                    // Found the client to follow.
                    int ret = follow(p, temp, active_clients);
                    if (ret == 1){
                        invalid_command(p, active_clients);
                        return;
                    }

                    flag = 0;
                }
                temp = temp->next;

            }
            
            // if user to follow not found then username to follow was invalid.
            // Notify user p.
            if (flag == 1){
                invalid_command(p, active_clients);
                return;
            }
        // If its a un-follow command.
        } else if (strcmp(command, "unfollow") == 0){
            int flag = 1;
            // Compare usernames that are in the active clients list.
            // If username matches the rest_command then un-follow that user.
            struct client *temp = *active_clients;
            while (temp != NULL) {
                if (strcmp(temp->username, rest_command) == 0){
                    // Found the client to un-follow.
                    int ret = unfollow(p, temp, active_clients);
                    // If can't follow.
                    if (ret == 1){
                        invalid_command(p, active_clients);
                        return;
                    }
                    flag = 0;
                }
                temp = temp->next;
            }
            // If user to un-follow not found then username to un-follow was invalid.
            // Notify user p.
            if (flag){
                invalid_command(p, active_clients);
                return;
            }
        // If the command is send send the message.    
        } else if (strcmp(command, "send") == 0){
            send_message(p, rest_command, active_clients);
        } else {
            invalid_command(p, active_clients);
        }
    }
}

// Helper method to write "Invalid Command to the user p.
void invalid_command(struct client *p, struct client **active_clients){
    char *msg = "Invalid command.\n";
        if (write(p->fd, msg, strlen(msg)) == -1) {
            fprintf(stderr, "Write to client failed.\n");
            exit_announce(active_clients, p->username);
            remove_client(active_clients, p->fd);
        }
}


void show_messages(struct client *p, struct client **active_clients){
    
    // Iterate over the following list of p.
    for (int i = 0; i < FOLLOW_LIMIT; i++){

        struct client *temp = p->following[i];
        // If p is following another user.
        if (temp != NULL){
            // Send all the previously sent messages of that user to p.
            for (int j = 0; j < MSG_LIMIT && temp->message[j][0] != '\0'; j++){
                
                // Initialising the message.
                int size = strlen(temp->username) + strlen(" wrote: ") + strlen(temp->message[j]) + 2;
                char buf[size];
                buf[0] = '\0';
                strncat(buf, temp->username, strlen(temp->username));
                strncat(buf, " wrote: ", strlen(" wrote: "));
                strncat(buf, temp->message[j], strlen(temp->message[j]));
                buf[size - 2] = '\n';
                buf[size - 1] = '\0';
                // Writing the message to p.
                if (write(p->fd, buf, strlen(buf)) == -1) {
                    fprintf(stderr, "Write to client failed. \n");
                    exit_announce(active_clients, p->username);
                    remove_client(active_clients, p->fd);
                }
            }
        }
    }
}

// Helper method that is responsible for partial reads from the user and store the data read in p-inbuf.
int read_from_any_client(struct client *p) {

    int num_buf = 0;                 // How many bytes currently in buffer?
    int room = sizeof(p->inbuf);    // How many bytes remaining in buffer?
    int nbytes;
    //memset(p->inbuf, '\0', strlen(p->inbuf));
    p->in_ptr = p->inbuf;
    while ((nbytes = read(p->fd, p->in_ptr, room)) > 0){

        // Step 1: update inbuf (how many bytes were just added?)
        num_buf += nbytes;
        p->in_ptr = &(p->inbuf[num_buf]);

        int where;

        // The loop condition below calls find_network_newline
        // to determine if a full line has been read from the client.
        while ((where = find_network_newline(p->inbuf, num_buf)) > 0 ){

            // We have a full line.
            // Output the full line, not including the "\r\n",
            // using print statement below.

            p->inbuf[where - 2] = '\0';
            printf("[%d] Read %d bytes\n", p->fd, num_buf);
            printf("[%d] Found Newline: %s\n", p->fd, p->inbuf);

            return 0;

        }
        // Update p->in_ptr and room, in preparation for the next read.
        p->in_ptr = &p->inbuf[num_buf];
        room = BUF_SIZE - num_buf;
    }
    // Error as we cant read from p.
    return 1;
}


void activate_client(struct client *c, struct client **active_clients_ptr, struct client **new_clients_ptr) {

    add_client(active_clients_ptr, c->fd, c->ipaddr);
    strncat((*active_clients_ptr)->username, c->inbuf, strlen(c->inbuf));

    //Removing the client from the new_clients list.
    struct client **p;
    for (p = new_clients_ptr; *p && (*p)->fd != c->fd; p = &(*p)->next);

    // Now, p points to the client to be removed.

    if (*p) {

        // Remove the client
        struct client *t = (*p)->next;
        free(*p);
        *p = t;
    }

    // Creating a message for new user joining Twitter.
    int size = strlen(" has just joined.\n") + 1 + strlen((*active_clients_ptr)->username);
    char msg[size]; 
    msg[0] = '\0'; 
    strncat(msg, (*active_clients_ptr)->username, strlen((*active_clients_ptr)->username));
    strncat(msg, " has just joined.\n", strlen(" has just joined.\n"));
    printf("%s", msg);
    // Send the message to all the active clients.
    announce(active_clients_ptr, msg);

}

// Helper method for remove_client. Partial_remove removes p from both the followers and following 
// lists of current_following and current_follower clients respepctively.
void partial_remove(struct client *p, struct client *current_following, struct client *current_follower) {

    // Removing p from the current_follower client's following list.
    if (current_follower != NULL) {
        struct client *temp = (current_follower->following)[0];
        if (temp != NULL) {
            for (int i = 0; i < FOLLOW_LIMIT; i++) {

                if (strcmp(temp->username, p->username) == 0){

                    current_follower->following[i] = NULL;
                    break;
                }
                temp += i;
            } 
        } 
    }

    // Removing p from the current_following client's followers list.
    if (current_following != NULL) {
        struct client *temp_new = (current_following->followers)[0];
        if (temp_new != NULL){

            for (int i = 0; i < FOLLOW_LIMIT; i++){

                if ((strcmp(temp_new->username, p->username)) == 0){

                    current_following->followers[i] = NULL;
                    break;
                }
                temp_new += i;
            }
        }
    }
}

// Client 1 unfollows client 2.
int unfollow(struct client *client1, struct client *client2, struct client **active_clients_ptr){

    if (client1 != NULL && client2 != NULL){

        // Removing client1 from the followers list of client2.
        for (int i = 0; i < FOLLOW_LIMIT; i++){
            if (client2->followers[i] != NULL) {
                if ((strcmp(client1->username, (client2->followers[i])->username)) == 0) {
                    client2->followers[i] = NULL;
                }
            }
        }

        // Removing client2 from the following list of client1.
        for (int i = 0; i < FOLLOW_LIMIT; i++){
            if (client1->following[i] != NULL) {
                if (strcmp(client2->username, (client1->following[i])->username) == 0) {
                    client1->following[i] = NULL;
                }
            }
        }

        // Success message to client 1 that he has successfully unfollowed client 2.
        int size = strlen("Unfollowed  successfully.") + strlen(client2->username) + 2;
        char msg[size];
        msg[0] = '\0';
        strncat(msg, "Unfollowed ", 11);
        strncat(msg, client2->username, strlen(client2->username));
        strncat(msg, " successfully.", 14);
        msg[size - 2] = '\n';
        msg[size - 1] = '\0';
        if (write(client1->fd, msg, strlen(msg)) == -1) {
                fprintf(stderr, "Write to client failed.\n");
                exit_announce(active_clients_ptr, client1->username);
                remove_client(active_clients_ptr, client1->fd);
        }
        return 0;
    }
    return 1;

}

// Client 1 follows client 2.
int follow(struct client *client1, struct client *client2, struct client **active_clients_ptr){

    if (client1 != NULL && client2 != NULL) {

        // Checking if client1 already follows client2.
        for (int i = 0; i < FOLLOW_LIMIT; i++) {
            // Check if client1 is already present in client2 followers list.
            if (client2->followers[i] != NULL) {
                // Check if client1 == client2 or if client 1 is already following client2.
                if (strcmp((client2->followers[i])->username, client1->username) == 0) {
                    char *msg = "Already following client.\n";
                    if (write(client1->fd, msg, strlen(msg)) == -1) {
                        fprintf(stderr, "Write to client failed. \n");
                        exit_announce(active_clients_ptr, client1->username);
                        remove_client(active_clients_ptr, client1->fd);
                    }
                    return 1;
                }
            }

            if (strcmp(client2->username, client1->username) == 0){
                char *msg = "Cant follow yourself.\n";
                if (write(client1->fd, msg, strlen(msg)) == -1) {
                    fprintf(stderr, "Write to client failed. \n");
                    exit_announce(active_clients_ptr, client1->username);
                    remove_client(active_clients_ptr, client1->fd);
                }
                return 1;
            }
        }

        //Add the client2 to the followings list of client1.
        int flag1 = 1;
        for (int i = 0; i < FOLLOW_LIMIT; i++) {
            if (client1->following[i] == NULL) {
                //There is empty space in the following list so add client 2 here.
                client1->following[i] = client2;
                flag1 = 0;
                break;
            }
        }

        // If no sufficient space in the followings list notify user.
        if (flag1 == 1) {
            char *msg = "Maximum following capacity reached couldn't follow.\n";
            if (write(client1->fd, msg, strlen(msg)) == -1) {
                fprintf(stderr, "Write to client failed. \n");
                exit_announce(active_clients_ptr, client1->username);
                remove_client(active_clients_ptr, client1->fd);
            }
            return 1;
        }

        //Add the client1 to the followers list of client2.
        int flag2 = 1;
        for (int i = 0; i < FOLLOW_LIMIT; i++) {
            if (client2->followers[i] == NULL) {
                client2->followers[i] = client1;
                flag2 = 0;
                break;
            }
        }

        // If no sufficient space in the followers list notify user.
        if (flag2 == 1) {
            char *msg = "Maximum followers capacity reached couldn't allow %s to follow.\n";
            if (write(client1->fd, msg, strlen(msg)) == -1) {
                fprintf(stderr, "Write to client failed.\n");
                exit_announce(active_clients_ptr, client1->username);
                remove_client(active_clients_ptr, client1->fd);
            }            
            return 1;
        }

        // Success message to client 1 that he has successfully followed client 2.
        int size = strlen("Following  successfully.") + strlen(client2->username) + 2;
        char msg[size];
        msg[0] = '\0';
        strncat(msg, "Following ", 10);
        strncat(msg, client2->username, strlen(client2->username));
        strncat(msg, " successfully.", 14);
        msg[size - 2] = '\n';
        msg[size - 1] = '\0';
        if (write(client1->fd, msg, strlen(msg)) == -1) {
                fprintf(stderr, "Write to client failed.\n");
                exit_announce(active_clients_ptr, client1->username);
                remove_client(active_clients_ptr, client1->fd);
        } 
        return 0;
    }
    return 1;
}

int send_message(struct client *client, char* msg, struct client **active_clients) {

    // Checking whether this client has issued MSG_LIMIT messages.
    int flag = 1;
    for (int i = 0; i < MSG_LIMIT; i++){
        if (client->message[i][0] == '\0'){
            flag = 0;
            strncpy(client->message[i], msg, strlen(msg));
            // The client has space to issue messages.
            // Broadcast the message to all of its followers.
            int size = strlen(client->username) + 4 + strlen(msg);
            char arr[size];
            arr[0] = '\0';
            strncat(arr, client->username, strlen(client->username));
            strncat(arr, ": ", 2);
            strncat(arr, msg, strlen(msg));
            arr[size - 2] = '\n';
            arr[size - 1] = '\0';
            for (int i = 0; i < FOLLOW_LIMIT; i++){
                if (client->followers[i] != NULL) {
                    if (write((client->followers[i])->fd, arr, strlen(arr)) == -1) {
                        fprintf(stderr, "Write to client failed. \n");
                        exit_announce(active_clients, client->username);
                        remove_client(active_clients, client->fd);
                    }
                }
            }
            return 0;
        }

    }   

    // User has reached limit number of messages can't send anymore.
    if (flag) {
        char *msg = "Reached maximum limit of messages to send. Cant send more messages.\n";
        if (write(client->fd, msg, strlen(msg)) == -1) {
            fprintf(stderr, "Write to client failed. \n");
            exit_announce(active_clients, client->username);
            remove_client(active_clients, client->fd);
        }            
        return 1;
    }

    return 0;
}

// Send message s to all of the active clients.
void announce(struct client **active_clients, char *s){

    struct client *temp = *active_clients;
    while (temp != NULL) {
        if (write(temp->fd, s, strlen(s)) == -1) {
            fprintf(stderr, "Write to client failed. \n");
            remove_client(active_clients, temp->fd);
            return;
        }
        temp = temp->next;
    }
}


// Announce to all clients except the client with username as he is the one exiting.
void exit_announce(struct client **active_clients_ptr, char* username){
    // Announcing that this user has exited Twitter.
    int size = strlen(username) + strlen(" has exited Twitter.") + 2;
    char msg[size];
    msg[0] = '\0';
    strncat(msg, username, strlen(username));
    strncat(msg, " has exited Twitter.", strlen(" has exited Twitter."));
    msg[size - 2] = '\n';
    msg[size - 1] = '\0';

    struct client *temp = *active_clients_ptr;
    while(temp != NULL){
        if (strcmp(temp->username, username) != 0){
                if (write(temp->fd, msg, strlen(msg)) == -1) {
                fprintf(stderr, "Write to client failed. \n");
                remove_client(active_clients_ptr, temp->fd);
                return;
            }
        }
        temp = temp->next;
    }
}

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 */
int find_network_newline(const char *buf, int n) {

    for (int i = 0; i < n - 1; i++){
        if (buf[i] == '\r' && buf[i + 1] == '\n'){
            return i + 2;
        }
    }
    return -1;
}
