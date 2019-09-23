#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>


#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 8080

// Notice, we include the command.h functions so this file can use the 'things' defined in command.h
#include "command.h"


#include "send.h"
#include "receive.h"
#define     BUF_SIZE     (MAX_FILENAME_LEN + 1)

/*
 * Checks the error of a socket call return value. If status < 0, will print error and exit.
 * 
 * @param status - the return value from the socket library function
 * @param line - the line number of the call to this function. If unsure, just pass __LINE__
 */
void checkError(int status,int line)
{
    if (status < 0) {
        printf("socket error(%d)-%d: [%s]\n",getpid(),line,strerror(errno));
        exit(-1);
    }
}

// return 0 on success or EOF on EOF
int skip_to_EOL()
{
    int c;

    do {
        c = getchar();
    } while (c != EOF && c != '\n');
    if (c == EOF)
        return EOF;
    return 0;
}

// buf_size should be 2 at least.
// > 0: length of the filename
// 0: no file name
// -1: EOF
// -2: too long
// should not use buf as string if resutn value < 0
int get_filename(char *buf, int buf_size)
{
    int c;
    int cnt = 0;

    if (buf_size <= 1) {
        skip_to_EOL();
        return -2;
    }

    // skip the spaces or tabs
    do {
        c = getchar();
    } while (c == ' ' || c == '\t');

    while (c != EOF && ! isspace(c)) {
        if (cnt < buf_size - 1) 
            buf[cnt++] = c;
        else {
            skip_to_EOL();
            return -2;
        }
        c = getchar();
    }
    buf[cnt] = 0;
    if (c == EOF)
        return EOF;
    if (c == '\n' && cnt) 
        ungetc(c, stdin);
    return cnt;
}

void display_error(int sid, Payload *p)
{
    if (p->code != PL_ERROR) {
        fprintf(stderr, "Fatal Error: invalid payload.\n");
        exit(1);
    }
    int len = p->length;
    if (len) { 
        char *buf = malloc(len + 1);
        assert (buf);

        int status = socket_recvall(sid, buf, len); checkError(status,__LINE__);
        buf[len] = 0;
        fprintf(stderr, "%s\n", buf);
        free(buf);
    } else  {
        fprintf(stderr, "Error Code. No error message.\n");
    }
}

void doLSCommand(int sid);
void doExitCommand(int sid);
void doGETCommand(int sid);
void doPUTCommand(int sid);
void doSIZECommand(int sid);
int  doMGETCommand(int sid);

int main(int argc,char* argv[])
{
    char fName[BUF_SIZE];


    char* host = DEFAULT_HOST;
    short int port = DEFAULT_PORT;

    /*

      Fill this in!


      If the user specified the host of the server on the command line, set host to it. 

      Otherwise, set host to DEFAULT_HOST

      If the user specified the port of the server on the command line, set port to it.

      Otherwise, set port to DEFAULT_PORT
     */
     if(argc >= 2){
         host = argv[1];
     }
     
     if(argc >= 3){
         port = atoi(argv[2]);
     }


    // Create a socket to connect to the server
    int sid = socket(PF_INET,SOCK_STREAM,0);
    assert(sid >=  0);

    // make connection
    struct sockaddr_in srv;
    // Performs the DNS lookup for the hostname and returns a hostent structure with the results
    struct hostent *server = gethostbyname(host);
    // Setting the IP address to use IPv4
    srv.sin_family = AF_INET;
    // Setting the port of the socket address:
    srv.sin_port   = htons(port); /* htons stands for 'host to network short' */
    // Copying the address to the address slot in the sockaddr_in struct:
    memcpy(&srv.sin_addr.s_addr,server->h_addr,server->h_length);

    // The syscall that connects to the given socket address:
    int status = connect(sid,(struct sockaddr*)&srv,sizeof(srv));
    checkError(status,__LINE__);

    int done = 0;
    /*
    * The Main while loop that gets commands from the user and executes them.
    * 
    * You have to add some new commands to the if statement. Make sure you understand what strncmp is doing exactly
    * > man 3 strncmp
    */
    do {
        char opcode[32];
        if (scanf("%10s",opcode) != 1)
            break;
        if (strncmp(opcode,"lsize",5) == 0) {  // check lsize before ls
            int rv; 
            rv = get_filename(fName, sizeof fName);
            while (rv == 0)  {
                printf("Give a local filename: ");
                rv = get_filename(fName, sizeof fName);
            }
            skip_to_EOL();
            if (rv > 0) {
                printf("File size of %s is %d bytes\n", fName, getFileSize(fName) );
            }
            else if (rv == -2)
                fprintf(stderr, "File name is too long.\n");
        } else if (strncmp(opcode,"exit",4) == 0) {
            doExitCommand(sid);
            done = 1;
        } else if (strncmp(opcode,"lls",3) == 0) {
            skip_to_EOL();
            char* fList = makeFileList(".");
            if (fList) {
                printf("[\n%s]\n", fList);
                free(fList);
            }
            else 
                fprintf(stderr, "LLS Error.\n");
        } else if (strncmp(opcode,"ls",2) == 0) {
            doLSCommand(sid);
        } else if (strncmp(opcode,"size",4) == 0) {
            doSIZECommand(sid);
        } else if (strncmp(opcode,"get",3)==0) {
            doGETCommand(sid);
        } else if (strncmp(opcode,"put",3)==0) {
            doPUTCommand(sid);
        } else if (strncmp(opcode,"mget",4)==0) {
            doMGETCommand(sid);
        } else {
            fprintf(stderr, "Invalid command.\n");
            skip_to_EOL();
        }
    } while(!done);

    close(sid);
    return 0;
}

/*
 * Executes the LS command. This is a remote LS command that prints out the contents of the 
 * local directory of the server at the client.
 * 
 * @param sid - the socket file descriptor.
 */
void doLSCommand(int sid)
{
    Command c;
    Payload p;
    int status;

    skip_to_EOL();

    c.code = htonl(CC_LS);
    c.arg[0] = 0;

    status = socket_sendall(sid,&c,sizeof(c));checkError(status,__LINE__);
    status = socket_recvall(sid,&p,sizeof(p));checkError(status,__LINE__);

    p.code = ntohl(p.code);
    p.length = ntohl(p.length);

    if(p.code == PL_TXT) {
        char* buf = malloc(sizeof(char)*p.length + 1);
        assert(buf != NULL);
        status = socket_recvall(sid, buf, p.length);checkError(status,__LINE__);
        buf[p.length] = 0;
        printf("Got: [\n%s]\n",buf);
        free(buf);
    }
    else 
        display_error(sid, &p);
}

/*
 * Executes the GET command. This grabs prompts the user for a file to get from the server, and then retreives it.
 * 
 * @param sid - the socket file descriptor.
 */
void doGETCommand(int sid)
{
    Command c;
    Payload p;
    int status;

    c.code = htonl(CC_GET);
    status = get_filename(c.arg, BUF_SIZE);
    while (status == 0) {
        printf("Give a filename:\n");
        status = get_filename(c.arg, BUF_SIZE);
    }
    if (status < 0)
        return;
    skip_to_EOL();

    status = socket_sendall(sid,&c,sizeof(c));checkError(status,__LINE__);
    status = socket_recvall(sid,&p,sizeof(p));checkError(status,__LINE__);
    p.code = ntohl(p.code);
    p.length = ntohl(p.length);
    if (p.code == PL_FILE) {
        receiveFileOverSocket(sid,c.arg,".download",p.length);
        printf("transfer done\n");
    }
    else
        display_error(sid, &p);
}


/*
 * Executes the put command. This prompts the user for a local filename and if it is
 * valid, sends it to the server.
 * 
 * @param sid - the socket file descriptor.
 */
void doPUTCommand(int sid)
{
    Command c;
    Payload p;
    int status;
    c.code = htonl(CC_PUT);
    printf("Give a local filename:");
    char fName[256];
    scanf("%s",fName);
    strncpy(c.arg,fName,255);
    c.arg[255] = 0;
    status = send(sid,&c,sizeof(c),0);checkError(status,__LINE__);
    int fs = getFileSize(c.arg);
    p.code = htonl(PL_FILE);
    p.length = htonl(fs);
    status = send(sid,&p,sizeof(p),0);checkError(status,__LINE__);
    sendFileOverSocket(c.arg,sid);
    printf("transfer done\n");
}


/*
 * Executes the exit command. This tells the server to exit.
 * 
 * @param sid - the socket file descriptor.
 */
void doExitCommand(int sid)
{
    Command c;
    int status;
    c.code = htonl(CC_EXIT);
    c.arg[0] = 0;
    status = socket_sendall(sid,&c,sizeof(c));checkError(status,__LINE__);
}

/*
  Gets the size of a file on the server.
 */

void doSIZECommand(int sid)
{
    Command c;
    Payload p;
    int status;

    c.code = htonl(CC_SIZE);
    status = get_filename(c.arg, BUF_SIZE);
    while (status == 0) {
        printf("Give a remote filename:\n");
        status = get_filename(c.arg, BUF_SIZE);
    }
    if (status < 0)
        return;
    skip_to_EOL();

    status = socket_sendall(sid,&c,sizeof(c));checkError(status,__LINE__);
    status = socket_recvall(sid,&p,sizeof(p));checkError(status,__LINE__);
    p.code = ntohl(p.code);
    p.length = ntohl(p.length);
    if (p.code == PL_SIZE)   
        printf("File size of %s is %d bytes\n", c.arg, p.length );
    else 
        display_error(sid, &p);
}


/*
  The #include here basically tells the compiler to past the contents of mget.h into this file here.

  I'm going it this way so I can swap out my mget function for yours. 
 */
#include "mget.h"
