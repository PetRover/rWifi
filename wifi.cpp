//
// Created by Bryce Carter on 8/25/15.
//

#include "rWifi.h"

#define IPV4_LENGTH 12

namespace RVR
{

// ==============================================================
// Connection Class Member functions
// ==============================================================

    int Connection::createEndpoint()
    //Socket creates endpoint for communication. Parameters determine following features: Domain of AF_INET indicates IPv4,
    //Type of SOCK_STREAM indicates sequenced, reliable, two-way, connection- based byte streams. 0 for protocol parameter
    //indicates using the default protocol for the domain specified will be used
    //On success, a file descriptor for the new socket is returned.  On error, -1 is returned
    {
        int fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
        return fileDescriptor;
    }

    int Connection::initiateConnection(int fileDescriptor, const struct sockaddr* ipAddress)
    //Connects to a socket. First input parameter is the file descriptor for the socket returned by createEndpoint
    //Second input parameter is the structure containing the peer address. Third input parameter is the length of the structure
    //pointed to by the second parameter (default to 12 for IPv4)
    //Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned
    {
        int successStatus = connect(fileDescriptor, ipAddress, IPV4_LENGTH);
        return successStatus;
    }

    int Connection::terminateConnection(int fileDescriptor)
    //First input parameter is the file descriptor for the socket returned by createEndpoint
    //Causes connection on the socket associated with first input parameter to be shut down. Further receptions and transmissions will be disallowed.
    //Second input parameter is defaulted to SHUT_RDWR to stop all further receptions and transmissions instead of just receptions or just transmissions
    //On success, zero is returned.  On error, -1 is returned
    {
        int successStatus = shutdown(fileDescriptor, SHUT_RDWR);
        return successStatus;
    }
}