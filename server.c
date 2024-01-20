#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <math.h> 

#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENT 100
pthread_mutex_t clientsHeadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t contactMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t messageArchiveMutex = PTHREAD_MUTEX_INITIALIZER;

struct Client{
	int id;
	int socket;
	char *name; 
	int isOnline;
	char *phoneNo;
	int *contactsId;
	int numOfContact;
	struct Client *next;
};
struct User{
	struct Client *Client;
	struct Client *ClientsHead;
};
struct Message{
	bool isResponse;
	bool isNewMessage;
	bool isNewMessageCount;
	bool isNewMessageText;
	bool isReaded;
	int senderId;
};
struct ArchiveMessage{
	int toId;
	int fromId;
	int isReaded;
	char *text;
	struct ArchiveMessage *next;
};

volatile sig_atomic_t ctrlCReceived = 0;

// Signal handler function
void ctrlCHandler(int signal) {
    if (signal == SIGINT) {
        ctrlCReceived = 1;
    }
}

struct Client* textDeserializer(char * line){
	struct Client* newClient = (struct Client*)malloc(sizeof(struct Client));
	char delimiter = ';';
	int delimiterCounter = 0;
	int partitionLen = 0;
	char *partition = malloc(sizeof(char));
	int i;
	for (i = 0; i < strlen(line); i++)
	{
		if(line[i] == delimiter){
			if(delimiterCounter == 0){//id
				sscanf(partition, "%d", &newClient->id);
				partitionLen = 0;
				partition = NULL;
			}
			else if(delimiterCounter == 1){//name
				newClient->name = (char*)malloc((partitionLen + 1) * sizeof(char));
				strcpy(newClient->name, partition);
				partitionLen = 0;
				partition = NULL;
			}
			else if(delimiterCounter == 2){//phone no
				newClient->phoneNo = (char*)malloc((partitionLen + 1) * sizeof(char));
				strcpy(newClient->phoneNo, partition);
				partitionLen = 0;
				partition = NULL;
			}
			/*else if(delimiterCounter == 3){//socket id
				newClient->socket = (int)*partition - '0';
				partitionLen = 0;
				partition = realloc(partition, partitionLen * sizeof(char));
			}*/
			else if(delimiterCounter == 3){//num of contact
				sscanf(partition, "%d", &newClient->numOfContact);
				partitionLen = 0;
				partition = NULL;
			}
			else if(delimiterCounter == 4){//contact's ids
				newClient->contactsId = malloc(newClient->numOfContact * sizeof(int));
				int j;
				int secondDeliminator = 0;
				int supPartitionLen = 0;
				char *supPartition = NULL;

				for(j = 0; j < partitionLen; j++){
					if(partition[j] == ','){
						sscanf(supPartition, "%d", &newClient->contactsId[secondDeliminator]);
						//newClient->contactsId[secondDeliminator] = (int)*supPartition - '0';
						secondDeliminator++;
						supPartitionLen = 0;
						supPartition = NULL;
					}
					else{
						supPartitionLen++;
						supPartition = (char*)realloc(supPartition, supPartitionLen * sizeof(char));
						supPartition[supPartitionLen-1] = partition[j];
					}
					
				}
				//free(supPartition);
				partitionLen = 0;
				partition = NULL;
			}
			delimiterCounter++;
		}
		else{
			partitionLen++;
			partition = realloc(partition, partitionLen * sizeof(char));
			partition[partitionLen-1] = line[i];
		}
	}
	//free(partition);
	return newClient;
}

void addToTheEndOfClientHead(struct Client* clientsHead, struct Client* client){
	
	struct Client* temp = clientsHead;
	while (temp->next != NULL)
	{
		temp = temp->next;
	}
	temp->next = client;
	client->next = NULL;
	
}
void saveClientToFile(struct Client* client){
	FILE *file;
	char ch;
	char *oldText;
	file = fopen("clients.txt", "a");
	if (file == NULL) {
        
        return;
    }
	fprintf(file, "%d;%s;%s;%d;", client->id, client->name, client->phoneNo, client->numOfContact);
	int i;
	for (i = 0; i < client->numOfContact; i++)
	{
		fprintf(file, "%d,", client->contactsId[i]);
	}
	//free(oldText);
	fprintf(file, ";\n");
	fclose(file);
}
void readClientsFromFile(struct Client* clientsHead, int *totalNumOfClients){
	FILE *file;
	char * line = NULL;
	char ch;
	size_t len;
	file = fopen("clients.txt", "r");
	if (file == NULL) {
        
        return;
    }
	int numOfClient = 0;
	while ((len = getline(&line, &len, file)) != -1) {
		struct Client *clientDeserialized = textDeserializer(line);
		numOfClient++;
		addToTheEndOfClientHead(clientsHead, clientDeserialized);
    }
	//free(line);
	fclose(file);
}
bool isSavedBefore(struct Client *client){
	char delimiter = ';';
	FILE *file;
	char * line = (char *)malloc(1024 * sizeof(char));
	char ch;
	size_t len = 0;
	ssize_t read;
	
	file = fopen("clients.txt", "r");
	if (file == NULL) {
        
        return false;
    }

	int partitionLen = 0;
	char *partition = (char *)malloc(partitionLen * sizeof(char));
	while ((read = getline(&line, &len, file)) != -1) {
		partitionLen = 0;
		partition = (char *)malloc(partitionLen * sizeof(char));
		while(line[partitionLen] != delimiter){
			partitionLen++;
			partition = realloc(partition, partitionLen * sizeof(char));
			partition[partitionLen-1] = line[partitionLen-1];
			int tmp;
			sscanf(partition, "%d", &tmp);
			if(client->id == tmp){
				//free(partition);
				//free(line);
				partition = NULL;
				return true;
			}
		}
    }
	//free(partition);
	//free(line);
	partition = NULL;
	fclose(file);
	return false;
}
void saveNewContactId(struct Client *client, int tempId){
	char delimiter = ';';
	FILE *file;
	FILE *fileNew;
	char * line = (char *)malloc(1024 * sizeof(char));
	char * tempLine = (char *)malloc(1024 * sizeof(char));
	char ch;
	size_t len;
	size_t oldLen;
	file = fopen("clients.txt", "r");
	if (file == NULL) {
        
        return;
    }
	fileNew = fopen("clientsNew.txt", "a");
	if (fileNew == NULL) {
        
        return;
    }
	int partitionLen = 0;
	char *partition = (char *)malloc(partitionLen * sizeof(char));; // to compare ID
	int lineIndex = 0;
	bool check = false;
	int indexToAdd = 0;
	int checkLine = -1;
	while ((len = getline(&line, &len, file)) != -1) {
		lineIndex++;
		partitionLen = 0;
		//free(partition);
		partition = (char *)malloc(partitionLen * sizeof(char));
		if(!check){
			while(line[partitionLen] != delimiter){
				partitionLen++;
				partition = realloc(partition, partitionLen * sizeof(char));
				partition[partitionLen-1] = line[partitionLen-1];
				int tmpClientId;
				sscanf(partition, "%d", &tmpClientId);
				if(client->id == tmpClientId){
					indexToAdd = len-2; 
					checkLine = lineIndex;
					check = true; 
				}
			}
		}
		
		if(!check || (checkLine != lineIndex) ){
			fprintf(fileNew, "%s", line);
		}
		else{
			int extraDigits = 0;
			int m;
			for (m = 0; m < client->numOfContact-1; m++)
			{
				extraDigits = extraDigits + floor(log10(client->contactsId[m]) + 1) - 1;  
			}

			int extraDigitForNumOfContact = 1;
			if(client->numOfContact != 1){
				extraDigitForNumOfContact = floor(log10(client->numOfContact-1) + 1) ;
			}
			int i;
			for(i = 0; i < indexToAdd-1-((client->numOfContact-1)*2)-extraDigits-extraDigitForNumOfContact; i++){
				fprintf(fileNew, "%c", line[i]);
			}
			fprintf(fileNew, "%d;", client->numOfContact);
			int k;
			for (k = 0; k < client->numOfContact-1; k++)
			{
				fprintf(fileNew, "%d,", client->contactsId[k]);
			}
			fprintf(fileNew, "%d,;\n", tempId);
		}
    }
	rename("clientsNew.txt", "clients.txt");
	fclose(fileNew);
	fclose(file);

}
bool deleteFromContactWithId(struct Client *clientsHead, struct Client *client, int tempId){
	struct Client *temp = clientsHead;
	if(temp == NULL){
		return false;
	}
	if(temp->next == NULL){
		return false;
	}
	bool isFind = false;
	while (temp->next != NULL)
	{
		if(temp->next->id == client->id){
			temp = temp->next;
			isFind = true;
			break;
		}
		temp = temp->next;
	}

	int i;
	for (i = 0; i < temp->numOfContact; i++)
	{
		if(temp->contactsId[i] == tempId){
			int j;
			for (j = i; j < temp->numOfContact-1; j++)
			{
				int exTemp = temp->contactsId[j];
				temp->contactsId[j] = temp->contactsId[j+1];
				temp->contactsId[j+1] = exTemp;
			}
			temp->numOfContact--;
			temp->contactsId = (int*)realloc(temp->contactsId, sizeof(int)*temp->numOfContact);
			return true;
		}
	}
	return false;	
}
void truncateAndSaveAllClientsToFile(struct Client* clientsHead){
	struct Client* temp = clientsHead;
	
	if(temp == NULL) return;
	if(temp->next == NULL) return;

	temp = temp->next;
	FILE *file;
	char ch;
	char *oldText;
	file = fopen("TruncateAndSetNewClients.txt", "w");
	if (file == NULL) {
        
        return;
    }
	while (temp != NULL)
	{
		fprintf(file, "%d;%s;%s;%d;", temp->id, temp->name, temp->phoneNo, temp->numOfContact);
		int i;
		for (i = 0; i < temp->numOfContact; i++)
		{
			fprintf(file, "%d,", temp->contactsId[i]);
		}
		fprintf(file, ";\n");
		temp = temp->next;
	}
	
	rename("TruncateAndSetNewClients.txt", "clients.txt");
	//free(oldText);
	fclose(file);
}

void addToTheEndOfMessHead(struct ArchiveMessage* messHead, struct ArchiveMessage* mess){
	struct ArchiveMessage* temp = messHead;
	while (temp->next != NULL)
	{
		temp = temp->next;
	}
	temp->next = mess;
	mess->next = NULL;
	
}
void saveMessageToFileWithFileName(char* message, int recieverId, int senderId, char* filename){
	FILE *file;
	char ch;
	char *oldText;
	file = fopen(filename, "a");
	if (file == NULL) {
        
        return;
    }
	int isReaded = 0;
	fprintf(file, "%d;%d;%d;%s", recieverId, senderId, isReaded, message);
	
	//free(oldText);
	fprintf(file, ";\n");
	fclose(file);
}
void saveMessageToFile(char* message, int recieverId, int senderId){
	FILE *file;
	char ch;
	char *oldText;
	file = fopen("messages.txt", "a");
	if (file == NULL) {
        
        return;
    }
	int isReaded = 0;
	fprintf(file, "%d;%d;%d;%s", recieverId, senderId, isReaded, message);
	
	//free(oldText);
	fprintf(file, ";\n");
	fclose(file);
}
struct ArchiveMessage* readMessagesFromFileById(int clientId){
	char delimiter = ';';
	int delimiterCounter = 0;
	FILE *file;
	char * line = NULL;
	char ch;
	size_t len;
	struct ArchiveMessage* archiveMessHead = (struct ArchiveMessage*)malloc(sizeof(struct ArchiveMessage));
	archiveMessHead->next = NULL;

	int counter = 1;
	int partitionLen = 0;
	char *partition = malloc(sizeof(char));

	file = fopen("messages.txt", "r");
	if (file == NULL) {
        
        return archiveMessHead;
    }
	int numOfClient = 0;

	while ((len = getline(&line, &len, file)) != -1) {
		struct ArchiveMessage* messTemp = (struct ArchiveMessage*)malloc(sizeof(struct ArchiveMessage));
		int i;
		delimiterCounter = 0;
		bool isNew = false;
		for (i = 0; i < strlen(line); i++){
			if(line[i] == delimiter){
				if(delimiterCounter == 0){ //recieverId
					int tmpPartition;
					sscanf(partition, "%d", &tmpPartition);
					if(tmpPartition != clientId){
						partitionLen = 0;
						partition = NULL;
						break;
					}
					isNew = true;
					//messTemp->next = (struct ArchiveMessage*)malloc(sizeof(struct ArchiveMessage));
					sscanf(partition, "%d", &messTemp->toId);
					partitionLen = 0;
					partition = NULL;
				}
				else if(delimiterCounter == 1){ //senderId
					sscanf(partition, "%d", &messTemp->fromId);
					partitionLen = 0;
					partition = NULL;
				}
				else if(delimiterCounter == 2){ //isReaded
					sscanf(partition, "%d", &messTemp->isReaded);
					partitionLen = 0;
					partition = NULL;
				}
				else if(delimiterCounter == 3){ //text
					//messTemp->text = (char*)malloc((partitionLen + 1) * sizeof(char));
					//strcpy(messTemp->text, partition);
					messTemp->text = strdup(partition);
					partitionLen = 0;
					partition = NULL;
				}
				delimiterCounter++;
			}
			else{
				partitionLen++;
				partition = realloc(partition, partitionLen * sizeof(char));
				partition[partitionLen-1] = line[i];
			}
		}
		if(isNew){
			// mesajı gönder
			addToTheEndOfMessHead(archiveMessHead, messTemp);
		}
    }
	//free(line);
	fclose(file);
	return archiveMessHead;
}
void truncateAndSaveAllMessagesToFile(struct ArchiveMessage* archiveMess, struct Client *clientsHead){
	struct ArchiveMessage* temp = archiveMess;
	struct Client *tempClient = clientsHead;
	
	if(temp == NULL) return;
	if(temp->next == NULL) return;

	temp = temp->next;
	FILE *file;
	FILE *fileOld;
	char ch;
	char *oldText;
	file = fopen("TruncateAndSetNewMessages.txt", "w");
	if (file == NULL) {
        
        return;
    }
	while (temp != NULL)
	{
		fprintf(file, "%d;%d;%d;%s", temp->toId, temp->fromId, temp->isReaded, temp->text);
		fprintf(file, ";\n");
		temp = temp->next;
	}
	fclose(file);

	if(tempClient->next != NULL && tempClient != NULL){
		while (tempClient->next != NULL)
		{
			tempClient = tempClient->next;
			if(tempClient->id == archiveMess->next->toId) continue;
			struct ArchiveMessage* messForNewHead = readMessagesFromFileById(tempClient->id);
			struct ArchiveMessage* tempForNewArcMes = messForNewHead;
			if(tempForNewArcMes==NULL) continue;
			while (tempForNewArcMes->next != NULL)
			{
				tempForNewArcMes = tempForNewArcMes->next;
				saveMessageToFileWithFileName(tempForNewArcMes->text, tempForNewArcMes->toId, tempForNewArcMes->fromId, "TruncateAndSetNewMessages.txt");
			}
		}
	}
	rename("TruncateAndSetNewMessages.txt", "messages.txt");
	//free(oldText);
	//fclose(file);
}

struct Client* getClientWithName(struct Client* ClientsHead , char* name){
	struct Client *temp = ClientsHead;
	while (temp->next != NULL)
	{
		if(strcasecmp(temp->next->name, name) == 0){
			return temp->next;
		}
		temp = temp->next;
	}
	return NULL;
}
struct Client* getClientWithId(struct Client* ClientsHead , int id){
	struct Client *temp = ClientsHead;
	if(id==0){
		return NULL;
	}
	if(temp == NULL){
		return NULL;
	}
	while (temp->next != NULL)
	{
		if(temp->next->id == id){
			return temp->next;
		}
		temp = temp->next;
	}
	return NULL;
}
void setName(struct Client* Client, char* name){
	Client->name = (char*)malloc((strlen(name) + 1) * sizeof(char));
	strcpy(Client->name, name);
}
void setPhoneNo(struct Client* Client, char* phoneNo){
	Client->phoneNo = (char*)malloc((strlen(phoneNo) + 1) * sizeof(char));
	strcpy(Client->phoneNo, phoneNo);
}
struct Client* addToClients(struct Client* ClientsHead, int id, int* totalNumClients, int newSocketId){
	struct Client* temp = ClientsHead;
	struct Client* newClient = (struct Client*)malloc(sizeof(struct Client));
	*totalNumClients = *totalNumClients+ 1;
	newClient->id = id;
	newClient->next = NULL;
	newClient->socket = newSocketId;
	newClient->isOnline = 0;
	newClient->numOfContact = 0;
	newClient->contactsId = (int*)malloc(sizeof(int)*newClient->numOfContact);
	while (temp->next != NULL)
	{
		temp = temp->next;
	}
	temp->next = newClient;
	return newClient;
}
struct Client* getOrAddClient(struct Client* ClientsHead , int id, int* totalNumClients, int newSocketId){
	struct Client *res = getClientWithId(ClientsHead, id);
	if(res == NULL){
		struct Client *newRes = addToClients(ClientsHead, id, totalNumClients, newSocketId);
		return newRes;
	}
	return res;
}
void setNameAndPhoneNo(struct Client *client){
	char buffer[MAX_BUFFER_SIZE];
	if(client->name == NULL && client->phoneNo == NULL){
			char* response = "00";
			send(client->socket, response, strlen(response), 0);

			ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}
			size_t length = strlen(buffer);
			char* name = (char*)malloc((length + 1) * sizeof(char));
			strcpy(name, buffer);
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			setName(client, name);

			dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}
			length = strlen(buffer);
			char* PhoneNo = (char*)malloc((length + 1) * sizeof(char));
			strcpy(PhoneNo, buffer);
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			setPhoneNo(client, PhoneNo);
	}
	else if(client->name == NULL){
			char* response = "0";
			send(client->socket, response, strlen(response), 0);
			ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}
			size_t length = strlen(buffer);
			char* name = (char*)malloc((length + 1) * sizeof(char));
			strcpy(name, buffer);
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			setName(client, name);
	}
	else if(client->phoneNo == NULL){
			char* response = "000";
			send(client->socket, response, strlen(response), 0);
			ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}
			size_t length = strlen(buffer);
			char* PhoneNo = (char*)malloc((length + 1) * sizeof(char));
			strcpy(PhoneNo, buffer);
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			setPhoneNo(client, PhoneNo);
	}
	else{
		char* response = "1";
		send(client->socket, response, strlen(response), 0);
	}
}
bool addToContactWithId(struct Client *clientsHead, struct Client *client, int id){
	if(getClientWithId(clientsHead, id)==NULL){
		return false;
	}
	int i;
	for (i = 0; i < client->numOfContact; i++)
	{
		if(client->contactsId[i] == id){
			return false;
		}
	}
	
	int lastIndex = client->numOfContact++;
	client->contactsId = (int*)realloc(client->contactsId, client->numOfContact * sizeof(int));
	client->contactsId[lastIndex] = id;
	return true;
}
bool isClientInContact(struct Client *client, int receiverId){
	int i;
	for (i = 0; i < client->numOfContact; i++)
	{
		if(client->contactsId[i] == receiverId){
			return true;
		}
	}
	return false;	
}
bool IsAlreadyOnline(struct Client* res){
	if(res->isOnline == 0){
			res->isOnline = 1;
			return false;
		}
		else{
			return true;
		}
}
bool IsValidId(int id){
	if(id > 0){
			return true;
	}
	else{
		return false;
	}
}
void splitString(char *buffer, char **recieverId, char **message) {
    char delimiter = ' ';
	bool isFirstPart = true;
	size_t idLen = 0;
	size_t messageLen = 0;
	*recieverId = (char*)malloc(idLen * sizeof(char));
    *message = (char*)malloc(messageLen * sizeof(char));
	int i;
	for (i = 0; i < strlen(buffer); i++)
	{
		if(buffer[i] == delimiter){
			if(isFirstPart){
				isFirstPart = false;
				continue;
			}
			isFirstPart = false;
		}
		if(isFirstPart){
			idLen++;
			*recieverId = (char*)realloc(*recieverId, idLen * sizeof(char));
			(*recieverId)[idLen - 1] = buffer[i];
		}
		else{
			messageLen++;
			*message = (char*)realloc(*message, messageLen * sizeof(char));
			(*message)[messageLen - 1] = buffer[i];
		}
	}
	//*message = (char*)realloc(*message, messageLen * sizeof(char));
	//(*recieverId)[idLen - 1] = '\0';
    //(*message)[messageLen -1] = '\0';
}
void sendInformationResponse(struct Client *client, char *resMessage){
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE); 

	struct Message* sendObj = (struct Message*)malloc(sizeof(struct Message));
	sendObj->isResponse = true;
	sendObj->isNewMessage = false;
	sendObj->isReaded = false;
	send(client->socket, sendObj, sizeof(struct Message), 0);
	ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
	if (dataLen <= 0) {
		printf("Client %d disconnected\n", client->id);
		client->isOnline = 0;
		close(client->socket);
		//free(sendObj);
		pthread_exit(NULL);
	}  
	memset(buffer, 0, MAX_BUFFER_SIZE); 
	strcat(buffer, "\t\t");
	strcat(buffer, resMessage);

	send(client->socket, buffer, strlen(buffer), 0);

	//free(sendObj);
	memset(buffer, 0, MAX_BUFFER_SIZE); 
}

void *clientPage(void *arg) {
    struct User *user = (struct User*)arg;
	struct Client *client = (user)->Client;
	struct Client *clientsHead = (user)->ClientsHead;
    char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	setNameAndPhoneNo(client);

	if(!isSavedBefore(client)){
		saveClientToFile(client); 
	}
    
	while (1) {
        // Receive message from client
        ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
        if (dataLen <= 0) {
            printf("Client %d disconnected\n", client->id);
			client->isOnline = 0;
            close(client->socket);
            pthread_exit(NULL);
        }

		if((ssize_t)strcmp(&buffer[0],"1")==10){ // list contact
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			int i;
			if(client->numOfContact == 0){
				sendInformationResponse(client, "Empty contact list!");
				continue;
			}
			else{
				char tempBuff[MAX_BUFFER_SIZE];
				struct Message* sendObj = (struct Message*)malloc(sizeof(struct Message));
				sendObj->isResponse = true;
				send(client->socket, sendObj, sizeof(struct Message), 0);
				ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
				if (dataLen <= 0) {
					printf("Client %d disconnected\n", client->id);
					client->isOnline = 0;
					close(client->socket);
					pthread_exit(NULL);
				} 
				memset(buffer, 0, MAX_BUFFER_SIZE); 
				pthread_mutex_lock(&clientsHeadMutex);
				int totalNumOfClients;
				for (i = 0; i < client->numOfContact; i++)
				{
					struct Client *tmp = getClientWithId(clientsHead, client->contactsId[i]);
					if(tmp != NULL){
						sprintf(tempBuff, "\t\tClient Id: %d, Client Name: %s, Client Phone: %s\n", tmp->id, tmp->name, tmp->phoneNo);
					}
					
					strcat(buffer, tempBuff);
					memset(tempBuff, 0, MAX_BUFFER_SIZE); 
				}
				pthread_mutex_unlock(&clientsHeadMutex);
				send(client->socket, buffer, strlen(buffer), 0);
				memset(buffer, 0, MAX_BUFFER_SIZE); 
			}
			
		}
		else if((ssize_t)strcmp(&buffer[0],"2")==10){ // add user
			memset(buffer, 0, MAX_BUFFER_SIZE);  
			ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}  
			int tempId;
			sscanf(buffer, "%d", &tempId);
			if(!IsValidId(tempId)){
				sendInformationResponse(client, "User Id is not valid!");
				continue;
			}
			pthread_mutex_lock(&clientsHeadMutex);
			int totalNumOfClients;
			bool isAdded = addToContactWithId(clientsHead, client, tempId);
			pthread_mutex_unlock(&clientsHeadMutex);
			if(!isAdded){
				sendInformationResponse(client, "Not valid user ID to add your contact!");
			}
			else{
				sendInformationResponse(client, "User added!");

				pthread_mutex_lock(&contactMutex);
				saveNewContactId(client, tempId);
				pthread_mutex_unlock(&contactMutex);
			}
			memset(buffer, 0, MAX_BUFFER_SIZE);  
		}
		else if((ssize_t)strcmp(&buffer[0],"3")==10){ // delete user
			memset(buffer, 0, MAX_BUFFER_SIZE);  
			ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}  
			int tempId;
			sscanf(buffer, "%d", &tempId);
			if(!IsValidId(tempId)){
				sendInformationResponse(client, "User Id is not valid to delete!");
				continue;
			}
			pthread_mutex_lock(&clientsHeadMutex);
			int totalNumOfClients;
			bool isDeleted = deleteFromContactWithId(clientsHead, client, tempId);
			pthread_mutex_unlock(&clientsHeadMutex);
			if(!isDeleted){
				sendInformationResponse(client, "Not valid user ID to delete your contact!");
			}
			else{
				sendInformationResponse(client, "User deleted!");

				pthread_mutex_lock(&contactMutex);
				truncateAndSaveAllClientsToFile(clientsHead);
				pthread_mutex_unlock(&contactMutex);
			}
			memset(buffer, 0, MAX_BUFFER_SIZE);  
		}
		else if((size_t)strcmp(&buffer[0],"4")==10){ // send message
			memset(buffer, 0, MAX_BUFFER_SIZE); 
			ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
			if (dataLen <= 0) {
				printf("Client %d disconnected\n", client->id);
				client->isOnline = 0;
				close(client->socket);
				pthread_exit(NULL);
			}  
			
			char *sendIdChar;
			char *message;
			splitString(buffer, &sendIdChar, &message);
			int recieverId;
			sscanf(sendIdChar, "%d", &recieverId);
			if(recieverId <= 0){
				sendInformationResponse(client, "Invalid Receiver ID! Please enter a valid ID!");
				continue;
			}
			if(isClientInContact(client, recieverId)){
				pthread_mutex_lock(&clientsHeadMutex);
				int totalNumOfClients;
				struct Client *clientReciever = getClientWithId(clientsHead, recieverId);
				pthread_mutex_unlock(&clientsHeadMutex);

				struct Message* sendObj = (struct Message*)malloc(sizeof(struct Message));
				sendObj->isNewMessage = true;
				sendObj->senderId = client->id;
				sendObj->isReaded = false;
				send(clientReciever->socket, sendObj, sizeof(struct Message), 0);
				memset(buffer, 0, MAX_BUFFER_SIZE); 
				send(clientReciever->socket, message, strlen(message), 0);
				pthread_mutex_lock(&messageArchiveMutex);
				saveMessageToFile(message, recieverId, client->id);
				pthread_mutex_unlock(&messageArchiveMutex);
				sendInformationResponse(client, "Message send!");
			}
			else{
				sendInformationResponse(client, "User is not in your contact list!");
			}
			memset(buffer, 0, MAX_BUFFER_SIZE);  
		}	
		else if((size_t)strcmp(&buffer[0],"5")==10){ // Check Messages
			memset(buffer, 0, MAX_BUFFER_SIZE);  
			
			pthread_mutex_lock(&messageArchiveMutex);
			int totalNumOfClients;
			struct ArchiveMessage *archiveMess = readMessagesFromFileById(client->id);
			struct ArchiveMessage * archiveMessTemp = archiveMess;
			struct ArchiveMessage * archiveMessTempBefore = archiveMess;
			pthread_mutex_unlock(&messageArchiveMutex);
			
			if(archiveMess != NULL && archiveMess->next != NULL){
				struct Message* sendObj = (struct Message*)malloc(sizeof(struct Message));
				sendObj->isNewMessageCount = true;
				send(client->socket, sendObj, sizeof(struct Message), 0);
				ssize_t dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
				char tempBuff[MAX_BUFFER_SIZE];
				memset(buffer, 0, MAX_BUFFER_SIZE);
				
				while (archiveMessTemp->next != NULL){
					archiveMessTemp = archiveMessTemp->next;
					sprintf(tempBuff + strlen(tempBuff), "\t\tYou Have Message From Client %d", archiveMessTemp->fromId);
					if(archiveMessTemp->isReaded == 0){
						sprintf(tempBuff + strlen(tempBuff), " (*)");
					}
					sprintf(tempBuff+strlen(tempBuff) , "\n");
				}
				send(client->socket, tempBuff, strlen(tempBuff), 0);
				memset(tempBuff, 0, MAX_BUFFER_SIZE); 
				memset(buffer, 0, MAX_BUFFER_SIZE);

				archiveMessTemp = archiveMess;

				dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0); // to show the text of messages with senderId
				if (dataLen <= 0) {
					printf("Client %d disconnected\n", client->id);
					client->isOnline = 0;
					close(client->socket);
					pthread_exit(NULL);
				}  

				int tempId;
				sscanf(buffer, "%d", &tempId);
				memset(buffer, 0, MAX_BUFFER_SIZE); 
				bool isValid = IsValidId(tempId);
				if(!isValid){
					sendInformationResponse(client, "Not a valid Id to read message(s)!");
					memset(tempBuff, 0, MAX_BUFFER_SIZE); 
					memset(buffer, 0, MAX_BUFFER_SIZE);
					continue;
				}
				
				bool atLeastOneMessageAvailable = false;

				while (archiveMessTemp->next != NULL){
					archiveMessTemp = archiveMessTemp->next;
					if(archiveMessTemp->fromId != tempId){
						continue;
					}
					archiveMessTemp->isReaded = 1;
					atLeastOneMessageAvailable = true;
					sprintf(tempBuff + strlen(tempBuff), "\t\tMessage From: %d, Message: %s\n", archiveMessTemp->fromId, archiveMessTemp->text);
				}
				if(atLeastOneMessageAvailable){
					pthread_mutex_lock(&messageArchiveMutex);
					truncateAndSaveAllMessagesToFile(archiveMess, clientsHead);
					pthread_mutex_unlock(&messageArchiveMutex);

					sendObj->isNewMessageCount = false;
					sendObj->isNewMessageText = true;
					send(client->socket, sendObj, sizeof(struct Message), 0);
					memset(buffer, 0, MAX_BUFFER_SIZE);

					dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0);
					memset(buffer, 0, MAX_BUFFER_SIZE);

					send(client->socket, tempBuff, strlen(tempBuff), 0);
					memset(tempBuff, 0, MAX_BUFFER_SIZE); 
					memset(buffer, 0, MAX_BUFFER_SIZE);
				}
				else{
					sendInformationResponse(client, "Invalid Id to see messages!");
					memset(tempBuff, 0, MAX_BUFFER_SIZE); 
					memset(buffer, 0, MAX_BUFFER_SIZE);
					continue;
				}

				archiveMessTemp = archiveMess;

				dataLen = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0); // to delete message with lineIndex
				if (dataLen <= 0) {
					printf("Client %d disconnected\n", client->id);
					client->isOnline = 0;
					close(client->socket);
					pthread_exit(NULL);
				}  
				int lineToDelete;
				sscanf(buffer, "%d", &lineToDelete);
				memset(buffer, 0, MAX_BUFFER_SIZE); 
				if(lineToDelete <= 0){
					memset(tempBuff, 0, MAX_BUFFER_SIZE); 
					memset(buffer, 0, MAX_BUFFER_SIZE);
					continue;
				}
				int counter = 0;
				while (archiveMessTemp->next != NULL){
					archiveMessTempBefore = archiveMessTemp;
					archiveMessTemp = archiveMessTemp->next;
					if(archiveMessTemp->fromId != tempId){
						continue;
					}
					counter++;
					if(counter == lineToDelete){
						if(archiveMessTemp->next != NULL){
							archiveMessTempBefore->next = archiveMessTemp->next;
						}
						else{
							archiveMessTempBefore->next = NULL;
						}
						
					}
				}		
				pthread_mutex_lock(&messageArchiveMutex);
				truncateAndSaveAllMessagesToFile(archiveMess, clientsHead);
				pthread_mutex_unlock(&messageArchiveMutex);	
				
			}
			else{
				sendInformationResponse(client, "No messages to check!");
			}
		}
        memset(buffer, 0, MAX_BUFFER_SIZE); 
    }
}

void setConnection(int *serverFd, int *newSocket, struct sockaddr_in address, int *opt){
	if ((*serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(*serverFd, SOL_SOCKET, SO_REUSEADDR , opt, sizeof(*opt))){
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(*serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(*serverFd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	printf("Server listening on port %d...\n", PORT);
}

int main(int argc, char const* argv[])
{
	int totalNumClients = 0;
	int serverFd, newSocket;
	ssize_t valread;
	struct sockaddr_in address;
	int opt = 1;
	socklen_t addrlen = sizeof(address);
	char buffer[MAX_BUFFER_SIZE] = { 0 };

	setConnection(&serverFd, &newSocket, address, &opt);

	pthread_t *threads = NULL;
	struct Client *clientsHead = (struct Client*)malloc(sizeof(struct Client));
	clientsHead->next = NULL;

	struct User *users = NULL;
	int userCounter = 0;
	
	readClientsFromFile(clientsHead, &totalNumClients);
	
	struct Client* res = (struct Client*)malloc(sizeof(struct Client));
	signal(SIGINT, ctrlCHandler);
	int newSocketId;
	while (!ctrlCReceived) {
		newSocketId = 0;

        if ((newSocketId = accept(serverFd, (struct sockaddr *)&address, &addrlen)) == -1) {
            continue;
        }
		ssize_t dataLen = recv(newSocketId, buffer, MAX_BUFFER_SIZE, 0);
		int tempId;
		sscanf(buffer, "%d", &tempId);
		memset(buffer, 0, MAX_BUFFER_SIZE); 
		bool isValid = IsValidId(tempId);

		pthread_mutex_lock(&clientsHeadMutex);
		res = getOrAddClient(clientsHead, tempId, &totalNumClients, newSocketId);
        pthread_mutex_unlock(&clientsHeadMutex);
		bool isAlreadyOnline = IsAlreadyOnline(res);
		if(!isAlreadyOnline && isValid){
			char* response = "1";
			res->socket = newSocketId;
			send(res->socket, response, strlen(response), 0);
		}
		else{
			char* response_fail = "0";
			send(newSocketId, response_fail, strlen(response_fail), 0);
			continue;
		}

		users = (struct User*)realloc(users, ++userCounter * sizeof(struct User));
		users[userCounter-1].Client = (struct Client*)malloc(sizeof(struct Client));
		users[userCounter-1].Client = res;
		users[userCounter-1].ClientsHead = (struct Client*)malloc(sizeof(struct Client) * totalNumClients);
		users[userCounter-1].ClientsHead = clientsHead;
		threads = (pthread_t *)realloc(threads, totalNumClients*sizeof(pthread_t));

        if (pthread_create(&threads[totalNumClients-1], NULL, clientPage, &users[userCounter-1]) != 0) {
            close(getClientWithName(clientsHead, res->name)->socket);
            continue;
        }
    }
	
	
	int i;
    for (i = 0; i < totalNumClients; i++) {
        pthread_detach(threads[i]);
    }
	close(newSocket);
	close(serverFd);
	free(users);
	free(threads);
	free(clientsHead);
	free(res);
	return 0;
}
