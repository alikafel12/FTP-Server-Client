#include "command.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>


/* Check filename.
 *
 * Assume the max lenth is MAX_FILENAME_LEN.
 *
 * The buffer must have at least MAX_FILENAME_LEN + 1 bytes.
 *
 * A file name cannot have '/'.
 *
 * return -1 for error.
 */
int check_filename(char *fn)
{
    fn[MAX_FILENAME_LEN] = 0;
    if (strchr(fn, '/') != NULL) 
        return -1;
    return 0;
}





// Return a list of file/dir names seprated by the newline.
// Return NULL on error
char* makeFileList(char* path)
{
    DIR* theCWD = opendir(path);
    struct dirent* cur = NULL;
    cur = readdir(theCWD);
    int ttlBytes = 0;
    while(cur != NULL) {
        ttlBytes += strlen(cur->d_name) + 1;
        cur = readdir(theCWD);
    }
    char* txt = malloc((ttlBytes + 10)*sizeof(char));
    if (!txt) {
        closedir(theCWD);
        return NULL;
    }
    rewinddir(theCWD);
    cur = readdir(theCWD);
    *txt = 0;
    while(cur != NULL) {
        strcat(txt,cur->d_name);
        strcat(txt,"\n");
        cur = readdir(theCWD);
    }
    closedir(theCWD);
    return txt;
}

// return the size of a file
// -1 on error.
int     getFileSize(char* fName)
{
    FILE* f = fopen(fName,"r");
    if (f == NULL)
        return -1;
    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return (int)sz;
}


