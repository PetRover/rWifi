//
// Created by Bryce Carter on 8/25/15.
//
#include "rWifi.h"

#define RECEIVE_HEADER_LENGTH    2
#define INTEGER                  6            //TODO - make these desired values not just convenient ones for testing
#define CHARACTER                7


int receiveHeaderValue[RECEIVE_HEADER_LENGTH-1] = {52};          //minus 1 becuase last byte is information carrying byte

namespace RVR
{
// ==============================================================
// Connection NetworkManager Member functions
// ==============================================================
    void NetworkManager::initializeNewConnection(std::string connectionName, const char* ipAddress)
    {
        Connection* connectionPtr = new Connection;

        connectionPtr->initializeNewSocket(connectionName, ipAddress);
        this->existingConnections.push_back(connectionPtr);//add the connection pointer to list of connection pointers

        connectionPtr->initiateConnection();
        return;
    }

    void NetworkManager::terminateConnection(std::string connectionName)
    {
        Connection* connectionPtr = getConnectionPtrByConnectionName(connectionName);
        int connectionPosition = getPositionByConnectionName(connectionName);
        connectionPtr->terminateConnection();
        existingConnections.erase (existingConnections.begin()+connectionPosition);

        return;
    }

    void NetworkManager::sendData(std::string connectionName, NetworkChunk *chunk) //TODO - determine type instead of void*
    {
        Connection* connectionPtr = getConnectionPtrByConnectionName(connectionName);
        connectionPtr->sendData(chunk);

        return;
        //TODO - check that the number of bytes sent is equal to the number of bytes i expect (length)
    }

    int NetworkManager::getData(std::string connectionName, NetworkChunk *chunk)
    //Received data will be stored in the buffer which is passed to us via the pointer chunk. Return value of 1 indicates
    //there was data placed in this chunk. Return value of 0 indicates no data was available to fill this chunk.
    {
        Connection* connectionPtr = getConnectionPtrByConnectionName(connectionName);
        int dataReceived = connectionPtr->processData(chunk);

        printf("length = %d (i.e. was correct value passed back to getData function)\n",chunk->numberBytes);

        return dataReceived;
    }

    Connection* NetworkManager::getConnectionPtrByConnectionName(std::string connectionNameToFind)
    //iterate through existingConnections list to find the connection ptr associated with the given name
    {
        for (std::vector<Connection*>::iterator it = this->existingConnections.begin() ; it != this->existingConnections.end(); ++it)
        {
            if ((*it)->connectionName == connectionNameToFind)
            {
                return *it;
            }
        }

        //TODO -implement error if connectionName not found
        return nullptr;
    }

    int NetworkManager::getPositionByConnectionName(std::string connectionNameToFind)
    //Used for deleting connection
    {
        int count = 0; //TODO - fix this dumb solution
        for (std::vector<Connection*>::iterator it = this->existingConnections.begin() ; it != this->existingConnections.end(); ++it)
        {
            if ((*it)->connectionName == connectionNameToFind)
            {
                return count;
            }
            count++;
        }

        //TODO -implement error if connectionName not found
        return 0;
    }
// ==============================================================
// Connection Class Member functions
// ==============================================================

    void Connection::initializeNewSocket(std::string connectionName, const char* ipAddress)
    {
        this->ipAddress = ipAddress;
        this->connectionName = connectionName;
        this->fileDescriptor = this->createEndpoint();

        if (fileDescriptor == -1)
        {
            printf("Failed to initialize new socket\n");
        }else{
            std::string errorMessage = "\nConnection name: " + this->connectionName + "\nIP Address: " + this->ipAddress + "\n\n";
            printf("Successfully initiated new socket!\nFileDescriptor: %d %s", this->fileDescriptor, errorMessage.c_str());
        }
        return;
    }

    int Connection::createEndpoint()
    //Socket creates endpoint for communication. Parameters determine following features: Domain of AF_INET indicates IPv4,
    //Type of SOCK_STREAM indicates sequenced, reliable, two-way, connection- based byte streams. 0 for protocol parameter
    //indicates using the default protocol for the domain specified will be used
    //On success, a file descriptor for the new socket is returned.  On error, -1 is returned
    {
        int fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
        return fileDescriptor;
    }

    void Connection::initiateConnection()
    //Connects to a socket. Input parameter is the IP address to be formatted.
    //Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned
    {
        struct sockaddr_in socketAddress;
        socketAddress.sin_addr.s_addr = inet_addr(this->ipAddress);
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_port = htons(1024);

        printf("Attempting to connect...\n");
        int successStatus = connect(this->fileDescriptor, (struct sockaddr *) &socketAddress, sizeof(socketAddress));
        if (successStatus == -1){
            printf("Failed to initiate connection\n");
        }else{
            printf("Sucessfully initiated connection\n");
        }
        return;
    }

    int Connection::terminateConnection()
    //Causes connection on the socket associated with first input parameter to be shut down. Further receptions and transmissions will be disallowed.
    //Second input parameter is defaulted to SHUT_RDWR to stop all further receptions and transmissions instead of just receptions or just transmissions
    //On success, zero is returned.  On error, -1 is returned
    {
        int successStatus = shutdown(this->fileDescriptor, SHUT_RDWR);
        return successStatus;
    }

    int Connection::sendData(NetworkChunk *chunk)
    //Upon successful completion, send() shall return the number of bytes sent. Otherwise, -1 shall be returned
    //The first input parameter is a pointer to the buffer where the message to be transmitted is stored.
    //The second input parameter is the message length.
    {
        ssize_t bytesSent = send(this->fileDescriptor, chunk->payload, chunk->numberBytes, 0);
        printf("Sent %d bytes\n", bytesSent);
        return bytesSent;
    }

    int Connection::processData(NetworkChunk *oldestChunk)
    //function takes everything that is on the buffer and puts it in a queue of NetworkChunk*'s. It then returns the oldest
    //to be processed
    {
        int bytesReceived;

        //TODO - make sure it can't hang here for too long if there's an incredible amount of stuff on buffer
        do{//while there is data to be received in the buffer
            NetworkChunk* receivedChunk = new NetworkChunk; //create a chunk
            bytesReceived = this->receiveDataFromBuffer(receivedChunk);//try to fill it from the buffer
            printf("Data is being received off buffer...\n");

            if(bytesReceived > 0){ //if something was received, store it into the queue
                printf("Data received off buffer and being pushed into queue\n");
                this->chunkQueue.push (receivedChunk);
                printf("length = %d (i.e. was correct number passed back to processData function and pushed into queue)\n", receivedChunk->numberBytes);
            }
            else {//buffer was empty - nothing was received
                printf("No data received off buffer to push into queue\n");
                bytesReceived = 0;
            }
        }while(bytesReceived > 0);

        if (!this->chunkQueue.empty())//if there's something in the queue (it's not empty)
        {
            printf("Data in queue, popping data to process\n");
            oldestChunk = this->chunkQueue.front();    //access the oldest chunk in the queue
            printf("length = %d (i.e. correct number popped off queue)\n",oldestChunk->numberBytes);
            this->chunkQueue.pop();//delete the oldest chunk from the queue
            return 1; //indicating data has taken off queue and returned
        } else{
            printf("No data in queue to pop\n");
            return 0; //indicating queue was empty and no data was returned
        }
    }

    int Connection::receiveDataFromBuffer(NetworkChunk *chunk)
    //Upon successful completion, recv() shall return the length of the message in bytes. If no messages are available to be
    //received and the peer has performed an orderly shutdown, recv() shall return 0. Otherwise, -1 shall be returned to indicate error.
    //Last input argument for recv = 0 to indicate no flags
    {
        char header[RECEIVE_HEADER_LENGTH];
        int length;
        int dataType;

        //Receive starter byte
        int starterBytesReceived = recv(this->fileDescriptor, header, RECEIVE_HEADER_LENGTH, 0);

        if (starterBytesReceived == RECEIVE_HEADER_LENGTH){ //if correct number of bytes for header were received
            //check that the header we received = header expected
            printf("Header received. Received %d bytes\n", starterBytesReceived);
            int correctHeader = 1;
            for (int i=0;i<(RECEIVE_HEADER_LENGTH-1);i++)//minus 1 because last byte is the one carrying lenght/type info
            {
                printf("Header byte: %d\n", header[i]);
                if(header[i] != receiveHeaderValue[i])
                {
                    printf("Incorrect header byte\n");
                    correctHeader = 0;
                }
            }
            if (correctHeader){
                printf("Received the correct header!\n");
                printf("Datatype/length byte is: %d\n", header[RECEIVE_HEADER_LENGTH-1]);
                dataType = header[RECEIVE_HEADER_LENGTH-1] >> 4; //data type info stored in first half of info carrying byte
                length = header[RECEIVE_HEADER_LENGTH-1] & 0x0f; //length info stored in second half of info carrying byte
                printf("data type indicated is: %d\n", dataType);
                printf("Length = %d (i.e. length indicated by Datatype/length byte)\n", length);

                //receive data of indicated length
                //must be if statements, not switch/case because of scoping issues
                if (dataType == INTEGER){
//                    printf("Data Type = integer\n");
                    int receiveBuffer[length];
                    int bytesReceived = recv(this->fileDescriptor, receiveBuffer, length, 0);
                    printf("length = %d (i.e. actual number received)\n", bytesReceived);

                    chunk->numberBytes = bytesReceived;
                    chunk->dataTypeIndetifier = dataType;
                    chunk->payload = receiveBuffer;

                    printf("length = %d (i.e. was correct number stored?)\n", chunk->numberBytes);
                    return bytesReceived;
                }
                else if(dataType == CHARACTER){
                    printf("Data Type = character\n");
                    char receiveBuffer[length];
                    int bytesReceived = recv(this->fileDescriptor, receiveBuffer, length, 0);
                    printf("Received %d bytes\n", bytesReceived);

                    chunk->numberBytes = bytesReceived;
                    chunk->dataTypeIndetifier = dataType;
                    chunk->payload = receiveBuffer;

                    printf("Pushing length %d\n", chunk->numberBytes);
                    return bytesReceived;
                }
            }else{
                return 0;//correct header not returned
                //TODO - enter mode where keep looking for next starter byte
            }
        }else{//not even a header was received off buffer
            return 0; //no bytes received
        }
    }

}
