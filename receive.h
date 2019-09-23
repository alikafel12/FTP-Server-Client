/* fd: socket id
 * p: buffer to place received bytes into 
 * nbytes: number of bytes to read from the socket
 *
 * return the number of bytes received.
 * To be clear: Unlike in the lab, you shouldn't stop reading if you encounter a newline!
 */
int socket_recvall(int fd, void *p, int nbytes)
{
    int received = 0;

    while (received < nbytes) {
      /* Fill this in! 
	 Remember to check for errors when reading.
       */
       int r = recv(fd, p + received, nbytes - received, 0);
       if(r < 0){
           return -1;
       }
       received += r;
    }
    return received;
}

/*
 * Receives a file over a socket. Saves it as a new file of the form
 *      <fname>.<ext>
 * 
 * @param sid - the socket ID to receive the file on
 * @param fname - string name of the file name to send
 * @param ext - string file extension (i.e. .txt/.pdf/.c/.h/etc)
 * @param fSize - the file size to receive in bytes
 */
int     receiveFileOverSocket(int sid, char* fname, char* ext, int fSize)
{
    if (strlen(fname) + strlen(ext) > MAX_FILENAME_LEN)
        return -1;

    char* buf = malloc(sizeof(char)*fSize);
    if (buf == NULL)
        return -1;
    /* 
       call a function to recieve the file contens over the socket
    */
    int rec  = 0,rem = fSize;
    while (rem != 0) {
		int rv = recv(sid,buf+rec,rem,0);
		rec += rv;
		rem -= rv;
		if (rv < 0) {
            free(buf);
            return rv;
        }
	}

    /*
      Open the file with name fName + ext. For example, if fName is 'thing', and ext is '.txt', then you open the file: thing.txt

      Then, write the buffer contents to the file.

      Don't forget to return -1 if you can't open the file!
     */
    char fn[512];
	strcpy(fn,fname);
	strcat(fn,ext);
	FILE* fd = fopen(fn,"w");
	fwrite(buf,sizeof(char),fSize,fd);
	fclose(fd);
	free(buf);	
    return 0;
}
