#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

int main(int argc, char** argv) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    uint16_t portNum;
    printf("Enter port number: ");
    scanf("%hd%*c", &portNum);

	fd_set sockets;
	FD_ZERO(&sockets); // Just in case this isn't already zero, just make sure it is
	FD_SET(sockfd,&sockets); // put that socket into that set.

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(portNum); // For port numbers, both TCP and UDP use 16 bit numbers
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	// Make the address by calling bind
	bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

	// Listen for any connections (we're using connect() on the client).
	// The 10 is a backlog. We're going to have a line of code that accepts connections
	// This is the number of connections the OS is willing to hold on before we call accept,
	// not the number we are going to accept. If 20 connect, the OS will only "save" the first 10.
	listen(sockfd, 10);

	// Problem with original while(1) {accept(); recv();}  is that we are listening to one, 
	// then waiting to receive from one, then two, then waiting to receive from two. The OS 
	// can buffer data from C2 while we're waiting on C1, but that sucks. We want to handle
	// everything at once.
	//
	// Two ways to handle this. Use select(), or use multithreading.
	while (1) {
		fd_set tmpset=sockets;
		int r = select(FD_SETSIZE, &tmpset,NULL, NULL, NULL);
		// Check if select identified the receiving socket as having data to read
		if (FD_ISSET(sockfd, &tmpset)) { // If it did then add the socket to our set
			socklen_t len = sizeof(struct sockaddr_in);

			// Like recvfrom(), except we aren't specifying the address of the client. We're
			// asking for the address of the client. The clientaddr will be filled with the information from the client
			// accept() returns another socket as its return value. That socket is the one we will need to use to communicate
			// with the client. If we accept multiple clients, then each time we accept(), we talk to a different client. We talk to 
			// each individual client by using each socket. One socket, per client that the server is communicating with.
			int clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
			FD_SET(clientsocket, &sockets);
		}
		
		// For every possible socket in our set
		for (int i = 0; i < FD_SETSIZE; i++) {
			// If there is a socket at the index
			if (FD_ISSET(i, &tmpset) && i !=sockfd) {

                /* Receive Requested File Name */
                char buffer[65534];
				char fileName[1024];
				int n = recv(i, fileName, 1024, 0);
				printf("Got file request from client: %s\n", fileName);
                printf("Searching for file...\n");

                /* Read File */
                FILE *file;
                file = fopen(fileName, "r");

                /* If File Doesn't Exist */
                if (file == NULL) {
                    printf("File could not be found...\n");
                    printf("Notifying client...\n");

                    /* Tell the client that the file was not found */
                    strcpy(buffer, "FILE_NOT_FOUND");
                    send(i, buffer, sizeof(buffer), 0);
                    memset(buffer, 0, sizeof(buffer));

                    /* close the socket */
                    FD_CLR(i, &sockets);
                    close(i);
                    break;
                } else {
                    /* Else File Does Exist */
                    printf("File Found\n");

                    /* Tell the client that the file was found */
                    strcpy(buffer, "FILE_FOUND");
                    send(i, buffer, sizeof(buffer), 0);
                    memset(buffer, 0, sizeof(buffer));

                    /* Send file to client */
                    printf("Sending file to client...\n");
                    printf("\treading chunk...\n");
                    int iBytesRead = fread(buffer, 1, sizeof(buffer), file) > 0;
                    while (iBytesRead > 0) {
                        /* Send file chunk & reset buffer */
                        printf("\tsending chunk...\n");
                        send(i, buffer, sizeof(buffer), 0);
                        memset(buffer, 0, sizeof(buffer));

                        /* Read next portion of file */
                        printf("\treading chunk...\n");
                        iBytesRead = fread(buffer, 1, sizeof(buffer), file) > 0;
                    }
                    printf("File successfully sent...\n");
                }

				// If we were planning on continuing talking to this client, we would not close the socket
				FD_CLR(i, &sockets); // If you are closing the socket, make sure you take it out of the set
                close(i);
			}
		}
	}
}
