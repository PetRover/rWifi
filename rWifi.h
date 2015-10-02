//
// Created by Bryce Carter on 8/25/15.
//

#ifndef FIRMWARE_WIFI_H
#define FIRMWARE_WIFI_H

#include <sys/types.h>
#include <sys/socket.h>

namespace RVR
{
    class NetworkManager
    {
    private:
    public:

    };

    class Connection
    // A connection object represents one socket
    {
    private:
    public:
        int createEndpoint();
        int initiateConnection(int fileDescriptor, const struct sockaddr* ipAddress);
        int terminateConnection(int fileDescriptor);
    };
}

#endif //FIRMWARE_WIFI_H
