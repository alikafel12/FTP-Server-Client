

/* sid: socket id
 * message: location of the message
 * length: number of bytes to be sent
 *
 * return the number of bytes sent or -1 for error.
 */
int socket_sendall(int sid, void *message, int length)
{
  /*
    Fill this in!
   */
   int sent = 0;

    while(sent < length){
        int r = send(sid, message + sent, length - sent, 0);
        if(r <= 0){
            return -1;
        }
        sent += r;
    }
    //sent = write(sid, message, length);
    return sent;
}

 
/*
 * Opens the file, reads it from memory, and sends it over the given chatsocket
 * 
 * @param fname - string name of the file name to send
 * @param chatSocket - the socket to send the file over.
 */
int     sendFileOverSocket (char* fName, int chatSocket)
{
    char buf[1024];

    int fileSize = getFileSize(fName);
    if (fileSize < 0)
        return -1;

    int nbread = 0, nbRem = fileSize;
    FILE* fp = fopen(fName,"r");
    if (fp == NULL)
        return -1;
    //nbRem is the number of bytes remaining to be send
    while(nbRem != 0) {
      /*

	Fill this in!

    
	Call the necessary functions to read from the file into buf, and send to the file.

	If socket_sendall returns an -1, close the file, and return the -1. xs
       */
	    nbread = fread(buf,sizeof(char), sizeof(buf), fp);
	    int rv = socket_sendall(chatSocket, buf, fileSize);
	    if(rv == -1){
	        fclose(fp);
	        return -1;
	    }
		nbRem -= nbread;
	}
   
    fclose(fp);
    return 0;
}

