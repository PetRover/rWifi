//
// Created by Bryce Carter on 8/25/15.
//
#include "rWifi.h"

#define IPV4_LENGTH 12

namespace RVR
{
// ==============================================================
// Connection NetworkManager Member functions
// ==============================================================
    void NetworkManager::initializeNewConnection(std::string connectionName, std::string ipAddress)
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

    void NetworkManager::sendData(std::string connectionName, const void *message, int length) //TODO - determine type instead of void*
    {
        Connection* connectionPtr = getConnectionPtrByConnectionName(connectionName);
        connectionPtr->sendData(message, length);

        return;
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
    }
// ==============================================================
// Connection Class Member functions
// ==============================================================

    void Connection::initializeNewSocket(std::string connectionName, std::string ipAddress)
    {
        this->ipAddress = ipAddress;
        this->connectionName = connectionName;
        this->fileDescriptor = this->createEndpoint();

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

    int Connection::initiateConnection()
    //Connects to a socket. Input parameter is the IP address to be formatted.
    //Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned
    {
        std::string ipAddressRaw = this->ipAddress;

        //reverse string for easier parsing
        for (int i = 0; i < ipAddressRaw.length(); i++)
        {
            ipAddress[i] = ipAddressRaw[ipAddressRaw.length()-1-i];
        }

        //parse IP address string into form used for connection
        int digit = 0;
        int byte = 0;
        int IPByte[4] = {0};
        for (int i=0; i<ipAddress.length(); ++i)
        {
            if (ipAddress[i] == '.')
            {
                digit = 0;
                byte++;
                continue;
            }
            IPByte[byte] = ipAddress[i]*10^digit;
            digit++;
        }

        struct sockaddr_in socketAddress;
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_port = 1024;
        socketAddress.sin_addr.s_addr = (((((IPByte[0] << 8) | IPByte[1]) << 8) | IPByte[2]) << 8) | IPByte[3];

        int successStatus = connect(this->fileDescriptor, (struct sockaddr *) &socketAddress, IPV4_LENGTH);
        return successStatus;
    }

    int Connection::terminateConnection()
    //Causes connection on the socket associated with first input parameter to be shut down. Further receptions and transmissions will be disallowed.
    //Second input parameter is defaulted to SHUT_RDWR to stop all further receptions and transmissions instead of just receptions or just transmissions
    //On success, zero is returned.  On error, -1 is returned
    {
        int successStatus = shutdown(this->fileDescriptor, SHUT_RDWR);
        return successStatus;
    }

    int Connection::sendData(const void *message, int messageLength)
    //Upon successful completion, send() shall return the number of bytes sent. Otherwise, -1 shall be returned
    //The first input parameter is a pointer to the buffer where the message to be transmitted is stored.
    //The second input parameter is the message length.
    {
        int bytesSent = send(this->fileDescriptor, message, messageLength, 0);
        return bytesSent;
    }
}