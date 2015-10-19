//
// Created by Bryce Carter on 8/25/15.
//

#ifndef FIRMWARE_WIFI_H
#define FIRMWARE_WIFI_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <queue>

namespace RVR
{
    class NetworkChunk
    {
    private:
    public:
        void* payload;
        int numberBytes;
        int dataTypeIndetifier;
    };

    class Connection
        // A connection object represents one socket
    {
    private:

        const char* ipAddress;
        int fileDescriptor;
        std::queue<NetworkChunk*> chunkQueue;                 // empty queue
    public:
        std::string connectionName;
        void initializeNewSocket(std::string connectionName, const char* ipAddress);
        int createEndpoint();
        void initiateConnection();
        int terminateConnection();
        int processData(NetworkChunk *chunk);
        int sendData(NetworkChunk *chunk);
        int receiveDataFromBuffer(NetworkChunk *chunk);
    };

    class NetworkManager
    {
    private:
        std::vector<Connection*> existingConnections;
        Connection* getConnectionPtrByConnectionName(std::string connectionNameToFind);
        int getPositionByConnectionName(std::string connectionNameToFind);
    public:
        void initializeNewConnection(std::string connectionName, const char* ipAddress);
        void terminateConnection(std::string connectionName);
        void sendData(std::string connectionName, NetworkChunk *chunk);
        int getData(std::string connectionName, NetworkChunk *chunk);
    };
}

#endif //FIRMWARE_WIFI_H
