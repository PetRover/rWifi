//
// Created by Bryce Carter on 8/25/15.
//
#include "rWifi.h"

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

    int NetworkManager::receiveData(std::string connectionName, char *receiveBuffer, int length)
    //Received data will be stored in the buffer which is passed to us via the pointer receiveBuffer
    {
        Connection* connectionPtr = getConnectionPtrByConnectionName(connectionName);
        int receivedBytes = connectionPtr->receiveData(receiveBuffer, length);

        return receivedBytes;
        //TODO - check that the number of bytes returned is equal to the number of bytes I expect (length)
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
   //     socketAddress.sin_addr.s_addr = inet_addr(this->ipAddress);
        socketAddress.sin_addr.s_addr = inet_addr("192.168.7.1");
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

    int Connection::receiveData(char *receiveBuffer, int length)
    //Upon successful completion, recv() shall return the length of the message in bytes. If no messages are available to be
    //received and the peer has performed an orderly shutdown, recv() shall return 0. Otherwise, -1 shall be returned to indicate error.
    //Last input argument for recv = 0 to indicate no flags
    {
        int bytesReceived = recv(this->fileDescriptor, receiveBuffer, length, 0);
        printf("Received %d bytes\n", bytesReceived);
        return bytesReceived;
    }
}
