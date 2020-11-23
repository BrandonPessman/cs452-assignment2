#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PACKET_MAX_SIZE 5
#define TOTAL_PACKET_MAX_SIZE 5

using namespace std;

/*
 * p0 is 10.35.195.251
 * p1 is 10.34.40.33
 * p2 is 10.35.195.250
 * p3 is 10.35.195.236
 */

void xorPacket(char *packet, char key[], int packetSize)
{
    // Encrypt or decrypt
    for (int i = 0; i < packetSize; i++)
    {
        packet[i + PACKET_MAX_SIZE] = packet[i + PACKET_MAX_SIZE] ^ key[i % strlen(key)];
    }
}

void printPacket(char packet[], int index, char type, int packetSize)
{
    // Sent packet
    if (type == 's')
    {
        cout << "Sent encrypted packet #" << index << " - encrypted as ";
    }

    // Recieve packet
    if (type == 'r')
    {
        cout << "Rec encrypted packet #" << index << " - encrypted as ";
    }

    // Printing
    printf("%02X\0", packet[0 + PACKET_MAX_SIZE]);
    printf("%02X\0", packet[1 + PACKET_MAX_SIZE]);
    cout << " ... ";
    printf("%02X\0", packet[packetSize + PACKET_MAX_SIZE - 2]);
    printf("%02X\0", packet[packetSize + PACKET_MAX_SIZE - 1]);
    cout << endl;
}

void packetToString(char packet[])
{
    cout << "[Packet Data]: " << packet << endl;
}

int main()
{
    char mode[1];

    cout << "Type 'c' for client and 's' for server: ";
    cin >> mode;

    // Client Mode
    if (mode[0] == 'c')
    {
        // Client Settings
        struct sockaddr_in client_addr;

        // Initialize variables
        char ip[20];
        char port[20];
        char sendFile[20];
        int packetSize = 20;
        char encryptKey[20];
        int totalPackets = 0;
        int leftOverPacket = 0;
        int numPackets = 0;

        // Gather information
        cout << "Connect to IP address: ";
        cin >> ip;
        cout << "Port #: ";
        cin >> port;
        cout << "File to be sent: ";
        cin >> sendFile;
        cout << "Pkt size: ";
        cin >> packetSize;
        cout << "Enter encryption key: ";
        cin >> encryptKey;

        // Client address initialization
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(atoi(port));
        client_addr.sin_addr.s_addr = inet_addr(ip);
        inet_pton(AF_INET, ip, &client_addr.sin_addr);

        // Open the socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("Socket Creation");
            return 0;
        }

        // Connect socket
        if (connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
        {
            perror("Connect");
            return 0;
        }

        // Get File
        FILE *pFile = fopen(sendFile, "r");
        fseek(pFile, 0, SEEK_END);
        long fileSize = ftell(pFile);
        rewind(pFile);

        // Calculate Packets from file size
        while (fileSize >= packetSize)
        {
            fileSize -= packetSize;
            totalPackets++;
        }

        // Adds an extra packet if there is less than a packet
        if (fileSize != 0)
        {
            leftOverPacket = fileSize;
            totalPackets++;
        }

        // Init Packet with max packet byte header
        char packet[packetSize + PACKET_MAX_SIZE];

        // Convert t to max packet byte number and add to packet
        char packetSizeToSend[PACKET_MAX_SIZE + sizeof(char)];
        sprintf(packetSizeToSend, "%d", packetSize);

        // Send Size
        write(sockfd, packetSizeToSend, PACKET_MAX_SIZE);

        // Send Total
        char totalPacketSizeToSend[TOTAL_PACKET_MAX_SIZE + sizeof(char)];
        sprintf(totalPacketSizeToSend, "%d", totalPackets);
        write(sockfd, totalPacketSizeToSend, TOTAL_PACKET_MAX_SIZE);

        for (int i = 0; i < totalPackets; i++)
        {
            int t = packetSize;
            if (i == totalPackets - 1 && leftOverPacket != 0)
            {
                t = leftOverPacket;
            }
            for (int j = 0; j < t; j++)
            {
                packet[j + PACKET_MAX_SIZE] = (char)fgetc(pFile);
            }

            // Convert t to max packet byte number and add to packet
            char sizeToSend[PACKET_MAX_SIZE + sizeof(char)];
            sprintf(sizeToSend, "%d", t);

            for (int i = 0; i < PACKET_MAX_SIZE; i++)
            {
                packet[i] = sizeToSend[i];
            }

            // Encrypt Packet
            xorPacket(packet, encryptKey, t);

            // Print Packet
            if (numPackets == 0 || numPackets == 1 || numPackets == totalPackets - 2 || numPackets == totalPackets - 1)
            {
                printPacket(packet, numPackets, 's', t);
            }

            // Write Packet
            write(sockfd, packet, t + PACKET_MAX_SIZE);

            numPackets++;
            bzero(packet, packetSize + PACKET_MAX_SIZE);
        }

        cout << "Send Success!" << endl;
        //write(sockfd, "", 0);

        // MD5 Hash
        cout << "MD5: " << endl;
        char sys[200] = "md5sum ";
        system(strcat(sys, sendFile));

        // Close the Socket
        close(sockfd);
    }

    // Server Mode
    if (mode[0] == 's')
    {
        // Server Settings
        struct sockaddr_in serv_addr, client_addr;
        int addrlen = sizeof(client_addr);

        // Initialize variables
        char ip[20];
        char port[20];
        char saveFile[100];
        char encryptKey[100];
        int maxPacketSize = 1000;
        int leftOverPacket = 0;
        int valread;
        int numPackets = 0;
        int totalPackets = 0;

        // Gather information
        cout << "Connect to IP address: ";
        cin >> ip;
        cout << "Port #: ";
        cin >> port;
        cout << "Save file to: ";
        cin >> saveFile;
        cout << "Enter encryption key: ";
        cin >> encryptKey;

        // Server address initialization
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(atoi(port));

        // Open the socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        // Bind the socket
        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0)
        {
            perror("Bind Failed");
            return 0;
        }

        // Set listen to 5 queued connections
        if (listen(sockfd, 5) < 0)
        {
            perror("Listen Failed");
            return 0;
        }

        // Accept a client connection
        int client_sock = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
        if (client_sock < 0)
        {
            perror("Accept Failed");
            return 0;
        }

        // Open a file
        FILE *pFile;
        pFile = fopen(saveFile, "w");

        // Get Packet Size
        char data_packetSize[PACKET_MAX_SIZE];
        read(client_sock, data_packetSize, PACKET_MAX_SIZE);
        maxPacketSize = atoi(data_packetSize);

        // Get total Packets
        char data_totalPackets[TOTAL_PACKET_MAX_SIZE];
        read(client_sock, data_totalPackets, TOTAL_PACKET_MAX_SIZE);
        totalPackets = atoi(data_totalPackets);

        char packet[maxPacketSize + PACKET_MAX_SIZE];
        bzero(packet, maxPacketSize + PACKET_MAX_SIZE);
        // Read all the packets
        while ((valread = read(client_sock, packet, maxPacketSize + PACKET_MAX_SIZE)) > 0)
        {
            // Get Size from Header
            char packetWriteSize[PACKET_MAX_SIZE];
            for (int i = 0; i < PACKET_MAX_SIZE; i++)
            {
                packetWriteSize[i] = packet[i];
            }
            int sz = atoi(packetWriteSize);

            // Print Packet
            if (numPackets == 0 || numPackets == 1 || numPackets == totalPackets - 2 || numPackets == totalPackets - 1)
            {
                printPacket(packet, numPackets, 'r', sz);
            }

            // Decrypt the Packet
            xorPacket(packet, encryptKey, sz);

            char packetWrite[sz];
            for (int i = 0; i < sz; i++)
            {
                packetWrite[i] = packet[i + PACKET_MAX_SIZE];
            }
            fwrite(packetWrite, 1, sz, pFile);

            numPackets++;
            bzero(packet, maxPacketSize + PACKET_MAX_SIZE);
        }

        // Recieve Success
        cout << "Recieve Success!" << endl;

        // Close file
        fclose(pFile);

        // MD5 Hash
        cout << "MD5: " << endl;
        char sys[200] = "md5sum ";
        system(strcat(sys, saveFile));

        // Close socket
        close(client_sock);
    }

    return 0;
}
