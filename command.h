#ifndef __COMMAND_H
#define __COMMAND_H

#define CC_LS	0
#define CC_GET	1
#define CC_PUT  2
#define CC_EXIT 3
#define CC_SIZE 4

#define PL_TXT  0
#define PL_FILE 1
#define PL_SIZE 2
#define PL_ERROR 9

#define MAX_FILENAME_LEN 255

typedef struct CommantTag {
	int  code;
	char arg[MAX_FILENAME_LEN + 1];
} Command;

typedef struct PayloadTag {
	int code;
	int length;
} Payload;

char* makeFileList(char* path);
int getFileSize(char* fName);




int check_filename(char *fn);

#endif
