//
// Created by Bryce Carter on 8/25/15.
//
#include "rWifi.h"
#include "../rCore/easylogging++.h"

#define RECEIVE_HEADER_LENGTH        1
#define RECEIVE_TYPELENGTH_LENGTH    2

//int receiveHeaderValue[RECEIVE_HEADER_LENGTH-1] = {52};          //minus 1 becuase last byte is information carrying byte
int receiveHeaderValue[] = {52};

namespace RVR
{
// ==============================================================
// NetworkChunk Member functions
// ==============================================================
    void NetworkChunk::setDataType(DataType dataTypeToSet)
    {
        this->dataType = dataTypeToSet;
    }

    void NetworkChunk::setData(char* dataToSet)
    {
        this->data = dataToSet;
    }

    void NetworkChunk::setLength(int lengthToSet)
    {
        this->length = lengthToSet;
    }

    DataType NetworkChunk::getDataType()
    {
        return this->dataType;
    }

    char* NetworkChunk::getData()
    {
        return this->data;
    }

    int NetworkChunk::getLength()
    {
        return this->length;
    }

// ==============================================================
// Command Member functions
// ==============================================================
    void Command::setCommandType(CommandType commandTypeToSet)
    {
        this->commandType = commandTypeToSet;
    }

    void Command::setCommandData(char* commandDataToSet){
        this->commandData = commandDataToSet;
    }

    CommandType Command::getCommandType()
    {
        return this->commandType;
    }

    char* Command::getCommandData()
    {
        return this->commandData;
    }

    Command::Command(NetworkChunk networkChunk)
    //This is the constructor. It takes a networkChunk and turns it into a command
    //First byte of data is the command type - data starts after that byte
    {
        this->commandType = static_cast<CommandType>((networkChunk.getData())[0]);
        this->commandData = networkChunk.getData()+1; //should be one byte after where the commandType was located
    }

    NetworkChunk Command::toNetworkChunk()
    {
        NetworkChunk* newNetworkChunk = new NetworkChunk;

        newNetworkChunk->setLength(COMMAND_LENGTH+1); //TODO - determine if desirable way to set length
        newNetworkChunk->setDataType(DataType::COMMAND);

        char* dataToSend = new char[COMMAND_LENGTH+1]; //TODO - Adjust for appropriate length if not 4
        dataToSend[0] = (static_cast<int>(this->getCommandType()) << 4) & COMMAND_LENGTH; //first 4 bits are command type. last 4 are length
        dataToSend[1] = (this->getCommandData())[0];
        dataToSend[2] = (this->getCommandData())[1];
        dataToSend[3] = (this->getCommandData())[2];
        dataToSend[4] = (this->getCommandData())[3];
        newNetworkChunk->setData(dataToSend);

        return *newNetworkChunk;
    }

// ==============================================================
// Status Member functions
// ==============================================================
    void Status::setStatusType(StatusType statusTypeToSet)
    {
        this->statusType = statusTypeToSet;
    }

    void Status::setStatusData(char* statusDataToSet)
    {
        this->statusData = statusDataToSet;
    }

    StatusType Status::getStatusType()
    {
        return this->statusType;
    }

    char* Status::getStatusData()
    {
        return this->statusData;
    }

    Status::Status(NetworkChunk networkChunk)
    {
        this->statusType = static_cast<StatusType>((networkChunk.getData())[0]);
        this->statusData = networkChunk.getData()+1; //should be one byte after where the commandType was located
    }

    NetworkChunk Status::toNetworkChunk()
    {
        NetworkChunk* newNetworkChunk = new NetworkChunk;

        newNetworkChunk->setLength(STATUS_LENGTH+1);//TODO - determine if desirable way to set length
        newNetworkChunk->setDataType(DataType::STATUS);

        char* dataToSend = new char[STATUS_LENGTH+1]; //TODO - Adjust for appropriate length if not 4
        dataToSend[0] = (static_cast<int>(this->getStatusType()) << 4) & STATUS_LENGTH; //first 4 bits are command type. last 4 are length
        dataToSend[1] = (this->getStatusData())[0];
        dataToSend[2] = (this->getStatusData())[1];
        dataToSend[3] = (this->getStatusData())[2];
        dataToSend[4] = (this->getStatusData())[3];
        newNetworkChunk->setData(dataToSend);

        return *newNetworkChunk;
    }


// ==============================================================
// Text Member functions
// ==============================================================
    void Text::setLength(int lengthToSet)
    {
        this->length = lengthToSet;
    }

    void Text::setTextMessage(char* textMessageToSet)
    {
        this->textMessage = textMessageToSet;
    }

    int Text::getLength()
    {
        return this->length;
    }

    char* Text::getTextMessage()
    {
        return this->textMessage;
    }

    Text::Text(NetworkChunk networkChunk)
    {
        this->length = networkChunk.getLength();
        this->textMessage = networkChunk.getData();
    }

    NetworkChunk Text::toNetworkChunk()
    {
        NetworkChunk* newNetworkChunk = new NetworkChunk;

        newNetworkChunk->setLength(this->length); //Equal to the length stored in the Text object
        newNetworkChunk->setDataType(DataType::TEXT);
        newNetworkChunk->setData(this->textMessage);

        return *newNetworkChunk;
    }

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

    NetworkChunk NetworkManager::getData(std::string connectionName)
    //Received data will be passed back in return argument
    {
        VLOG(1) << "Receiving data...";
        Connection* connectionPtr = getConnectionPtrByConnectionName(connectionName);
        NetworkChunk data = connectionPtr->processData();

        return data;
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
            VLOG(2) << "Failed to initialize new socket";
        }else{
            VLOG(2) << "Successfully initiated new socket!\nFileDescriptor: " << this->fileDescriptor << "\nConnection name: " << this->connectionName << "\nIP Address: " << this->ipAddress <<"\n";
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

        VLOG(1) << "Attempting to connect...";
        int successStatus = connect(this->fileDescriptor, (struct sockaddr *) &socketAddress, sizeof(socketAddress));
        if (successStatus == -1){
            VLOG(1) << "Failed to initiate connection";
        }else{
            VLOG(1) << "Sucessfully initiated connection";
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
        ssize_t bytesSent = send(this->fileDescriptor, chunk->getData(), chunk->getLength(), 0);
        printf("Sent %d bytes\n", bytesSent);
        return bytesSent;
    }

    NetworkChunk Connection::processData()
    //function takes everything that is on the buffer and puts it in a queue of NetworkChunk*'s. It then returns the oldest
    //to be processed
    {
        int bytesReceived;
        int chunksReceived = 0;

        do{//while there is data to be received in the buffer
            NetworkChunk* receivedChunk = new NetworkChunk; //create a chunk
            VLOG(2) << "Data is being received off buffer...";
            bytesReceived = this->receiveDataFromBuffer(&receivedChunk);//try to fill it from the buffer

            if(bytesReceived > 0){ //if something was received, store it into the queue
                VLOG(2) << "Data being pushed into queue";
                this->chunkQueue.push (receivedChunk);
            }
            else {//buffer was empty - nothing was received
                VLOG(2) << "No data to push into queue";
                bytesReceived = 0;
            }
            chunksReceived++;
        }while(bytesReceived > 0 & chunksReceived < 1);

        if (!this->chunkQueue.empty())//if there's something in the queue (it's not empty)
        {
            NetworkChunk *oldestChunk;
            VLOG(2) << "Popping data out of queue";
            oldestChunk = this->chunkQueue.front();    //access the oldest chunk in the queue
            this->chunkQueue.pop();//delete the oldest chunk from the queue
            return *oldestChunk;
        } else{
            VLOG(2) << "No data in queue to pop";
            //TODO - what to return here?
        }
    }

    int Connection::checkReceivedDataHeader(char* header)
    {
        VLOG(2) << "Checking data header...";
        for (int i=0;i<(RECEIVE_HEADER_LENGTH);i++)//minus 1 because last byte is the one carrying length/type info
        {
            if(header[i] != receiveHeaderValue[i])//check that the header we received = header expected
            {
                VLOG(2) << "Incorrect data header";
                return 0;
            }
        }
        VLOG(2) << "Correct data header";
        return 1;
    }

    int Connection::receiveDataFromBuffer(NetworkChunk **receivedChunk)
    //Upon successful completion, recv() shall return the length of the message in bytes. If no messages are available to be
    //received and the peer has performed an orderly shutdown, recv() shall return 0. Otherwise, -1 shall be returned to indicate error.
    //Last input argument for recv = 0 to indicate no flags
    {
        char header[RECEIVE_HEADER_LENGTH+RECEIVE_TYPELENGTH_LENGTH];

        int headerBytesReceived = recv(this->fileDescriptor, header, RECEIVE_HEADER_LENGTH+RECEIVE_TYPELENGTH_LENGTH, 0);         //Receive starter byte

        if (headerBytesReceived == RECEIVE_HEADER_LENGTH+RECEIVE_TYPELENGTH_LENGTH){ //if correct number of bytes for header were received
            if(this->checkReceivedDataHeader(header)){
                DataType dataType = static_cast<DataType>(header[RECEIVE_HEADER_LENGTH] >> 4);//typecast into dataType
                int length = (((header[RECEIVE_HEADER_LENGTH] & 0x0f) << 8) | header[RECEIVE_HEADER_LENGTH + 1]);
                VLOG(2) << "Length is " << length;
                char *receiveBuffer = new char[length];
                int bytesReceived = 0;

                bytesReceived = recv(this->fileDescriptor, receiveBuffer, length, 0);

                (*receivedChunk)->setDataType(dataType);
                (*receivedChunk)->setLength(length);
                (*receivedChunk)->setData(receiveBuffer);
                return bytesReceived;
            }else{
                VLOG(2) << "Received data has incorrect header";
                return 0;//correct header not returned
                //TODO - enter mode where keep looking for next starter byte
            }
        }else{//not even a header was received off buffer
            VLOG(1) << "No data received";
            return 0;
        }
    }

}
