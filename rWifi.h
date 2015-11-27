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
#include <unistd.h>
#include <unordered_map>
#include <math.h>
#include <time.h>


namespace RVR
{
    const int COMMAND_LENGTH     = 4;
    const int STATUS_LENGTH      = 4;
    const int CBHEADER_LENGTH    = 6;
    const int CBDATA_INFOLENGTH  = 3;
    const int CBDATA_DATALENGTH  = 500;


    enum class DataType
    //This enum holds the possible types of messages that can be sent. Each of these will be interpretted differently
    {
        COMMAND,
        STATUS,
        CAMERA,
        TEXT,
        CBHEADER, //4
        CBDATA, //5
        NONE
    };

    enum class ConnectionInitType
    // Values representing whether a connection is to host a new socket connection and wait for the a connection or actively try to connect
    {
        LISTEN,
        CONNECT
    };

    enum class ConnectionProtocol
    {
        TCP,
        UDP
    };

    enum class ReceiveType
    //When data has been processed, this enum is used to indicate what type was processed to calling function
    {
        NETWORKCHUNK,
        SEGMENT,
        NODATA
    };

    enum class CommandType
    //This enum holds the list of different commands that Rover can execute
    {
        DRIVE_FORWARD,
        DRIVE_BACKWARD,
        TURN_LEFT,
        TURN_RIGHT,
        STOP_DRIVE,
        START_STREAM,
        DISPENSE_TREAT,
        FLIP_CAMPERA
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

    class CbHeader
    //packs UID, size, and count into data. constructor decodes them. Method encodes them
    {
    private:
        int UID;
        int numBytes;
        int numSegments;
    public:
        CbHeader() { }
        CbHeader(NetworkChunk *networkChunk); //constructor - takes NetworkChunk (of type CBHEADER) and creates CbHeader
        void toNetworkChunk(NetworkChunk *newNetworkChunk); //export to NetworkChunk to be ready to send over network

        void setUID(int UIDToSet);
        void setNumBytes(int numBytesToSet);
        void setNumSegments(int numSegmentsToSet);
        int getUID();
        int getNumBytes();
        int getNumSegments();
    };

    class CbData
    //packs UID, index and data into data. constructor decodes them. Method encodes them
    {
    private:
        int UID;
        int index;
        char *data;
    public:
        CbData() { }
        CbData(NetworkChunk *networkChunk); //constructor - takes NetworkChunk and creates CbData
        void toNetworkChunk(NetworkChunk *newNetworkChunk); //export to NetworkChunk to be ready to send over network

        void setUID(int UIDToSet);
        void setIndex(int indexToSet);
        void setData(char *dataToSet);
        int getUID();
        int getIndex();
        char* getData();
    };

    class ChunkBox
    {
    private:
        int segmentsFilled;
        int totalSegments;
        int totalBytes;
        char* data;
        int isFull;
    public:
        ChunkBox() { }
        ChunkBox(CbHeader *cbHeader);

        int segmentsReceived = 0; //TODO - remove when not needed. Just for test
        time_t start,end; //for test
        void add(CbData *cbData);
        void setSegmentsFilled(int segmentsFilledToSet);
        void setTotalSegments(int totalSegmentsToSet);
        void setTotalBytes(int totalBytesToSet);
        void setData(char *dataToSet);
        void setIsFull(int isFullToSet);
        int getSegmentsFilled();
        int getTotalSegments();
        int getTotalBytes();
        char* getData();
        int getIsFull();
    };

    class Command
    {
    private:
        CommandType commandType; //what type of comamnd this is
        char* commandData = new char[COMMAND_LENGTH];
        bool dataExists = 0;
    public:
        Command() { }
        Command(NetworkChunk networkChunk); //constructor - takes NetworkChunk and creates command
        NetworkChunk toNetworkChunk(); //export to NetworkChunk to be ready to send over network

        void setDataExists(bool dataExistsToSet);
        void setCommandType(CommandType commandTypeToSet);
        void setCommandData(char* commandDataToSet);
        CommandType getCommandType();
        char* getCommandData();
        bool getDataExists();
    };

    class Status
    {
    private:
        StatusType statusType;
        char* statusData;
        bool dataExists = 0;
    public:
        Status() { }
        Status(NetworkChunk networkChunk);
        NetworkChunk toNetworkChunk();

        void setDataExists(bool dataExistsToSet);
        void setStatusType(StatusType statusTypeToSet);
        void setStatusData(char* statusDataToSet);
        StatusType getStatusType();
        char* getStatusData();
        bool getDataExists();
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
        const char *ipAddressLocal;
        const char *ipAddressRemote;
        struct sockaddr_in socketLocal;
        struct sockaddr_in socketRemote;
        ConnectionProtocol protocol;
        int fileDescriptor;
        std::queue<NetworkChunk*> chunkQueue;
        std::unordered_map<int,ChunkBox*> chunkAccumulator;
        int currUID = 0;
        char* udpData;
        int udpReadPosition; //TODO - make a class out of this
    public:
        std::string connectionName;
        int getFileDescriptor();
        int initializeNewSocket(std::string connectionName, const char* ipAddressLocal, const char* ipAddressRemote, u_short port, ConnectionProtocol protocol);
        int listenForConnection(int timeOut_ms);
        int initiateConnection();
        int terminateConnection();
        int bindToSocket();
        NetworkChunk processNewData();
        int sendData(NetworkChunk *chunk);
        ReceiveType receiveChunk(NetworkChunk *receivedChunk);
        int receive(char* buffer, int length);
        int checkDataHeader();
        void makeStream(NetworkChunk *chunk);
        int sendDataUnsegmented(NetworkChunk *chunk);
        void sendDataSegmented(NetworkChunk *chunk);
        int sendBitStream(char *bitStream, int length);
        void processDataOnBuffer();
        void processDataInMap();
        int getReceivedData(char **buffer, int length);
        ReceiveType popChunkFromQueue(NetworkChunk *chunk);
    };

    class NetworkManager
    {
    private:
        int connectTimeout_ms = 1000;
        std::vector<Connection*> existingConnections;
        Connection* getConnectionPtrByConnectionName(std::string connectionNameToFind);
        int getPositionByConnectionName(std::string connectionNameToFind);
    public:
        int initializeNewConnection(std::string connectionName, const char *ipAddressLocal, const char *ipAddressRemote, u_short port, ConnectionInitType initType, ConnectionProtocol protocol);

        void setConnectTimeout(int timeout_ms);

        void terminateConnection(std::string connectionName);
        void sendData(std::string connectionName, NetworkChunk *chunk);
        ReceiveType getData(std::string connectionName, NetworkChunk* chunk);
    };
}

#endif //FIRMWARE_WIFI_H
