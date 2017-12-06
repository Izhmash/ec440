/* Test of Character Device -- Version 1.0

usage:
	dtest <device> [value | R | S | Z ]*

writes each value as bytes (NOT INCLUDING A FINAL NULL CHARACTER)
Z writes a null character
S writes a space
R reads and prints result string (including a newline afterwards if not already there)
Each action prints out a description.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 25

int main(int argc, char *arg[]){
	int fd;
	char *devname;  // name of the device we are testing
	char *action;   // name for current accumulation
	char *result;   // result to print out
	char *s;        // current action string
	char nullc=0;
	int i,len;
	char buffer[BUFSIZE];

	if(argc<2) return -1;

	devname=arg[1];
	fd=open(devname,O_RDWR);
	if (fd<0){
		printf("Could not open file %s\n",devname);
		perror("*** open device");
		return -1;
	}
	for(i=2;i<argc;++i){
		s=arg[i];
		if (strcmp(s,"R")==0){ // read action
			action="read";
			len=read(fd,buffer,BUFSIZE); // read data (assume string already)
			if (len>0){
				result=buffer;
				// check if need to add a \n at the end?
				if (index(buffer,'\n')==NULL ){
					strncat(buffer,"\n",BUFSIZE);
				}
			} else if (len==0){
				result="EOF\n";
			} else {
				result="*ERR*\n";
			}
		} else if (strcmp(s,"Z")==0){ // write a null character
			len=write(fd,&nullc,1);
			action="write";
			result="null\n";
		} else if (strcmp(s,"S")==0){// write a space
			write(fd," ",1);
			action="write";
			result="space\n";
		} else {
			action="write";
			strncpy(buffer,"\"",BUFSIZE);
			strncat(buffer,s,BUFSIZE);
			strncat(buffer,"\"\n",BUFSIZE);
			result=buffer;
			len=write(fd,s,strlen(s)); // does NOT include final null character
			if (len<0) perror("*** attempt to write data");
		}
		printf("%6s: %s",action,result);
	}

}
