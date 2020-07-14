#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct _msg {
    char* data;
    struct _msg *next;
} msg;

typedef struct _msgBox {
    int state; // closed: 0, open: 1
    pthread_t owner;
    char* name;
    msg* msg;
    struct _msgBox *next;
} msgBox;

pthread_mutex_t mutex;
msgBox* head_box = NULL;

void outputEvent(int client_socket, char* cmd) {
    char time_buff[12];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(time_buff, 12, "%H%M %d %b", tm);

    struct sockaddr_in peer;
    int peer_len = sizeof(peer);
    getpeername(client_socket, (struct sockaddr*)&peer, &peer_len);

    fprintf(stderr, "%s %s %s\n", time_buff, inet_ntoa(peer.sin_addr), cmd);
}

void outputError(int client_socket, char* error) {
    char time_buff[12];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(time_buff, 12, "%H%M %d %b", tm);

    struct sockaddr_in peer;
    int peer_len = sizeof(peer);
    getpeername(client_socket, (struct sockaddr*)&peer, &peer_len);

    fprintf(stderr, "%s %s %s\n", time_buff, inet_ntoa(peer.sin_addr), error);
}

void hello(int client_socket) {
    char* message = "HELLO DUMBv0 ready!";
    write(client_socket, message, strlen(message)+1);
    outputEvent(client_socket, "HELLO");
}

// if client's message box is still open, close it
// close the socket connection
void goodbye(int client_socket) {
    outputEvent(client_socket, "GDBYE");
    close(client_socket);
}

void printBoxes() {
    msgBox* curr_box = head_box;
    while (curr_box != NULL) {
        printf("%d %s [", curr_box->state, curr_box->name);
        msg* curr_msg = curr_box->msg;

        while (curr_msg != NULL) {
            printf("\"%s\"", curr_msg->data);
            curr_msg = curr_msg->next;
        }

        printf("]\n");
        curr_box = curr_box->next;
    }
}

// create a new message box
void createBox(int client_socket, char* name) {
    pthread_mutex_lock(&mutex);
    // OK! if success
    // ER:EXIST if box with given name already exists
    msgBox* curr_box = head_box;

    // check if box exists
    while (curr_box != NULL) {
        if (strcmp(curr_box->name, name) == 0) {
            char* message = "ER:EXIST";
            write(client_socket, message, strlen(message)+1);
            outputError(client_socket, "ER:EXIST");
            pthread_mutex_unlock(&mutex);
            return;
        }
        curr_box = curr_box->next;
    }

    msgBox* new_box = malloc(sizeof(msgBox));
    char* box_name = malloc(strlen(name)+1);
    strcpy(box_name, name);
    new_box->name = box_name;
    if (head_box != NULL) {
        new_box->next = head_box;
    } else {
        new_box->next = NULL;
    }
    head_box = new_box;
    pthread_mutex_unlock(&mutex);
    //printBoxes();

    char* message = "OK!";
    write(client_socket, message, strlen(message)+1);

    outputEvent(client_socket, "CREAT");
}

// open an existing message box
msgBox* openBox(int client_socket, char* name, int n) {
    // OK! if success
    // ER:HAVBX if the client already has a box open
    // ER:NEXST if box with given name does not exist
    // ER:OPEND if box with given name is already open

    if (n > 0) {
        outputError(client_socket, "ER:HAVBX");
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    msgBox* curr_box = head_box;

    // check if it exists and already open
    int exists = 0;
    int opened = 0;
    while (curr_box != NULL) {
        if (strcmp(curr_box->name, name) == 0) {
            exists = 1;
            if (curr_box->state == 1) {
                opened = 1;
            }
            break;
        }
        curr_box = curr_box->next;
    }

    if (exists == 0) {
        char* message = "ER:NEXST";
        write(client_socket, message, strlen(message)+1);
        outputError(client_socket, "ER:NEXST");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    if (opened == 1) {
        char* message = "ER:OPEND";
        write(client_socket, message, strlen(message)+1);
        outputError(client_socket, "ER:OPEND");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    curr_box->state = 1;
    curr_box->owner = pthread_self();
    pthread_mutex_unlock(&mutex);

    //printBoxes();

    char* message = "OK!";
    write(client_socket, message, strlen(message)+1);

    outputEvent(client_socket, "OPNBX");

    return curr_box;
}

// get next message from currently open message box
void nextMsg(int client_socket, msgBox* box) {
    // OK!arg0!msg if success
    // ER:EMPTY if there are no messages left in the box
    // ER:NOOPN if there is no message box currently open by the user
    msgBox* curr_box = box;

    if (curr_box == NULL) {
        char* message = "ER:NOOPN";
        write(client_socket, message, strlen(message)+1);
        outputEvent(client_socket, "ER:NOOPN");
        return;
    }

    msg* prev_msg = NULL;
    msg* curr_msg = curr_box->msg;

    if (curr_msg == NULL) {
        char* message = "ER:EMPTY";
        write(client_socket, message, strlen(message)+1);
        outputEvent(client_socket, "ER:EMPTY");
        return;
    }

    while (curr_msg != NULL) {
        if (curr_msg->next == NULL)
          break;
        prev_msg = curr_msg;
        curr_msg = curr_msg->next;
    }

    char* response_message = "OK!";
    char response_buff[1024];
    strcpy(response_buff, response_message);

    char len_buff[1024];
    sprintf(len_buff, "%d", (int)strlen(curr_msg->data));
    strcat(response_buff, len_buff);
    strcat(response_buff, "!");

    char* sending_message = curr_msg->data;
    strcat(response_buff, sending_message);

    write(client_socket, response_buff, strlen(response_buff)+1);

    if (prev_msg == NULL) {
        curr_box->msg = NULL;
    } else {
        prev_msg->next = NULL;
    }

    //printBoxes();
    outputEvent(client_socket, "NXTMG");
}

// put a message in to the currently open message box
void putMsg(int client_socket, msgBox* box, char* message) {
    // OK!arg0 if success
    // ER:NOOPN if there is no message box currently open by the user
    msgBox* curr_box = box;

    if (curr_box == NULL) {
        char* message = "ER:NOOPN";
        write(client_socket, message, strlen(message)+1);
        outputError(client_socket, "ER:NOOPN");
        return;
    }

    msg* new_msg = malloc(sizeof(msg));
    char* new_data = malloc(strlen(message)+1);
    strcpy(new_data, message);
    new_msg->data = new_data;
    new_msg->next = curr_box->msg;
    curr_box->msg = new_msg;

    printBoxes();

    char* response_message = "OK!";
    char response_buff[1024];
    strcpy(response_buff, response_message);

    char len_buff[1024];
    sprintf(len_buff, "%d", (int)strlen(new_msg->data));
    strcat(response_buff, len_buff);

    write(client_socket, response_buff, strlen(response_buff)+1);
    outputEvent(client_socket, "PUTMG");
}

// delete a message box
void delBox(int client_socket, char* name) {
    // OK! if success
    // ER:NEXST if box with given name does not exist
    // ER:OPEND if box with given name is currently open
    // ER:NOTMT if box with given name is not empty yet
    pthread_mutex_lock(&mutex);

    msgBox* prev_box = NULL;
    msgBox* curr_box = head_box;

    // check if it exists and already open
    int exists = 0;
    int opened = 0;
    int empty = 0;
    while (curr_box != NULL) {
        if (strcmp(curr_box->name, name) == 0) {
            exists = 1;
            if (curr_box->state == 1) {
                opened = 1;
            }
            if (curr_box->msg == NULL) {
                empty = 1;
            }
            break;
        }
        prev_box = curr_box;
        curr_box = curr_box->next;
    }

    if (exists == 0) {
        char* message = "ER:NEXST";
        write(client_socket, message, strlen(message)+1);

        outputError(client_socket, "ER:NEXST");
        pthread_mutex_unlock(&mutex);
        return;
    }

    if (opened == 1) {
        char* message = "ER:OPEND";
        write(client_socket, message, strlen(message)+1);
        outputError(client_socket, "ER:OPEND");
        pthread_mutex_unlock(&mutex);
        return;
    }

    if (empty = 0) {
        char* message = "ER:NOTMT";
        write(client_socket, message, strlen(message)+1);
        outputError(client_socket, "ER:NOTMT");
        pthread_mutex_unlock(&mutex);
        return;
    }

    if (prev_box == NULL) {
        head_box = NULL;
    } else {
        prev_box->next = NULL;
    }

    pthread_mutex_unlock(&mutex);

    printBoxes();

    char* message = "OK!";
    write(client_socket, message, strlen(message)+1);
    outputEvent(client_socket, "DELBX");
}

// close an existing message box
int closeBox(int client_socket, char* name) {
    // OK! if success
    // ER:NEXST if box with given name does not exist
    // ER:NOOPN if box with given name is not open by the user
    pthread_mutex_lock(&mutex);
    msgBox* curr_box = head_box;

    // check if it exists and already open
    int exists = 0;
    int opened = 0;
    while (curr_box != NULL) {
        if (strcmp(curr_box->name, name) == 0) {
            exists = 1;
            if (curr_box->state == 1) {
                opened = 1;
            }
            break;
        }
        curr_box = curr_box->next;
    }

    //printf("This box is open by %ld\n", curr_box->owner);

    // check if open by different client
    if (exists == 0 || opened == 0 || curr_box->owner != pthread_self()) {
        char* message = "ER:NOOPN";
        write(client_socket, message, strlen(message)+1);
        outputError(client_socket, "ER:NOOPN");
        pthread_mutex_unlock(&mutex);
        return 1;
    }

    curr_box->state = 0;
    //printBoxes();
    pthread_mutex_unlock(&mutex);

    char* message = "OK!";
    write(client_socket, message, strlen(message)+1);
    outputEvent(client_socket, "CLSBX");

    return 0;
}

void* newClient(void* client_socket) {
    int client_sock = *((int*)client_socket);
    //printf("My client id: %ld\n", pthread_self());

    msgBox* my_box;

    char buff[1024], command[1024], arg[1024], msg[1024];
    int i, nbytes, nboxes = 0;

    while (1) {
        nbytes = read(client_sock, buff, sizeof(buff));
        if (nbytes == 0) {
            break;
        }
        //printf("Read %d bytes: %s\n", nbytes, buff);
        for (i = 0; i < nbytes; i++) {
            if (buff[i] == ' ' || buff[i] == '!') {
                command[i] = '\0';
                i++;
                break;
            }
            command[i] = buff[i];
        }

        printf("Command: \"%s\"\n", command);
        //printf("Command length: %d\n", (int)strlen(command));

        if (strcmp(command, "HELLO") == 0) {
            hello(client_sock);
        } else if (strcmp(command, "GDBYE") == 0) {
            goodbye(client_sock);
            break;
        } // if command is CREAT, OPNBX, DELBX, or CLSBX we expect arg0
        else if (strcmp(command, "CREAT") == 0) {
            for (int j = 0; j < nbytes; j++) {
                arg[j] = buff[i];
                if (buff[i] == '\0') {
                    break;
                }
                i++;
            }
            //printf("Arg: %s\n", arg);
            createBox(client_sock, arg);
        } else if (strcmp(command, "OPNBX") == 0) {
            for (int j = 0; j < nbytes; j++) {
                arg[j] = buff[i];
                if (buff[i] == '\0') {
                    break;
                }
                i++;
            }
            //printf("Arg: %s\n", arg);
            my_box = openBox(client_sock, arg, nboxes);
            if (my_box != NULL) {
                nboxes = 1;
            }
        } else if (strcmp(command, "NXTMG") == 0) {
            nextMsg(client_sock, my_box);
        } else if (strcmp(command, "DELBX") == 0) {
            for (int j = 0; j < nbytes; j++) {
                arg[j] = buff[i];
                if (buff[i] == '\0') {
                    break;
                }
                i++;
            }
            //printf("Arg: %s\n", arg);
            delBox(client_sock, arg);
        } else if (strcmp(command, "CLSBX") == 0) {
            for (int j = 0; j < nbytes; j++) {
                arg[j] = buff[i];
                if (buff[i] == '\0') {
                    break;
                }
                i++;
            }
            //printf("Argument: %s\n", arg);
            if (closeBox(client_sock, arg) == 0) {
                nboxes = 0;
            }
        } // if command is PUTMG we expect ! followed by arg0 followed by ! followed by msg
        else if (strcmp(command, "PUTMG") == 0) {

            for (int j = 0; j < nbytes; j++) {
                if (buff[i] == '!') {
                    arg[j] = '\0';
                    break;
                }
                arg[j] = buff[i];
                i++;
            }
            //printf("Argument: %s\n", arg);
            i++;
            for (int j = 0; j < nbytes; j++) {
                msg[j] = buff[i];
                if (buff[i] == '\0') {
                    break;
                }
                i++;
            }
            //printf("Msg: %s\n", msg);
            putMsg(client_sock, my_box, msg);
        } else {
            char* message = "ER:WHAT?";
            write(client_sock, message, strlen(message)+1);
            outputError(client_sock, "ER:WHAT?");
        }
    }

    pthread_exit(0);

    return NULL;
}

int main(int argc, char* argv[]) {
    /*printf("argc: %d\n", argc);
    printf("Args:\n");
    for (int i=0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }*/

    pthread_mutex_init(&mutex, NULL);

    int port = atoi(argv[1]);
    if (port <= 4096) {
        fprintf(stderr,"ERROR: Port must be strictly greater than 4096\n");
        return -1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        fprintf(stderr, "ERROR: Failed to create socket");
        return -1;
    }

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "ERROR: Socket bind failed");
        return -1;
    }

    if (listen(server_socket, 5) != 0) {
        fprintf(stderr, "ERROR: Socket listen failed");
        return -1;
    }

    int addr_size, client_socket;
    while (1) {
        addr_size = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket < 0) {
            fprintf(stderr, "ERROR: Socket accept failed");
            return -1;
        }

        pthread_t newThread;
        int* pSocket = malloc(sizeof(int));
        *pSocket = client_socket;
        pthread_create(&newThread, NULL, newClient, pSocket);
        outputEvent(*pSocket, "connected");
        //printf("New client id: %ld\n", newThread);
    }

    return 0;
}
