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

namespace RVR
{
    class Connection
        // A connection object represents one socket
    {
    private:

        const char* ipAddress;
        int fileDescriptor;
    public:
        std::string connectionName;
        void initializeNewSocket(std::string connectionName, const char* ipAddress);
        int createEndpoint();
        void initiateConnection();
        int terminateConnection();
        ssize_t sendData(char *message);
        ssize_t receiveData(void *receiveBuffer, size_t length);
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
        void sendData(std::string connectionName, char *message);
        void receiveData(std::string connectionName, void *receiveBuffer, size_t length);
    };
}

#endif //FIRMWARE_WIFI_H
