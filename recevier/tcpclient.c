#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
	// Make sure to close your sockets. If you don't close your sockets, the operating
	// system will keep that socket open for a number of minutes and will mess up testing
	// the next time you run the program. This can also happen when you close client and 
	// server in the wrong order
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    char ipaddr[20];
    printf("Enter server IP address: ");
    scanf("%s%*c", ipaddr);

    uint16_t portNum;
    printf("Enter port number: ");
    scanf("%hd%*c", &portNum);

    char fileName[1024];
    printf("Enter requested file name: ");
    scanf("%s%*c", fileName);

	struct sockaddr_in serveraddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(portNum);
	serveraddr.sin_addr.s_addr=inet_addr(ipaddr);

	// Use connect() to connect the socket to the address
	int n = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (n < 0) { // make sure that everything is good
		printf("There was a problem connecting\n");
		close(sockfd);
		return 1;
	}

	// now using send() instead of sendto(). Because we used to include
	// where we were sending, but now we implicitly know because we used
	// connect.
	send(sockfd, fileName, strlen(fileName) + 1, 0); // Why send len(line) + 1

    // Get confirmation of file discovery
    char buffer[1024];
    recv(sockfd, &buffer, 1024, 0);

    // Check if file was found
    if (strcmp(buffer, "FILE_NOT_FOUND") == 0) {
        // File was not found, close client
        printf("Server could not find file. Closing client...\n");
        close(sockfd);
        return 1;
    } else if (strcmp(buffer, "FILE_FOUND") == 0) {
        // File was found, prepare to receive file
        printf("Server found file. Receiving contents...\n");

        // Open new file to write to
        FILE* file;
        file = fopen(fileName, "w");

        /* Receive File */
        memset(buffer, 0, sizeof(buffer));
        printf("%s\n", buffer);
        // TODO: somethings wrong with this received
        int iBytesReceived = recv(sockfd, buffer, sizeof(buffer), 0);
        while (iBytesReceived > 0) {
            printf("Buffer Received:\n%s\n", buffer);
            // TODO: printnf, basically just print till ur done

            // Write buffer to file & reset buffer
            fwrite(buffer, sizeof(char), iBytesReceived, file);
            memset(buffer, 0, sizeof(buffer));

            // Receive next chunk
            iBytesReceived = recv(sockfd, &buffer, sizeof(buffer), 0);
        }

//        while (recv(sockfd, &buffer, 1024, 0) > 0) {
//            // TODO: check these sizes, buffer may be less than 1024 at EOF.
//            fwrite(buffer, 1, sizeof(buffer), file);
//        }
        fclose(file);
        printf("File successfully written. Closing client.\n");
    }

	close(sockfd);
	return 0;

}
