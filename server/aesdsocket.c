#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>

#define PORT 9000
#define BUFFER_SIZE 1024

const char *filename = "/var/tmp/aesdsocketdata";
static int serverFD, clientFD;
static FILE *file;

void handleSIGINT(int sig){
    syslog(LOG_DEBUG, "Caught signal, exiting");

    // Closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
    close(serverFD);
    close(clientFD);
    
    if(remove(filename) == 0){
        printf("File %s deleted successfully.\n", filename);
    }else
        perror("Error deleting the file");

    // Close syslog
    closelog();

    exit(0);
}

void handleSIGTERM(int sig){
    // printf("Caught signal, exiting");
    syslog(LOG_DEBUG, "Caught signal, exiting");

    // Closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
    close(serverFD);
    close(clientFD);
    
    if(remove(filename) == 0){
        printf("File %s deleted successfully.\n", filename);
    }else
        perror("Error deleting the file");

    // Close syslog
    closelog();

    exit(0);
}

char *readUntilNewLind(int clientFD){
    char buffer[BUFFER_SIZE] = {};
    char *result = NULL;
    size_t totalSize = 0;

    while(1){
        memset(buffer, 0, sizeof(buffer));
        size_t bytesNum = recv(clientFD, buffer, BUFFER_SIZE, 0);
        if (bytesNum < 0){
            perror("Read error");
            free(result);
            return NULL;
        }else if(bytesNum == 0){
            // Client was close
            break;
        }

        // Expand buffer(result)
        result = realloc(result, totalSize + bytesNum);
        // Copy to buffer(result)
        memcpy(result + totalSize, buffer, bytesNum);
        totalSize += bytesNum;
        if(buffer[bytesNum-1] == '\n'){
            // EOF
            result[totalSize] = '\0';
            break;
        }
    };
    return result;
}

int main(){
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char clientIP[INET_ADDRSTRLEN];
    char buffer[BUFFER_SIZE] = {};
    int one = 1;

    if (signal(SIGINT, handleSIGINT) == SIG_ERR) {
        perror("Unable to set SIGINT handler");
        exit(1);
    }

    if (signal(SIGTERM, handleSIGTERM) == SIG_ERR) {
        perror("Unable to set SIGTERM handler");
        exit(1);
    }

    if((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        return -1;
    }

    // Aviod bind failed: Address already in use
    if(setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0){
        close(serverFD);
        return -1;
    }

    // Config the IP of server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(serverFD, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        close(serverFD);
        return -1;
    }
    
    // Open syslog，named "App aesdsocket"
    openlog("App aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    if(listen(serverFD, 1) < 0){
        perror("listen failed");
        close(serverFD);
        return -1;
    }

    while(1){
        if((clientFD = accept(serverFD, (struct sockaddr *)&address, &addr_len)) < 0){
            perror("accept failed");
            close(serverFD);
            return -1;
        }

        // Show the IP of client
        inet_ntop(AF_INET, &address.sin_addr, clientIP, sizeof(clientIP));
        syslog(LOG_DEBUG, "Accepted connection from %s\n", clientIP);

        // Open/Create file
        file = fopen(filename, "a+");
        if(file == NULL){
            printf("Error opening file\n");
            return -1;
        }

        // Receive data from client
        char *result = readUntilNewLind(clientFD);

        // Write data to file
        if(fprintf(file, "%s", result) < 0){
            printf("Error writing to file");
            fclose(file);
            return -1;
        }
        fclose(file);

        // Send file data to client    
        file = fopen(filename, "r");
        if(file == NULL){
            printf("Cant open file\n");
            return -1;
        }

        while(fgets(buffer, BUFFER_SIZE, file) != NULL){
            send(clientFD, buffer, strlen(buffer), 0);
            memset(buffer, 0, BUFFER_SIZE);
        }

        free(result);
        // Close sockets
        close(clientFD);
        fclose(file);
        syslog(LOG_DEBUG, "Closed connection from %s\n", clientIP);
    }
    close(serverFD);
    // Close syslog
    closelog();

    return 0;
}
