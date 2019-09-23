int doMGETCommand(int sid)
{
    Command c;
    Payload p;
    int status;
    int  rv;

    c.code = htonl(CC_GET);
    rv = get_filename(c.arg, BUF_SIZE);
    while (rv == 0)  {
        printf("Give a list of filenames: ");
        rv = get_filename(c.arg, BUF_SIZE);
    }
    while(rv > 0)
    {
      /*
	Fill this in!

	We've filled in the Command struct, so you just need to call a function to send it to the server.

	Then, you need to call a function to receive the payload into p, and then change the byte order from network to host. 
       */
       status = socket_sendall(sid,&c,sizeof(c));checkError(status,__LINE__);
       status = socket_recvall(sid,&p,sizeof(p));checkError(status,__LINE__);
       p.code = ntohl(p.code);
       p.length = ntohl(p.length);
       
        if (p.code == PL_FILE) {
            receiveFileOverSocket(sid, c.arg, ".download", p.length);
            rv = get_filename(c.arg, BUF_SIZE);
        }
        else {
            display_error(sid, &p); 
            skip_to_EOL();
            rv = -3;
        }
    }
    if (rv == 0)  
        printf("transfer done.\n");
    else if (rv == -2) 
        printf("Long file name.\n");
    return rv;
}
