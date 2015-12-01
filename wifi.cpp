//
// Created by Bryce Carter on 8/25/15.
//
#include "rWifi.h"
#include "../rCore/easylogging++.h"

#define RECEIVE_HEADER_LENGTH        1
#define RECEIVE_TYPELENGTH_LENGTH    3
#define CHUNKBOX_FULL_PERCENT        100
#define MAX_UID                      255

int receiveHeaderValue[] = {67};

const char*  ROVER_IP = "192.168.1.222";
const char*  APP_IP = "192.168.1.6";

namespace RVR
{
// ==============================================================
// NetworkChunk Member functions
// ==============================================================
    void NetworkChunk::setDataType(DataType dataTypeToSet)
    {
        this->dataType = dataTypeToSet;
    }

    void NetworkChunk::setData(char *dataToSet)
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

    char *NetworkChunk::getData()
    {
        return this->data;
    }

    int NetworkChunk::getLength()
    {
        return this->length;
    }

    NetworkChunk::NetworkChunk(DataType dataTypeToSet, int lengthToSet, char *dataToSet)
    {
        this->dataType = dataTypeToSet;
        this->length = lengthToSet;
        this->data = dataToSet;
    }

    NetworkChunk::~NetworkChunk()
    {
        if(this->deallocateArray){
            delete [] (this->data);
        }
    }

// ==============================================================
// ChunkBox Member functions
// ==============================================================
    void ChunkBox::setSegmentsFilled(int segmentsFilledToSet)
    {
        this->segmentsFilled = segmentsFilledToSet;
    }

    void ChunkBox::setTotalSegments(int totalSegmentsToSet)
    {
        this->totalSegments = totalSegmentsToSet;
    }

    void ChunkBox::setTotalBytes(int totalBytesToSet)
    {
        this->totalBytes = totalBytesToSet;
    }

    void ChunkBox::setData(char *dataToSet)
    {
        this->data = dataToSet;
    }

    void ChunkBox::setIsFull(int isFullToSet)
    {
        this->isFull = isFullToSet;
    }

    int ChunkBox::getSegmentsFilled()
    {
        return this->segmentsFilled;
    }

    int ChunkBox::getTotalSegments()
    {
        return this->totalSegments;
    }

    int ChunkBox::getTotalBytes()
    {
        return this->totalBytes;
    }

    char* ChunkBox::getData()
    {
        return this->data;
    }

    int ChunkBox::getIsFull()
    {
        return this->isFull;
    }

    void ChunkBox::add(CbData *cbData)
    {
        int index = cbData->getIndex();
        this->segmentsFilled++;

        VLOG(3) << "Adding data into chunkBox at index: " << index;

        std::copy ( (cbData->getData()), (cbData->getData())+CBDATA_DATALENGTH, (this->data)+index*CBDATA_DATALENGTH ); //Faster than a for loop for copying

        return;
    }

    ChunkBox::ChunkBox(CbHeader *cbHeader)
    {
        this->segmentsFilled = 0;
        this->totalSegments = cbHeader->getNumSegments();
        this->totalBytes = cbHeader->getNumBytes();
        this->isFull = 0;
        VLOG(3) << "Total bytes: " << this->totalBytes;
        VLOG(3) << "Total segments: " << this->totalSegments;

        char *newData = new char[this->totalSegments*CBDATA_DATALENGTH];
        this->data = newData;

        VLOG(3) << "Made chunkBox of size: " << this->totalSegments*CBDATA_DATALENGTH;

    }

    ChunkBox::~ChunkBox()
    {
        delete[] (this->data);
    }

// ==============================================================
// Command Member functions
// ==============================================================
    void Command::setCommandType(CommandType commandTypeToSet)
    {
        this->commandType = commandTypeToSet;
    }

    void Command::setDataExists(bool dataExistsToSet)
    {
        this->dataExists = dataExistsToSet;
    }

    void Command::setCommandData(char *commandDataToSet)
    {
        this->commandData = commandDataToSet;
        this->dataExists = 1;
    }

    CommandType Command::getCommandType()
    {
        return this->commandType;
    }

    bool Command::getDataExists()
    {
        return this->dataExists;
    }

    char *Command::getCommandData()
    {
        return this->commandData;
    }

    Command::Command(NetworkChunk *networkChunk)
    //This is the constructor. It takes a networkChunk and turns it into a command
    //First byte of data is the command type - data starts after that byte
    {
        this->commandType = static_cast<CommandType>((networkChunk->getData())[0] >> 4);
        VLOG(3) << "Creating Command: Type = " << (int) ((networkChunk->getData())[0] >> 4);

        (this->commandData)[0] = (networkChunk->getData())[1];
        (this->commandData)[1] = (networkChunk->getData())[2];
        (this->commandData)[2] = (networkChunk->getData())[3];
        (this->commandData)[3] = (networkChunk->getData())[4];

        this->dataExists = (this->commandData[0] & 0x80) ? 1 : 0; //If dataExists bit (x) 0bx0000000 is a 1, then set dataExists = 1

        networkChunk->setData(networkChunk->getData()-(RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH));

        delete[] networkChunk->getData();
    }

    NetworkChunk* Command::toNetworkChunk()
    {
        NetworkChunk *newNetworkChunk = new NetworkChunk;

        newNetworkChunk->setLength(COMMAND_LENGTH + RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH + 1);
        newNetworkChunk->setDataType(DataType::COMMAND);

        char *dataToSend = new char[COMMAND_LENGTH + RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH+ 1];
        dataToSend += (RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH); //pointer is placed 3 from the start, but space allocated 3 behind it.

        dataToSend[0] = (((static_cast<int>(this->getCommandType())) << 4) |
                         COMMAND_LENGTH); //first 4 bits are command type. last 4 are length

        if (this->dataExists)
        {
            dataToSend[1] = 0x80 | ((this->getCommandData())[0] & 0x7f); //dataExists bit (x) 0bx0000000 is a 1
            dataToSend[2] = (this->getCommandData())[1];
            dataToSend[3] = (this->getCommandData())[2];
            dataToSend[4] = (this->getCommandData())[3];
        } else
        {
            dataToSend[1] = 0; //dataExists bit (x) 0bx0000000 is a 0
            dataToSend[2] = 0;
            dataToSend[3] = 0;
            dataToSend[4] = 0;
        }

        newNetworkChunk->setData(dataToSend);

        return newNetworkChunk;
    }

// ==============================================================
// Status Member functions
// ==============================================================
    void Status::setStatusType(StatusType statusTypeToSet)
    {
        this->statusType = statusTypeToSet;
    }

    void Status::setStatusData(char *statusDataToSet)
    {
        this->statusData = statusDataToSet;
        this->dataExists = 1;
    }

    void Status::setDataExists(bool dataExistsToSet)
    {
        this->dataExists = dataExistsToSet;
    }

    StatusType Status::getStatusType()
    {
        return this->statusType;
    }

    char *Status::getStatusData()
    {
        return this->statusData;
    }

    bool Status::getDataExists()
    {
        return this->dataExists;
    }

    Status::Status(NetworkChunk networkChunk)
    {
        this->statusType = static_cast<StatusType>((networkChunk.getData())[0] >> 4);
        VLOG(2) << "Creating Status: Type = " << (int) ((networkChunk.getData())[0] >> 4);
        this->statusData = networkChunk.getData() + 1; //should be one byte after where the commandType was located

        VLOG(2) << "Data = " << (int) this->statusData[0] << " " << (int) this->statusData[1] << " " <<
                (int) this->statusData[2] << " " << (int) this->statusData[3];
        this->dataExists = (this->statusData[0] & 0x80) ? 1
                                                        : 0; //If dataExists bit (x) 0bx0000000 is a 1, then set dataExists = 1, else 0
        VLOG(2) << "This status contains data: " << this->dataExists;
    }

    NetworkChunk* Status::toNetworkChunk()
    {
        NetworkChunk *newNetworkChunk = new NetworkChunk;

        newNetworkChunk->setLength(STATUS_LENGTH + 1);
        newNetworkChunk->setDataType(DataType::STATUS);

        char *dataToSend = new char[STATUS_LENGTH + 1];
        dataToSend[0] = (((static_cast<int>(this->getStatusType())) << 4) |
                         STATUS_LENGTH); //first 4 bits are command type. last 4 are length
        if (this->dataExists)
        {
            dataToSend[1] = 0x80 | ((this->getStatusData())[0] & 0x7f); //dataExists bit (x) 0bx0000000 is a 1
            dataToSend[2] = (this->getStatusData())[1];
            dataToSend[3] = (this->getStatusData())[2];
            dataToSend[4] = (this->getStatusData())[3];
        } else
        {
            dataToSend[1] = 0; //dataExists bit (x) 0bx0000000 is a 0
            dataToSend[2] = 0;
            dataToSend[3] = 0;
            dataToSend[4] = 0;
        }

        newNetworkChunk->setData(dataToSend);

        return newNetworkChunk;
    }


// ==============================================================
// Text Member functions
// ==============================================================
    void Text::setLength(int lengthToSet)
    {
        this->length = lengthToSet;
    }

    void Text::setTextMessage(char *textMessageToSet)
    {
        this->textMessage = textMessageToSet;
    }

    int Text::getLength()
    {
        return this->length;
    }

    char *Text::getTextMessage()
    {
        return this->textMessage;
    }

    Text::Text(NetworkChunk networkChunk)
    {
        this->length = networkChunk.getLength();
        this->textMessage = networkChunk.getData();
    }

    NetworkChunk* Text::toNetworkChunk()
    {
        NetworkChunk *newNetworkChunk = new NetworkChunk;

        newNetworkChunk->setLength(this->length); //Equal to the length stored in the Text object
        newNetworkChunk->setDataType(DataType::TEXT);
        newNetworkChunk->setData(this->textMessage);

        return newNetworkChunk;
    }

// ==============================================================
// CbHeader Member functions
// ==============================================================
    void CbHeader::setUID(int UIDToSet)
    {
        this->UID = UIDToSet;
    }

    void CbHeader::setNumBytes(int numBytesToSet)
    {
        this->numBytes = numBytesToSet;
    }

    void CbHeader::setNumSegments(int numSegmentsToSet)
    {
        this->numSegments = numSegmentsToSet;
    }

    int CbHeader::getUID()
    {
        return this->UID;
    }

    int CbHeader::getNumBytes()
    {
        return this->numBytes;
    }

    int CbHeader::getNumSegments()
    {
        return this->numSegments;
    }

    CbHeader::CbHeader(NetworkChunk *networkChunk)
    {
        this->UID = static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[0]));
        VLOG(2) << "received CbHeader UID " << this->UID;
        this->numBytes = (static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[1])) << 16) | (static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[2])) << 8) | static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[3]));
        this->numSegments = (static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[4])) << 8) | static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[5]));

    }

    NetworkChunk* CbHeader::toNetworkChunk()
    {
        NetworkChunk *transmitChunk = new NetworkChunk();

        transmitChunk->setLength(CBHEADER_LENGTH + CBDATA_DATALENGTH - 3);//-1
        transmitChunk->setDataType(DataType::CBHEADER);

        char *dataToSend = new char[RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH + CBHEADER_LENGTH + CBDATA_DATALENGTH - 1];
        dataToSend += (RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH); //pointer is placed 3 from the start, but space allocated 3 behind it.

        dataToSend[0] = this->UID;
        dataToSend[1] = ((this->numBytes) >> 16);
        dataToSend[2] = ((this->numBytes) >> 8) & 0xff;
        dataToSend[3] = (this->numBytes) & 0xff;
        dataToSend[4] = ((this->numSegments) >> 8);
        dataToSend[5] = (this->numSegments) & 0xff;

        transmitChunk->setData(dataToSend);

        return transmitChunk;
    }

// ==============================================================
// CbData Member functions
// ==============================================================
    void CbData::setUID(int UIDToSet)
    {
        this->UID = UIDToSet;
    }

    void CbData::setIndex(int indexToSet)
    {
        this->index = indexToSet;
    }

    void CbData::setData(char *dataToSet)
    {
        this->data = dataToSet;
    }

    int CbData::getUID()
    {
        return this->UID;
    }

    int CbData::getIndex()
    {
        return this->index;
    }

    char* CbData::getData()
    {
        return this->data;
    }

    CbData::CbData(NetworkChunk *networkChunk)
    {
//        this->UID = (networkChunk->getData())[0];
        this->UID = static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[0]));
        this->index = (static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[1])) << 8) | static_cast<int>(static_cast<unsigned char>((networkChunk->getData())[2]));
        this->data = networkChunk->getData() + 3;

        VLOG(3) << "Received CbData UID " << this->UID;
        VLOG(3) << "Received CbData index " << this->index;
    }

    NetworkChunk* CbData::toNetworkChunk()
    {
        NetworkChunk *transmitChunk = new NetworkChunk();

        transmitChunk->setLength(CBDATA_INFOLENGTH+CBDATA_DATALENGTH);
        transmitChunk->setDataType(DataType::CBDATA);

        //It's faster not to copy all of the data over and to append the info on to the front. This is done for every CbData except
        //the first where we don't have permission to write into the 6 bytes of memory before the data
        if (this->index == 0){
            char *dataToSend = new char[RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH + CBDATA_INFOLENGTH + CBDATA_DATALENGTH];
            dataToSend += (RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH);

            dataToSend[0] = this->UID;
            dataToSend[1] = (this->index) >> 8;
            dataToSend[2] = (this->index) & 0xff;

            std::copy ( this->data, this->data+CBDATA_DATALENGTH, dataToSend+3 ); //Faster than a for loop for copying

            transmitChunk->setData(dataToSend);
        }else{
            this->data = this->data-(CBDATA_INFOLENGTH+1); //change pointer back 3 (to location originally allocated);
            (this->getData())[0] = this->UID;
            (this->getData())[1] = (this->index) >> 8;
            (this->getData())[2] = (this->index) & 0xff;

            transmitChunk->setData(this->getData());
            transmitChunk->deallocateArray = 0;
        }

        return transmitChunk;
    }

// ==============================================================
// Connection NetworkManager Member functions
// ==============================================================

    int NetworkManager::initializeNewConnection(std::string connectionName, const char *ipAddressLocal, const char *ipAddressRemote, u_short port, ConnectionInitType initType, ConnectionProtocol protocol)
    {
        Connection *connectionPtr = new Connection(); //never deleted - no instance in code where we end a connection
        connectionPtr->initializeNewSocket(connectionName, ipAddressLocal, ipAddressRemote, port, protocol);
        this->existingConnections.push_back(connectionPtr);//add the connection pointer to list of connection pointers

        switch (initType)
        {
            case ConnectionInitType::CONNECT:
                switch (protocol)
                {
                    case ConnectionProtocol::TCP:
                        if (!(connectionPtr->initiateConnection()))
                        {
                            return 0; //failed to initiateConnection
                        }
                        {
                            int flags = fcntl(connectionPtr->getFileDescriptor(), F_GETFL, 0);
                            flags |= O_NONBLOCK;
                            fcntl(connectionPtr->getFileDescriptor(), F_SETFL, flags); //set socket to non-blocking
                        }
                        break;
                    case ConnectionProtocol::UDP: //BBB
                        if (!(connectionPtr->bindToSocket()))
                        {
                            return 0; //failed to bind to socket
                        }
                        {
                            int flags = fcntl(connectionPtr->getFileDescriptor(), F_GETFL, 0);
                            flags |= O_NONBLOCK;
                            fcntl(connectionPtr->getFileDescriptor(), F_SETFL, flags); //set socket to non-blocking
                        }
                        break;
                }
                break;
            case ConnectionInitType::LISTEN:
                switch (protocol)
                {
                    case ConnectionProtocol::TCP:
                    {
                        int flags = fcntl(connectionPtr->getFileDescriptor(), F_GETFL, 0);
                        flags |= O_NONBLOCK;
                        fcntl(connectionPtr->getFileDescriptor(), F_SETFL, flags); //set socket to non-blocking
                    }
                        connectionPtr->listenForConnection(this->connectTimeout_ms);
                        break;
                    case ConnectionProtocol::UDP:
                        if (!(connectionPtr->bindToSocket()))
                        {
                            return 0; //failed to bind to socket
                        }
                        {
                            int flags = fcntl(connectionPtr->getFileDescriptor(), F_GETFL, 0);
                            flags |= O_NONBLOCK;
                            fcntl(connectionPtr->getFileDescriptor(), F_SETFL, flags); //set socket to non-blocking
                        }
                        break;
                }
                break;

        }
        return 1;
    }

    void NetworkManager::terminateConnection(std::string connectionName)
    {
        Connection *connectionPtr = getConnectionPtrByConnectionName(connectionName);
        if (connectionPtr!= nullptr)
        {
            int connectionPosition = getPositionByConnectionName(connectionName);
            connectionPtr->terminateConnection();
            existingConnections.erase(existingConnections.begin() + connectionPosition);
        }
        return;
    }

    void NetworkManager::sendData(std::string connectionName, NetworkChunk *chunk)
    {
        Connection *connectionPtr = getConnectionPtrByConnectionName(connectionName);
        if (connectionPtr!= nullptr)
        {
            connectionPtr->makeStream(chunk);
            if(connectionPtr->connectionName == "CAMERA")
            {
                chunk->deallocateArray = 0;
                delete[] (connectionPtr->getUdpData());
            }
            delete chunk; //delete the memory for the chunk allocated with new by the camera
        }
        return;
    }

    ReceiveType NetworkManager::getData(std::string connectionName, NetworkChunk *chunk)
    //Received data will be passed back in return argument
    {
        VLOG(3) << "Receiving data";
        Connection *connectionPtr = getConnectionPtrByConnectionName(connectionName);
        if (connectionPtr!= nullptr)
        {
            VLOG(3) << "Processing buffer data...";
            connectionPtr->processDataOnBuffer();
            VLOG(3) << "[ DONE ] buffer data processed";
            VLOG(3) << "Processing Map data...";
            connectionPtr->processDataInMap();
            VLOG(3) << "[ DONE ] map data processed";
            VLOG(3) << "Getting the next in line...";
            ReceiveType typeReceived = connectionPtr->popChunkFromQueue(chunk);

            return typeReceived;
        }
        return ReceiveType::NODATA;
    }

    Connection *NetworkManager::getConnectionPtrByConnectionName(std::string connectionNameToFind)
    //iterate through existingConnections list to find the connection ptr associated with the given name
    {
        for (std::vector<Connection *>::iterator it = this->existingConnections.begin();
             it != this->existingConnections.end(); ++it)
        {
            if ((*it)->connectionName == connectionNameToFind)
            {
                return *it;
            }
        }

        VLOG(2) << "Invalid connection name";
        return nullptr;
    }

    int NetworkManager::getPositionByConnectionName(std::string connectionNameToFind)
    //Used for deleting connection
    {
        int count = 0;
        for (std::vector<Connection *>::iterator it = this->existingConnections.begin();
             it != this->existingConnections.end(); ++it)
        {
            if ((*it)->connectionName == connectionNameToFind)
            {
                return count;
            }
            count++;
        }

        return 0;
    }

    void NetworkManager::setConnectTimeout(int timeout_ms)
    {
        this->connectTimeout_ms = timeout_ms;
    }
// ==============================================================
// Connection Class Member functions
// ==============================================================

    int Connection::initializeNewSocket(std::string connectionName, const char *ipAddressLocal, const char *ipAddressRemote, u_short port, ConnectionProtocol protocol)
    {
        this->connectionName = connectionName;
        this->protocol = protocol;

        this->ipAddressLocal = ipAddressLocal;
        this->socketLocal.sin_addr.s_addr = inet_addr(ipAddressLocal);
        this->socketLocal.sin_family = AF_INET;
        this->socketLocal.sin_port = htons(port);

        this->ipAddressRemote = ipAddressRemote;
        this->socketRemote.sin_addr.s_addr = inet_addr(ipAddressRemote);
        this->socketRemote.sin_family = AF_INET;
        this->socketRemote.sin_port = htons(port);

        switch (this->protocol)
        {
            case ConnectionProtocol::TCP:
                this->fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
                break;
            case ConnectionProtocol::UDP:
                this->fileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
                break;
        }

        if (fileDescriptor == -1)
        {
            VLOG(2) << "Failed to initialize new socket";
            return 0;
        } else
        {
            VLOG(2) << "Successfully initiated new socket!\nFileDescriptor: " << this->fileDescriptor <<
                    "\nConnection name: " << this->connectionName << "\nIP Address: " << this->ipAddressLocal << "\n";
            return 1;
        }
    }

    int Connection::getFileDescriptor()
    {
        return this->fileDescriptor;
    }

    char* Connection::getUdpData()
    {
        return this->udpData;
    }


    int Connection::bindToSocket()
    {
        int successStatus = bind(this->fileDescriptor, (struct sockaddr *) &this->socketLocal, sizeof(this->socketLocal));
        if (successStatus == -1)
        {
            VLOG(2) << "Failed to bind to socket";
            return 0;
        } else
        {
            VLOG(2) << "Successfully bount to socket";
            return 1;
        }
    }

    int Connection::listenForConnection(int timeOut_ms)
    {
        // Put the socket in non blocking mode (making sure to preserve any previous flags)
        int flags = fcntl(this->fileDescriptor, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(this->fileDescriptor, F_SETFL, flags);

        // setup the timeout struct for the select call
        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;//100*timeOut_ms;

        // Bind the socket to the sever address
        VLOG(2) << "Binding to: " << this->ipAddressLocal << ":" << this->socketLocal.sin_port;
        bind(this->fileDescriptor, (struct sockaddr *) &this->socketLocal, sizeof(this->socketLocal));
        VLOG(2) << "[DONE]";

        // start the sever listening for connection with a queue size of 1
        VLOG(2) << "Starting listener";
        listen(this->fileDescriptor, 1);
        VLOG(2) << "[DONE]";

        fd_set fdSet;

        FD_ZERO(&fdSet);
        FD_SET(this->fileDescriptor, &fdSet);

        VLOG(2) << "Waiting for a connection to the listening socket...";

        int err = select(this->fileDescriptor + 1, &fdSet, NULL, NULL, &tv);
        LOG(DEBUG) << strerror(errno);
        if (err > 0)
        {
            if (FD_ISSET(this->fileDescriptor, &fdSet))
            {

                socklen_t addrLen = sizeof(this->socketLocal);
                int fd = accept(this->fileDescriptor, (struct sockaddr *) &this->socketLocal, &addrLen);
                VLOG(1) << "Connection established to: " << int(this->socketLocal.sin_addr.s_addr & 0xFF)
                        << "." << int((this->socketLocal.sin_addr.s_addr & 0xFF00) >> 8)
                        << "." << int((this->socketLocal.sin_addr.s_addr & 0xFF0000) >> 16)
                        << "." << int((this->socketLocal.sin_addr.s_addr & 0xFF000000) >> 24)
                        << ":" << this->socketLocal.sin_port;
                VLOG(2) <<
                        "We got a connection. Closing the old file descriptor and pointing this object to the new socket";
                close(this->fileDescriptor);
                this->fileDescriptor = fd;
                VLOG(2) << "[DONE]";
            }
        }

        return 0;
    }

    int Connection::initiateConnection()
    //Connects to a socket. Input parameter is the IP address to be formatted.
    //Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned
    {

        VLOG(1) << "Attempting to connect...";
        int successStatus = connect(this->fileDescriptor, (struct sockaddr *) &this->socketRemote, sizeof(socketRemote));
        if (successStatus == -1)
        {
            VLOG(1) << "Failed to initiate connection";
            return 0;
        } else
        {
            VLOG(1) << "Sucessfully initiated connection";
            return 1;
        }
    }

    int Connection::terminateConnection()
    //Causes connection on the socket associated with first input parameter to be shut down. Further receptions and transmissions will be disallowed.
    //Second input parameter is defaulted to SHUT_RDWR to stop all further receptions and transmissions instead of just receptions or just transmissions
    //On success, zero is returned.  On error, -1 is returned
    {
        int successStatus = shutdown(this->fileDescriptor, SHUT_RDWR);
        return successStatus;
    }

    void Connection::makeStream(NetworkChunk *chunk)
    {
        VLOG(3) << "Chunk is of length: " << chunk->getLength();
        if (chunk->getLength() < CBDATA_DATALENGTH)
        {
            this->sendDataUnsegmented(chunk);

        }else{
            this->sendDataSegmented(chunk);
        }
        return;
    }

    void Connection::sendDataSegmented(NetworkChunk *chunk)
    {
        VLOG(3) << "Segmenting data";

        double chunkLength = chunk->getLength();
        double numSegments = ceil(chunkLength/CBDATA_DATALENGTH); //Determine number of segments

        //create cbHeader -> turn into NC -> send NC -> delete cbHeader
        CbHeader *cbHeader = new CbHeader();  //create cbHeader

        cbHeader->setUID(this->currUID);
        cbHeader->setNumBytes(chunkLength);
        cbHeader->setNumSegments(numSegments);

        VLOG(2) << "sending UID: " << cbHeader->getUID();
        VLOG(3) << "sending NumBytes: " << cbHeader->getNumBytes();
        VLOG(3) << "sending NumSegments: " << cbHeader->getNumSegments();

        NetworkChunk *transmitChunk = cbHeader->toNetworkChunk(); //turn header to NC
        VLOG(3) << "verifying UID: " << static_cast<int>((transmitChunk->getData())[0]);
        VLOG(3) << "verifying NumBytes: " << static_cast<int>(((transmitChunk->getData())[1]) << 8 | (transmitChunk->getData())[2]);
        VLOG(3) << "verifying NumSegments: " << static_cast<int>(((transmitChunk->getData())[3]) << 8 | (transmitChunk->getData())[4]);

        int cbHeaderSent = this->sendDataUnsegmented(transmitChunk); //send NC

        delete cbHeader;
        delete transmitChunk;

        if (cbHeaderSent != -1){
            //create cbData -> turn into NC -> send NC -> delete cbData
            int cbDataSent;
            for (int i = 0; i < numSegments; i++)
            {
                CbData *cbData = new CbData();

                cbData->setUID(this->currUID);
                cbData->setIndex(i);
                cbData->setData((chunk->getData())+CBDATA_DATALENGTH*i); //i can write over the 6 before it except for the first one. so that one needs to be set back 6

                VLOG(3) << "sending CbData UID: " << cbData->getUID();
                VLOG(3) << "sending index: " << cbData->getIndex();

                NetworkChunk *transmitChunk = cbData->toNetworkChunk();
                VLOG(3) << "verifying CbData UID: " << static_cast<int>((transmitChunk->getData())[0]);
                VLOG(3) << "verifying index: " << static_cast<int>(((transmitChunk->getData())[1]) << 8 | (transmitChunk->getData())[2]);

                cbDataSent = this->sendDataUnsegmented(transmitChunk);

                delete transmitChunk;
                delete cbData;
                if (cbDataSent == -1){break;}
            }
        }

        if (this->currUID == MAX_UID){
            this->currUID = 0;
        }else{
            (this->currUID)++;
        }

        return;
    }

    int Connection::sendDataUnsegmented(NetworkChunk *chunk)
    //Upon successful completion, send() shall return the number of bytes sent. Otherwise, -1 shall be returned
    //The first input parameter is a pointer to the buffer where the message to be transmitted is stored.
    //The second input parameter is the message length.
    {
        VLOG(3) << "Sending one segment of data";
        int lengthToSend = RECEIVE_HEADER_LENGTH+RECEIVE_TYPELENGTH_LENGTH+chunk->getLength();

        //just for debugging
        if (chunk->getDataType() == DataType::CBHEADER){
            VLOG(3) << "Sending CbHeader";
        }else if(chunk->getDataType() == DataType::CBDATA){
            VLOG(3) << "Sending CbData";
        }

        chunk->setData(chunk->getData()-(RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH)); //change pointer back 3 (to location originally allocated)
        (chunk->getData())[0] = receiveHeaderValue[0];
        (chunk->getData())[1] = (static_cast<int>(chunk->getDataType()) << 4 | (chunk->getLength() >> 16));
        (chunk->getData())[2] = (chunk->getLength() >> 8); //lsb of length
        (chunk->getData())[3] = (chunk->getLength() & 0xff); //was working when this was 2?

        int bytesSent = this->sendBitStream((chunk->getData()),lengthToSend);

        VLOG(3) << "sent " << bytesSent << " bytes";
        return bytesSent;
    }

    int Connection::sendBitStream(char *bitStream, int length)
    {
        ssize_t bytesSent;
        socklen_t slen = sizeof(this->socketRemote);
        int timesTried = 0;

//        std::fstream csvFile;
//        csvFile.open ("/home/debian/rover/sendLoops.csv", std::fstream::in | std::fstream::out | std::fstream::app);

        switch (this->protocol)
        {
            case ConnectionProtocol::TCP:
                bytesSent = send(this->fileDescriptor, bitStream, length, 0);
                break;
            case ConnectionProtocol::UDP:
//                for (int i = 0; i < 16000; i++)
//                {
                do{
                    bytesSent = sendto(this->fileDescriptor, bitStream, length, 0, (struct sockaddr *) &this->socketRemote, slen);
                    timesTried++;
//                }
                }while(bytesSent == -1);
                VLOG(3) <<"Tried to call sento function " << timesTried << " times";
//                csvFile << timesTried << "\r\n";
//                csvFile.close();

                VLOG(3) << "Sending length: " << length;

                break;
        }
        return bytesSent;
    }

    void Connection::processDataOnBuffer()
    //function takes everything that is on the buffer and puts it in a queue of NetworkChunk*'s. It then returns the oldest
    //to be processed
    {
        ReceiveType typeReceived;
        int receivedCount = 0;
        do {
            VLOG(3) << "Receiving chunk off buffer";
            NetworkChunk *receivedChunk = new NetworkChunk();
            typeReceived = this->receiveChunk(receivedChunk);

            switch (typeReceived)
            {
                case ReceiveType::NETWORKCHUNK:
                    VLOG(3) << "Received NetworkChunk. Pushing into queue";
                    this->chunkQueue.push(receivedChunk);
                    break;
                case ReceiveType::SEGMENT:
                {
                    VLOG(2) << "Received CbHeader or CbData SEGMENT";
                    receivedChunk->setData(receivedChunk->getData() - (RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH));
                    delete receivedChunk; //should be data to delete filled in then turned into CBheader
                    break;
                }
                case ReceiveType::NODATA:
                {
                    VLOG(3) << "No data received to push into queue";
                    receivedChunk->deallocateArray = 0;
                    delete receivedChunk;
                    break;
                }
            }
            receivedCount++;
//        } while (typeReceived != ReceiveType::NODATA);//TODO - we get stuck in this loop
        } while (receivedCount < 1);
    }

    int Connection::getReceivedData(char **buffer, int length)
    {
        int bytesReceived;
        VLOG(3) << "Getting received data";
        switch (this->protocol)
        {
            case ConnectionProtocol::TCP:
                bytesReceived = this->receive(*buffer, length);
                break;
            case ConnectionProtocol::UDP:
                *buffer = this->udpData + this->udpReadPosition;

                VLOG(3) << "Reading from udpData array from position: " << this->udpReadPosition;
                this->udpReadPosition += length;
                bytesReceived = length;
                break;
        }
        return bytesReceived;
    }

    ReceiveType Connection::receiveChunk(NetworkChunk *receivedChunk)
    //Upon successful completion, recv() shall return the length of the message in bytes. If no messages are available to be
    //received and the peer has performed an orderly shutdown, recv() shall return 0. Otherwise, -1 shall be returned to indicate error.
    //Last input argument for recv = 0 to indicate no flags
    {
        VLOG(3) << "Receiving chunk";
        int bytesReceived;
        if(this->protocol == ConnectionProtocol::UDP)
        {
            int length = RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH + CBDATA_INFOLENGTH + CBDATA_DATALENGTH;
            this->udpReadPosition = 0;
            char *tempData = new char[length];
            if(this->receive(tempData, length) == -1){return ReceiveType::NODATA;} //take fixed length UDP packet off buffer
            this->udpData = tempData;
        }
        if(this->checkDataHeader())
        {
            char typeLengthInfo[RECEIVE_TYPELENGTH_LENGTH];
            char* typeLengthInfoPtr = typeLengthInfo;
            bytesReceived = this->getReceivedData(&typeLengthInfoPtr, RECEIVE_TYPELENGTH_LENGTH);
            VLOG(3) << "typeLengthInfo[0]="<<static_cast<int>(typeLengthInfoPtr[0]);
            VLOG(3) << "typeLengthInfo[1]="<<static_cast<int>(typeLengthInfoPtr[1]);
            VLOG(3) << "Bytes received " << bytesReceived;

            DataType dataType = static_cast<DataType>(typeLengthInfoPtr[0] >> 4);//typecast into dataType
            int length = (static_cast<int>(static_cast<unsigned char>(typeLengthInfoPtr[0] & 0x0f)) << 16) | (static_cast<int>(static_cast<unsigned char>(typeLengthInfoPtr[1])) << 8) | static_cast<int>(static_cast<unsigned char>(typeLengthInfoPtr[2]));
            VLOG(3) << "length: " << length;

            char *receiveBuffer = new char[RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH + length];
            receiveBuffer += (RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH); //pointer is placed 3 from the start, but space allocated 3 behind it.
            bytesReceived = this->getReceivedData(&receiveBuffer, length);
            VLOG(3) << "receiveBuffer[0]="<<static_cast<int>(receiveBuffer[0]);
            VLOG(3) << "receiveBuffer[1]="<<static_cast<int>(receiveBuffer[1]);
            VLOG(3) << "receiveBuffer[2]="<<static_cast<int>(receiveBuffer[2]);
            VLOG(3) << "receiveBuffer[3]="<<static_cast<int>(receiveBuffer[3]);

            receivedChunk->setDataType(dataType);
            receivedChunk->setLength(length);

            receivedChunk->setData(receiveBuffer);

            //fill in networkChunk or create chunkBox(CbHeader) or fill in chunkBox(CbData)
            ReceiveType typeReceived;
            switch(dataType)
            {
                case DataType::COMMAND:
                case DataType::STATUS:
                case DataType::CAMERA:
                case DataType::TEXT:
                    switch(dataType)
                    {
                        case DataType::COMMAND:
                            switch(static_cast<CommandType>((receiveBuffer[0] & 0xf0) >> 4))
                            {
                                case CommandType::DRIVE_FORWARD:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: DRIVE_FORWARD -> Length:" << length;
                                    break;
                                case CommandType::DRIVE_BACKWARD:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: DRIVE_BACKWARD -> Length:" << length;
                                    break;
                                case CommandType::TURN_LEFT:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: TURN_LEFT -> Length:" << length;
                                    break;
                                case CommandType::TURN_RIGHT:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: TURN_RIGHT -> Length:" << length;
                                    break;
                                case CommandType::STOP_DRIVE:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: STOP_DRIVE -> Length:" << length;
                                    break;
                                case CommandType::START_STREAM:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: START_STREAM -> Length:" << length;
                                    break;
                                case CommandType::DISPENSE_TREAT:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: DISPENSE_TREAT -> Length:" << length;
                                    break;
                                case CommandType::FLIP_CAMPERA:
                                    VLOG(2) << "NetworkChunk received-> DataType: COMMAND -> CommandType: FLIP_CAMPERA -> Length:" << length;
                                    break;
                            }
                            break;
                        case DataType::STATUS:
                            VLOG(2) << "NetworkChunk received-> DataType: STATUS Length:" << length;
                            break;
                        case DataType::CAMERA:
                            VLOG(2) << "NetworkChunk received-> DataType: CAMERA Length:" << length;
                            break;
                        case DataType::TEXT:
                            VLOG(2) << "NetworkChunk received-> DataType: TEXT Length:" << length;
                            break;
                    }
                    typeReceived = ReceiveType::NETWORKCHUNK;
                    break;
                case DataType::CBHEADER:
                {
                    VLOG(2) << "Chunk received-> DataType: CBHEADER Length:" << length;
                    CbHeader *cbHeader = new CbHeader(receivedChunk); //pass CbHeader a networkChunk to create the header
                    ChunkBox *chunkBox = new ChunkBox(cbHeader);

                    if (this->chunkAccumulator.count(cbHeader->getUID()) > 0) //Entry with this UID already exists - must delete before inserting it
                    {
                        delete this->chunkAccumulator[cbHeader->getUID()]; //delete the chunkBox stored here
                        this->chunkAccumulator.erase(cbHeader->getUID());
                    }
                    this->chunkAccumulator.insert({(cbHeader->getUID()), chunkBox});

                    typeReceived = ReceiveType::SEGMENT;

                    //just for test - print number of segments received in previous chunkBox
                    if (cbHeader->getUID() > 0)
                    {
                        if ((this->chunkAccumulator[cbHeader->getUID()-1]) != nullptr)
                        {
                            VLOG(2) << (this->chunkAccumulator[cbHeader->getUID()-1])->segmentsReceived << " segments";
                        }
                    }
                }
                    break;
                case DataType::CBDATA:
                {
                    VLOG(2) << "Chunk received-> DataType: CBDATA Length:" << length;

                    CbData *cbData = new CbData(receivedChunk); //make a new CbData and fill it in via NetworkChunk
                    VLOG(2) << "After create new CbData";
                    ChunkBox *chunkBox = this->chunkAccumulator[cbData->getUID()]; //determine which chunkbox it should be put in based on UID
                    VLOG(2) << " After create new chunkbox";
                    if (chunkBox == nullptr)
                    {
                        VLOG(2) << "ChunkBox is nullptr. cbHeader not received";
                    }else{
                        if(!chunkBox->getIsFull())
                        {
                            chunkBox->add(cbData); //add data to chunkBox
                        }
                        chunkBox->segmentsReceived++; //just for test to see how many are received
                    }
                    typeReceived = ReceiveType::SEGMENT;
                }
                    break;
            }

            return typeReceived; //data received
        }else{
            VLOG(3) << "Received data has incorrect header";
            return ReceiveType::NODATA; //no data received
        }
    }

    void Connection::processDataInMap()
    {
        VLOG(3) << "Processing data in map";
        //iterates through each chunkBox in the map
        for ( auto it = this->chunkAccumulator.begin(); it != this->chunkAccumulator.end(); ++it )
        {
            if((it->second)!=nullptr){
                int filled = (it->second)->getSegmentsFilled();
                int total = (it->second)->getTotalSegments();
                if(!(it->second->getIsFull()) & ((filled*100)/total >= CHUNKBOX_FULL_PERCENT))
                {
                    NetworkChunk *processedChunk = new NetworkChunk();
                    processedChunk->setDataType(DataType::CAMERA); //TODO - CBHEADER needs to keep track of what type of NC it should turn into - for now, likely camera
                    processedChunk->setLength((it->second->getTotalBytes()));
                    processedChunk->setData((it->second->getData()));
                    VLOG(3) << filled << " segments filled. Putting into NetworkChunk";

                    this->chunkQueue.push(processedChunk);
                    it->second->setIsFull(1);
                }
            }
        }
        return;
    }

    ReceiveType Connection::popChunkFromQueue(NetworkChunk* chunk)
    {
        VLOG(3) << "Attempting to pop chunk from queue";
        if (!this->chunkQueue.empty())//if there's something in the queue (it's not empty)
        {
            NetworkChunk* tempChunk;
            tempChunk = this->chunkQueue.front();    //access the oldest chunk in the queue
            *chunk = *tempChunk;

            tempChunk->setData(tempChunk->getData()-(RECEIVE_HEADER_LENGTH + RECEIVE_TYPELENGTH_LENGTH));
            tempChunk->deallocateArray = 0;
            delete tempChunk;

            this->chunkQueue.pop();//delete the oldest chunk from the queue
            return ReceiveType::NETWORKCHUNK;
        } else{
            VLOG(3) << "No data in queue to pop"; //don't delete chunk. ReceiveType indicates chunk not filled - chunk reused each loop.
            return ReceiveType::NODATA;
        }
    }

    int Connection::checkDataHeader()
    {
        char* header = new char[RECEIVE_HEADER_LENGTH];
        VLOG(3) << "Checking data header";
        int bytesReceived = getReceivedData(&header, RECEIVE_HEADER_LENGTH);

        if (bytesReceived > 0)
        {
            VLOG(3) << "header[0]="<<static_cast<int>(header[0]);
            for (int i=0;i<(RECEIVE_HEADER_LENGTH);i++)//minus 1 because last byte is the one carrying length/type info
            {
                if(header[i] != receiveHeaderValue[i])//check that the header we received = header expected
                {
                    VLOG(1) << "Received data, but it's the incorrect header";
                    if(this->protocol == ConnectionProtocol::TCP){delete[] header;}
                    return 0;
                }
            }
            VLOG(3) << "Correct data header";
            if(this->protocol == ConnectionProtocol::TCP){delete[] header;}
            return 1;
        }else {
            if(this->protocol == ConnectionProtocol::TCP){delete[] header;}
            return 0;
        }
    }

    int Connection::receive(char* buffer, int length)
    {
        int bytesReceived = 0;
        socklen_t slen = sizeof(this->socketRemote);

        switch (this->protocol)
        {
            case ConnectionProtocol::TCP:
                bytesReceived = recv(this->fileDescriptor, buffer, length, 0);
                if (bytesReceived != -1)
                {
                    VLOG_EVERY_N(100,1) << "GOT TCP DATA!";
                }
                break;
            case ConnectionProtocol::UDP:
                VLOG(3) << "Trying to receive " << length << " bytes";
                do
                {
                    bytesReceived = recvfrom(this->fileDescriptor, buffer, length, 0, (struct sockaddr *) &this->socketRemote, &slen);
                }while(bytesReceived != length);
                if(bytesReceived == -1){perror("recvfrom()");}
                break;
        }

        return bytesReceived;
    }
}
