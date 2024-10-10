#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]){
    // Check arguments numfiles and writestr are exist.
    if(argc != 3){
        // printf("Ivalid arguments, please input: writer writefile writestr");
        syslog(LOG_USER|LOG_ERR, "Ivalid arguments, please input: writer writefile writestr");
        return 1;
    }

    char *writefile = argv[1];
    char *writestr = argv[2];
    FILE* pFile = fopen(writefile,"w");
    
    // Was file created? 
    if(pFile == NULL){
        // printf("%s can't be create\n", writefile);
        syslog(LOG_USER|LOG_ERR, "%s can't be create", writefile);
    }else{
        fprintf(pFile, "%s", argv[2]);
        syslog(LOG_USER|LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    }

    fclose(pFile);

    return 0;
}