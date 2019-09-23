#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/wait.h>
#include <assert.h>

#include "command.h"
#include "send.h"
#include "receive.h"
#define DEFAULT_PORT 8080

void checkError(int status)
{
    if (status < 0) {
        fprintf(stderr, "Process %d: socket error: [%s]\n", getpid(),strerror(errno));
        exit(-1);
    }
}

void handleNewConnection(int chatSocket);

int main(int argc,char* argv[])
{
   short int port = DEFAULT_PORT;
  /*
    Fill this in!

    If user specified port on the command line, set port to it.

    Otherwise, set port to DEFAULT_PORT
   */
    if (argc >= 2)
        port = atoi(argv[1]);


    // Create a socket
    int sid = socket(PF_INET,SOCK_STREAM,0);
    checkError(sid);
    // setup our address -- will listen on 'port' --
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    //pairs the newly created socket with the requested address.
    int status = bind(sid,(struct sockaddr*)&addr,sizeof(addr));
    checkError(status);
    // listen on that socket for "Let's talk" message. No more than 10 pending at once
    status = listen(sid,50);
    checkError(status);

    // Looping through to get any number of clients:
    while(1) {
        struct sockaddr_in client;
        socklen_t clientSize = sizeof(client);
	// Accepting a new client. This returns a new socket file descriptor for communicating with just that client.
        int chatSocket = accept(sid,(struct sockaddr*)&client,&clientSize);
        checkError(chatSocket);
        fprintf(stderr, "Accepted a socket: %d\n",chatSocket);
	// Forking to handle the connected client in a child process:
        pid_t child = fork();
        if (child == 0) {
	   // Closing the socket that handles accepting new clients:
            close(sid);
	    // Handling client commands:
            handleNewConnection(chatSocket);
	    // Closing the client connection socket because we do not need it anymore.
            close(chatSocket);
            fprintf(stderr, "Process %d: closing socket %d and exiting ..\n", getpid(), chatSocket);
            return -1; /* Report that I died voluntarily */
        } else  {
            if (child > 0) 
                fprintf(stderr, "Created a child: %d\n",child);
            else 
                fprintf(stderr, "fork() failed.\n");
	    // Closing the recently connected client's socket in the parent process because it is not needed.
            close(chatSocket);
            int status;
            pid_t deadChild; // reap the dead children (as long as we find some)
            do {
	      // For information on why this command works, read the man page for waitpid(...)
                deadChild = waitpid(0,&status,WNOHANG);checkError(deadChild);
                if (deadChild > 0)
                    fprintf(stderr, "Reaped %d\n",deadChild);
                else if (deadChild < 0)
                    fprintf(stderr, "waitpid() returned %d\n", deadChild);
            } while (deadChild > 0);
        }
    }
    return 0;
}


 
/*
 * Handles reciving and executing client commands. 
 * 
 * @param chatSocket - the file descriptor of the client's socket to communicate on.
 */
void handleNewConnection(int chatSocket)
{
    // This is the child process.
    pid_t  pid = getpid();

    int done = 0;
    // Looping through each client command:
    do {
      // Getting a command like this allows us to automatically format the bytes to a structure and access the pieces of the 
      // structure without parsing anything.
        Command c;
        int status = socket_recvall(chatSocket,&c,sizeof(Command));checkError(status);
	// Converting the code to the proper endian:
        c.code = ntohl(c.code);
        char *errmsg = NULL; 

        // socket is closed on the client side
        if (status != sizeof(Command))
            break;

	// Handling each type of client command. You will add extra cases here to handle new commands from the client.
        switch(c.code){
            case CC_LS: {
                fprintf(stderr, "Process %d: Received LS.\n", pid);
                Payload p;
                char* msg = makeFileList(".");
                if (msg) {
                    int msg_len = strlen(msg) + 1;
                    p.code = htonl(PL_TXT);
                    p.length = htonl(msg_len);
                    status = socket_sendall(chatSocket,&p,sizeof(Payload));checkError(status);
                    status = socket_sendall(chatSocket,msg,msg_len);checkError(status);
                    free(msg);
                }
                else 
                    errmsg = "LS Error: Cannot get file list.";
            }break;
            case CC_GET: { // send the named file back to client
                fprintf(stderr, "Process %d: Received GET.\n", pid);
                Payload p;
                int fileSize = (check_filename(c.arg) < 0) ? -1 : getFileSize(c.arg);
                if (fileSize >= 0) {
                    p.code = htonl(PL_FILE);
                    p.length = htonl(fileSize);
                    status = socket_sendall(chatSocket,&p,sizeof(Payload));checkError(status);
                    status = sendFileOverSocket(c.arg,chatSocket);checkError(status);
                    fprintf(stderr, "GET File [%s] sent\n",c.arg);
                }
                else 
                    errmsg = "GET Error: Invalid file name.";
            }break;
            case CC_PUT: {// save locally a named file sent by the client
                fprintf(stderr, "Process %d: Received PUT.\n", pid);

                Payload p;
                status = socket_recvall(chatSocket,&p,sizeof(p));checkError(status);
                assert(status == sizeof(p));
                p.code = ntohl(p.code);
                p.length = ntohl(p.length);
                // do not change the behavior of put
                if (check_filename(c.arg) == 0) {
                    receiveFileOverSocket(chatSocket,c.arg,".upload",p.length);
                    fprintf(stderr, "PUT File [%s] received\n",c.arg);
                } else {
                    fprintf(stderr, "PUT Error: Invalid file name.\n");
                    receiveFileOverSocket(chatSocket,"/dev/null","",p.length);
                }
            }break;
            case CC_SIZE: { // send the size of the named file back to client
                fprintf(stderr, "Process %d: Received SIZE.\n", pid);
                  Payload p;
                  int fileSize = (check_filename(c.arg) < 0) ? -1 : getFileSize(c.arg);
                  if (fileSize >= 0) {
                      p.code = htonl(PL_SIZE);
                      p.length = htonl(fileSize);
                      status = socket_sendall(chatSocket,&p,sizeof(Payload));checkError(status);
                      fprintf(stderr, "Size of file [%s] sent\n",c.arg);
                  } else {
                      errmsg="SIZE Error: Cannot get file size.";
                  }
            }break;
            case CC_EXIT:
                fprintf(stderr, "Process %d: Received EXIT.\n", pid);
                  done = 1;
                  break;
            default: 
                fprintf(stderr, "Process %d: Unknown command.\n", pid);
                break;
        }
        if (errmsg) {
            fprintf(stderr, "Process %d: %s\n", pid, errmsg);

            int len = strlen(errmsg) + 1;

            Payload p;
            p.code = htonl(PL_ERROR);
            p.length = htonl(len);
            status = socket_sendall(chatSocket,&p,sizeof(Payload));checkError(status);
            if (p.length > 0) {
                status = socket_sendall(chatSocket,errmsg,len);checkError(status);
            }
        }
    } while (!done);
}
