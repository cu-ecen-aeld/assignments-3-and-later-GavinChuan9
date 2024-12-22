#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>

#define PORT 9000
#define BUFFER_SIZE 1024

char *readUntilNewLind(int clientFD){
    char buffer[BUFFER_SIZE] = {};
    char *result = NULL;
    size_t totalSize = 0;

    while(1){
        size_t bytesNum = recv(clientFD, buffer, BUFFER_SIZE, 0);
        if (bytesNum < 0){
            perror("Read error");
            free(result);
            return NULL;
        }else if(bytesNum == 0){
            // EOF
            break;
        }

        // Expand buffer(result)
        result = realloc(result, totalSize + bytesNum);
        // Copy to buffer(result)
        memcpy(result + totalSize, buffer, bytesNum);
        totalSize += bytesNum;

        if(buffer[bytesNum-1] == '\n')
            break;
    };

    return result;
}

int main(){
    int serverFD, clientFD, one = 1;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char clientIP[INET_ADDRSTRLEN];
    const char *filename = "/var/tmp/aesdsocketdata";
    FILE *file;
    char buffer[BUFFER_SIZE] = {};

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
        printf("Accepted connection from %s\n", clientIP);

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
        }else{
            printf("Open file\n");
        }

        while(fgets(buffer, BUFFER_SIZE, file) != NULL){
            send(clientFD, buffer, strlen(buffer), 0);
            memset(buffer, 0, BUFFER_SIZE);
        }

        free(result);
        // Close sockets
        close(clientFD);
        fclose(file);
        printf("Closed connection from %s\n", clientIP);
        printf("----------------------------\n");
    }
    close(serverFD);
    // Close syslog
    closelog();

    return 0;
}
