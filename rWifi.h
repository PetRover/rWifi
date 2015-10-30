//
// Created by Bryce Carter on 8/25/15.
//

#ifndef FIRMWARE_WIFI_H
#define FIRMWARE_WIFI_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>

namespace RVR
{
    const int COMMAND_LENGTH = 4;
    const int STATUS_LENGTH = 4;


    enum class DataType
    //This enum holds the possible types of messages that can be sent. Each of these will be interpretted differently
    {
        COMMAND,
        STATUS,
        CAMERA,
        TEXT,
        NONE
    };

    enum class ConnectionInitType
    // Values representing whether a connection is to host a new socket connection and wait for the a connection or actively try to connect
    {
        LISTEN,
        CONNECT
    };

    enum class CommandType
    //This enum holds the list of different commands that Rover can execute
    {
        DRIVE_FORWARD,
        DRIVE_BACKWARD,
        DRIVE_LEFT
    };

    enum class StatusType
    //This enum holds the list of different pieces of information. This information does not require anything be executed
    {
        CHARGING,
        NOT_CHARGING
    };

    class NetworkChunk
    {
    private:
        DataType dataType;
        char* data;
        int length;
    public:
        NetworkChunk() { }
        NetworkChunk(DataType dataTypeToSet, int lengthToSet, char* dataToSet);
        void setDataType(DataType dataTypeToSet);
        void setData(char* dataToSet);
        void setLength(int lengthToSet);
        DataType getDataType();
        char* getData();
        int getLength();
    };

//    NetworkChunk NULL_CHUNK = NetworkChunk(DataType::NONE, 0, nullptr);


    class Command
    {
    private:
        CommandType commandType; //what type of comamnd this is
        char* commandData; //holds array of bytes that are passed in as data
    public:
        Command() { }
        Command(NetworkChunk networkChunk); //constructor - takes NetworkChunk and creates command
        NetworkChunk toNetworkChunk(); //export to NetworkChunk to be ready to send over network

        void setCommandType(CommandType commandTypeToSet);
        void setCommandData(char* commandDataToSet);
        CommandType getCommandType();
        char* getCommandData();
    };

    class Status
    {
    private:
        StatusType statusType;
        char* statusData;
    public:
        Status() { }
        Status(NetworkChunk networkChunk);
        NetworkChunk toNetworkChunk();

        void setStatusType(StatusType statusTypeToSet);
        void setStatusData(char* statusDataToSet);
        StatusType getStatusType();
        char* getStatusData();
    };

    class Text
    {
    private:
        int length;
        char* textMessage;
    public:
        Text() { }
        Text(NetworkChunk networkChunk);
        NetworkChunk toNetworkChunk();

        void setLength(int lengthToSet);
        void setTextMessage(char* textMessageToSet);
        int getLength();
        char* getTextMessage();
    };

    class Connection
        // A connection object represents one socket
    {
    private:
        const char *ipAddress;
        struct sockaddr_in socketAddress;
        int type;
        int fileDescriptor;
        std::queue<NetworkChunk*> chunkQueue;                 // empty queue
    public:
        std::string connectionName;
        void initializeNewSocket(std::string connectionName, const char* ipAddress, u_short port, int type);
        int createEndpoint();
        int listenForConnection(int timeOut_ms);
        void initiateConnection();
        int terminateConnection();
        NetworkChunk processData();
        int sendData(NetworkChunk *chunk);
        int receiveDataFromBuffer(NetworkChunk **chunk);
        int checkReceivedDataHeader(char* header);
        int scanToFindCorrectHeader(NetworkChunk **receivedChunk, char* header);
        int processReceivedData(NetworkChunk **receivedChunk, char* header);

    };

    class NetworkManager
    {
    private:
        int connectTimeout_ms = 1000;
        std::vector<Connection*> existingConnections;
        Connection* getConnectionPtrByConnectionName(std::string connectionNameToFind);
        int getPositionByConnectionName(std::string connectionNameToFind);
    public:
        void initializeNewConnection(std::string connectionName, const char *ipAddress, u_short port, ConnectionInitType initType); // Connection type defaults to SOCK_STREAM
        void initializeNewConnection(std::string connectionName, const char *ipAddress, u_short port, ConnectionInitType initType, int socketType);

        void setConnectTimeout(int timeout_ms);

        void terminateConnection(std::string connectionName);
        void sendData(std::string connectionName, NetworkChunk *chunk);
        NetworkChunk getData(std::string connectionName);
    };
}

#endif //FIRMWARE_WIFI_H
