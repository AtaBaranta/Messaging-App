#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENT 100

struct Message{
	bool isResponse;
	bool isNewMessage;
	bool isNewMessageCount;
	bool isNewMessageText;
	bool isReaded;
	int senderId;
};

void deleteNewLine(char* buffer){
	if (buffer != NULL) {
        size_t length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
        }
    }
}

int connectSocket(int* clientFd, int* status, struct sockaddr_in serv_addr){
	if ((*clientFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error on socket creation!\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<= 0) {
		printf("\nInvalid address/ Address not supported!\n");
		return -1;
	}
	if ((*status = connect(*clientFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
		printf("Connection Failed!\n");
		return -1;
	}
	return 1;
}

void sendMessage(int clientFd){
	char buffer[MAX_BUFFER_SIZE] = { 0 };
	printf("Enter Message: ");	

	fgets(buffer, sizeof(buffer), stdin);
	deleteNewLine(buffer);

    send(clientFd, buffer, strlen(buffer), 0);
    memset(buffer, 0, MAX_BUFFER_SIZE);  
}

void *listenerForSender(void *arg){
	int *client_fd = (int*)arg;
	char buffer[MAX_BUFFER_SIZE] = { 0 };
	char* option = (char*)malloc((MAX_BUFFER_SIZE) * sizeof(char));
	while (1) {
		if(strcasecmp(option, "5") == 10 ){
			printf("Whose message do you want to read? : ");
			fgets(buffer, sizeof(buffer), stdin);
			send(*client_fd, buffer, strlen(buffer), 0);
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			option[0] = '6';
			sleep(1);
		}
		else if(strcasecmp(option, "6") == 10 ){
			printf("To delete a message, Enter the line (or 0 to cont.): ");
			fgets(buffer, sizeof(buffer), stdin);
			send(*client_fd, buffer, strlen(buffer), 0);
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			strcpy(option, buffer);
			sleep(1);
		}
		else{
			printf("1. List Contacts\n");
			printf("2. Add User\n");
			printf("3. Delete User\n");
			printf("4. Send Message\n");
			printf("5. Check Messages\n");
			printf("Exit with 'X'\n");
			printf("Please type your choice: ");
			fgets(buffer, sizeof(buffer), stdin);
			send(*client_fd, buffer, strlen(buffer), 0);

			size_t length = strlen(buffer);
			
			strcpy(option, buffer);

			if(strcasecmp(&buffer[0],"1")==10){ // list contact
				memset(buffer, 0, MAX_BUFFER_SIZE); 
			}
			else if(strcasecmp(&buffer[0],"2")==10){ // add user
				memset(buffer, 0, MAX_BUFFER_SIZE);  
				sendMessage(*client_fd); 
			}
			else if(strcasecmp(&buffer[0],"3")==10){ // delete user
				memset(buffer, 0, MAX_BUFFER_SIZE);  
				sendMessage(*client_fd); 
			}
			else if(strcasecmp(&buffer[0],"4")==10){ // send message
				memset(buffer, 0, MAX_BUFFER_SIZE);  
				sendMessage(*client_fd);
			}	
			else if(strcasecmp(&buffer[0],"5")==10){ // Check Messages
				memset(buffer, 0, MAX_BUFFER_SIZE);  
			}
			else if(strcasecmp(&buffer[0],"X")==10){ // Exit
				close(*client_fd);
				pthread_exit(NULL);
			}
			else{
				printf("Error on selecting options. Try again a valid option!\n");
				memset(buffer, 0, MAX_BUFFER_SIZE);  
			}
		}
		
    }
}

void *listenerForReciever(void *arg){
	int* client_fd = (int*)arg;
	char buffer[MAX_BUFFER_SIZE] = { 0 };
	struct Message *message = (struct Message*)malloc(sizeof(struct Message));
	while (1)
	{ 
		ssize_t dataLen = recv(*client_fd, message, sizeof(struct Message), 0);
        if (dataLen <= 0) {
            pthread_exit(NULL);
        }
		char* m = "information";
		send(*client_fd, m, strlen(m), 0);
		ssize_t dataLen_2 = recv(*client_fd, buffer, MAX_BUFFER_SIZE, 0);
		if (dataLen_2 <= 0) {
            pthread_exit(NULL);
        }

		if(message->isResponse){
			//deleteNewLine(buffer);
			printf("\n\tInformation: \n%s\n", buffer);
			
		}
		else if(message->isNewMessageCount){
			//deleteNewLine(buffer);
			printf("\n\tMessages: \n%s\n", buffer);
		}
		else if(message->isNewMessageText){
			//deleteNewLine(buffer);
			printf("\n\tNew Messages: \n%s\n", buffer);
		}
		else if(message->isNewMessage){
			printf("\n\tNew Message From Client %d:\n\t\t--> %s\n", message->senderId, buffer);
		}
		memset(buffer, 0, MAX_BUFFER_SIZE); 
	}
	
}

int main(int argc, char const* argv[])
{
	int status, valread, client_fd;
	struct sockaddr_in serv_addr;
	char buffer[MAX_BUFFER_SIZE] = { 0 };
	pthread_t **Threads = (pthread_t **)malloc(2*sizeof(pthread_t*));
	Threads[0] = (pthread_t*)malloc(sizeof(pthread_t));
	if (Threads[0] == NULL) {
        perror("Error during the allocating memeory for a new thread!\n");
        exit(EXIT_FAILURE);
    }
	Threads[1] = (pthread_t*)malloc(sizeof(pthread_t));
	if (Threads[1] == NULL) {
        perror("Error during the allocating memeory for a new thread!\n");
        exit(EXIT_FAILURE);
    }

	printf("User ID: %s\n", argv[1]);
    strcpy(buffer, argv[1]);
	deleteNewLine(buffer);

	int successConnection = connectSocket(&client_fd, &status, serv_addr);
	if(!successConnection){
		return -1;
	}
	
    send(client_fd, buffer, strlen(buffer), 0);
	memset(buffer, 0, MAX_BUFFER_SIZE); 
	recv(client_fd, buffer, MAX_BUFFER_SIZE, 0);
	if(strcasecmp(buffer, "0") == 0){//invalid Id or user already connected
		printf("User ID is invalid or User is already connected!\n" );
		memset(buffer, 0, MAX_BUFFER_SIZE); 
		close(client_fd);
		return 0;
	}
	printf("Successful Login!\n" );
	memset(buffer, 0, MAX_BUFFER_SIZE); 

	recv(client_fd, buffer, MAX_BUFFER_SIZE, 0);
	if(strcasecmp(buffer, "00") == 0){
		printf("Name information is missing. Please enter your name: ");
		fgets(buffer, sizeof(buffer), stdin);
    	deleteNewLine(buffer);
		send(client_fd, buffer, strlen(buffer), 0);
		memset(buffer, 0, MAX_BUFFER_SIZE); 

		printf("Phone information is missing. Please enter your phone number: ");
		fgets(buffer, sizeof(buffer), stdin);
    	deleteNewLine(buffer);
		send(client_fd, buffer, strlen(buffer), 0);
		memset(buffer, 0, MAX_BUFFER_SIZE); 
	}
	else if(strcasecmp(buffer, "0") == 0){
		printf("Name information is missing. Please enter your name: ");
		fgets(buffer, sizeof(buffer), stdin);
    	deleteNewLine(buffer);
		send(client_fd, buffer, strlen(buffer), 0);
		memset(buffer, 0, MAX_BUFFER_SIZE); 
	}
	else if(strcasecmp(buffer, "000") == 0){
		printf("Phone information is missing. Please enter your phone number: ");
		fgets(buffer, sizeof(buffer), stdin);
    	deleteNewLine(buffer);
		send(client_fd, buffer, strlen(buffer), 0);
		memset(buffer, 0, MAX_BUFFER_SIZE); 
	}
	
	if (pthread_create(Threads[1], NULL, listenerForSender, &client_fd) != 0) {
            perror("Error creating thread\n");
            close(client_fd);
    }
	if (pthread_create(Threads[0], NULL, listenerForReciever, &client_fd) != 0) {
            perror("Error creating thread\n");
            close(client_fd);
    }

	for (int i = 0; i < 2; ++i) {
        if (pthread_join(*Threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            return 1;
        }
    }
	close(client_fd);
	return 0;
}
