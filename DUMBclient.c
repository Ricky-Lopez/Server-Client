#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char **argv){

    if(atoi(argv[2]) < 4096){
        printf("ERROR: Improper port number.\n");
        return 0;
    }

    //creation of the socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);


    //specifying address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons( atoi(argv[2]) );
    inet_pton(AF_INET, argv[1], &(server_address.sin_addr));

    int connection_status;


    //client tries connecting to DUMBserver 3 times 
    for(int i = 0; i<3; i++){
        connection_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
        
        if(connection_status != -1){
            printf("Successfully connected! Redirecting to command menu:\n\n> ");
            break;
        }
    }

    if(connection_status == -1){
        printf("Error: Connection could not be established.\n");
        close(client_socket);
        return 0;
    }

    //sending HELLO to the server
    char server_message[1024];

    //THERE MAY BE AN ISSUE WITH THE IMPLICATION OF THIS SEND CALL, FIGURE OUT WHETHER IT IS THE CLIENT SOCKET OR THE SERVER SOCKET THAT SHOULD BE THE PARAMETER!
    write(client_socket, "HELLO", (strlen("HELLO")+1));

    //THERE MAY BE AN ISSUE WITH THE IMPLICATION OF THIS RECV CALL, FIGURE OUT WHETHER IT IS THE CLIENT SOCKET OR THE SERVER SOCKET THAT SHOULD BE THE PARAMETER!    
    connection_status = read(client_socket, &server_message, sizeof(server_message));

    if(connection_status < 1){
        printf("error receiving message from server. Shutting down. \n");
        close(client_socket);
        return 0;
    }


    //loop which will read user input until it is understood

    int understood;
    char user_input[1024];

    while(understood != 1){
        fgets(user_input, sizeof(user_input), stdin);
        if(strlen(user_input) > 0){
            user_input[(strlen(user_input))-1] = '\0';
        }


        if(strcmp(user_input, "quit") == 0){                    // USER INPUT _ "QUIT"
            write(client_socket, "GDBYE", (strlen("GDBYE")+1));
            connection_status = read(client_socket, &server_message, sizeof(server_message));
            if(connection_status <= 0){
                printf("Server successfully closed! Goodbye! :) \n");
                return 0;
            }else{
                printf("Server has not closed. Something went wrong.\n>");
            }


        }else if(strcmp(user_input, "create") == 0){           // USER INPUT _ "CREATE" 
            printf("Okay, please write a name for your message box.\n\ncreate:> ");
            fgets(user_input, sizeof(user_input), stdin);

            if(strlen(user_input) > 0){
                user_input[(strlen(user_input))-1] = '\0';
            }

            if(strlen(user_input) < 5 || strlen(user_input) > 25){ //I accidentally checked client-side here, bad habit. Easy fix but seems unnecessary.
                printf("Error: name must be between 5 and 25 characters. Returning to command menu.\n");

            }else{
                char client_message[4100] = "CREAT ";
                strcat(client_message, user_input);

                write(client_socket, client_message, (strlen(client_message)+1));
                connection_status = read(client_socket, &server_message, sizeof(server_message));

                if(connection_status > 0){ //"CREATE" Error handling
                    if(strcmp(server_message, "OK!") == 0){
                        printf("Message box successfully created! Returning to command menu.\n\n> ");
                    }else if(strcmp(server_message, "ER:EXIST") == 0){
                        printf("Error: That message box already exists. Returning to command menu.\n\n> ");
                    }else if(strcmp(server_message, "ER:WHAT?") == 0){
                        printf("Error: The name is malformed in some way, shape or form. Returning to command menu.\n\n> ");
                    }
                }
            }
            


        }else if(strcmp(user_input, "delete") == 0){            // USER INPUT _ "DELETE"
            printf("Okay, please write the name of the message box you would like to delete.\n\ndelete:> ");
            fgets(user_input, sizeof(user_input), stdin);

            if(strlen(user_input) > 0){
                user_input[(strlen(user_input))-1] = '\0';
            }

            char client_message[4100] = "DELBX ";
            strcat(client_message, user_input);

            write(client_socket, client_message, (strlen(client_message)+1));
            connection_status = read(client_socket, &server_message, sizeof(server_message));

            if(connection_status > 0){ //"DELETE" Error handling
                if(strcmp(server_message, "OK!") == 0){
                    printf("Message box successfully deleted! Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:NEXST") == 0){
                    printf("Error: That message box does not exist. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:OPEND") == 0){
                    printf("Error: That message box is currently open, and therefore cannot be deleted right now. Returning to commmand menu.\n\n> ");
                }else if(strcmp(server_message, "ER:NOTMT") == 0){
                    printf("Error: That message box is not empty, and therefore cannot be deleted. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:WHAT?") == 0){
                    printf("Error: The message is malformed in some way, shape or form. Returning to command menu.\n\n> ");
                }
            }


        }else if(strcmp(user_input, "open") == 0){              // USER INPUT _ "OPEN"
            printf("Okay, please write the name of the message box you would like to open.\n\nopen:> ");
            fgets(user_input, sizeof(user_input), stdin);

            if(strlen(user_input) > 0){
                user_input[(strlen(user_input))-1] = '\0';
            }

            char client_message[4100] = "OPNBX ";
            strcat(client_message, user_input);

            write(client_socket, client_message, (strlen(client_message)+1));
            connection_status = read(client_socket, &server_message, sizeof(server_message));

            if(connection_status > 0){ //"OPEN" Error handling
                if(strcmp(server_message, "OK!") == 0){
                    printf("Message box successfully opened! You now have exclusive access to this message box. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:NEXST") == 0){
                    printf("Error: The message box does not exist, and therefore cannot be opened. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:OPEND") == 0){
                    printf("Error: The message box is currently opened by another user, so you must wait. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:WHAT?") == 0){
                    printf("Error: The message is malformed in some way, shape, or form. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:HVEBX") == 0){
                    printf("Extra Credit Error: You already have another box open. You may only have one box open at a time. Returning to command menu.\n\n> ");
                }
            }


        }else if(strcmp(user_input, "close") == 0){            // USER INPUT _ "CLOSE"
            printf("Okay, please write the name of the message box you would like to close.\n\nclose:> ");
            fgets(user_input, sizeof(user_input), stdin);

            if(strlen(user_input) > 0){
                user_input[(strlen(user_input))-1] = '\0';
            }

            char client_message[4100] = "CLSBX ";
            strcat(client_message, user_input);

            write(client_socket, client_message, (strlen(client_message)+1));
            connection_status = read(client_socket, &server_message, sizeof(server_message));
            
            if(connection_status > 0){ //"CLOSE" Error handling
                if(strcmp(server_message, "OK!") == 0){
                    printf("Message box successfully closed! Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:NOOPN") == 0){
                    printf("Error: You do not have that message box open right now, so you cannot close it. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:WHAT?") == 0){
                    printf("Error: The message is malformed in some way, shape, or form. Returning to command menu.\n\n> ");
                }
            }


        }else if(strcmp(user_input, "next") == 0){             // USER INPUT _ "NEXT"
            write(client_socket, "NXTMG" , (strlen("NXTMG")+1));
            connection_status = read(client_socket, &server_message, sizeof(server_message));


            if(server_message[0] == 'O'){ //"NEXT" Error handling
                if(strlen(server_message) <= 104){
                    printf("Message retrieved:\n%s\n\n> ", server_message + 6);
                }else if(strlen(server_message) > 105){
                    printf("Message retrieved:\n%s\n\n> ", server_message + 7);
                }else if(strlen(server_message) > 1006){
                    printf("Message retrieved:\n%s\n\n> ", server_message + 8);
                }
            }else if(strcmp(server_message, "ER:EMPTY") == 0){
                printf("Error: There are no messages in this message box. Returning to command menu.\n\n> ");
            }else if(strcmp(server_message, "ER:NOOPN") == 0){
                printf("Error: You do not have a message box open. Returning to command menu.\n\n> ");
            }else if(strcmp(server_message, "ER:WHAT?") == 0){
                printf("Error: The message is malformed in some way, shape, or form. Returning to command menu.\n\n> ");
            }




        }else if(strcmp(user_input, "put") == 0){              // USER INPUT _ "PUT"
            printf("Okay, what would you like to send?\n\nput:> ");
            fgets(user_input, sizeof(user_input), stdin);

            if(strlen(user_input) > 0){
                user_input[(strlen(user_input))-1] = '\0';
            }

            char client_message[4100] = "PUTMG!";
            char byte_length[7];
            sprintf(byte_length, "%d", (int) strlen(user_input));
            strcat(client_message, byte_length);
            strcat(client_message, "!");
            strcat(client_message, user_input);

            write(client_socket, client_message, (strlen(client_message)+1));
            connection_status = read(client_socket, &server_message, sizeof(server_message));
            
            if(connection_status > 0){
                if(server_message[0] == 'O'){
                    printf("Message successfully stored within the message box! Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:NOOPN") == 0){
                    printf("Error: You do not have a message box open. Returning to command menu.\n\n> ");
                }else if(strcmp(server_message, "ER:WHAT?") == 0){
                    printf("Error: The message is malformed in some way, shape, or form. Returning to command menu.\n\n> ");
                }
            }


        }else if(strcmp(user_input, "help") == 0){            // USER INPUT _ "HELP"
            printf("\nLIST OF COMMANDS:\ncreate - creates a new message box.\ndelete - deletes a user/message box on the DUMB server.\nopen - opens a message box.\nclose - closes a message box.\nnext - gets the next message from the currently open message box.\nput - puts a new message in the currently open message box.\n>");

        }else{                                                //USER INPUT _ INCOMPREHENSIBLE
            printf("That is not a command. For a command list enter \"help\".\n> ");
        }
        

    }



    return 0;
}