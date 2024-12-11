#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main(){
    int serverFD, clientFD;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE] = {};
    socklen_t addr_len = sizeof(address);
    char clientIP[INET_ADDRSTRLEN];

    if((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
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

    if((clientFD = accept(serverFD, (struct sockaddr *)&address, &addr_len)) < 0){
        perror("accept failed");
        close(serverFD);
        return -1;
    }

    // Show the IP of client
    inet_ntop(AF_INET, &address.sin_addr, clientIP, sizeof(clientIP));
    syslog(LOG_INFO, "Accepted connection from %s\n", clientIP);

    // Receive data from client
    read(clientFD, buffer, BUFFER_SIZE);
    printf("Client: %s\n", buffer);

    // Close sockets
    close(clientFD);
    close(serverFD);
    syslog(LOG_INFO, "Closed connection from %s\n", clientIP);

    // Close syslog
    closelog();

    return 0;
}
