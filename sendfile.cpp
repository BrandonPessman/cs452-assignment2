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

using namespace std;

/*
 * p0 is 10.35.195.251
 * p1 is 10.34.40.33
 * p2 is 10.35.195.250
 * p3 is 10.35.195.236
 */

void xorPacket(char packet[], char key[])
{
    // Calculate length of packet
    int length = strlen(packet);

    // Encrypt or decrypt
    for (int i = 0; i < length; i++)
    {
        packet[i] = packet[i] ^ key[i % strlen(key)];
    }
}

void printPacket(char packet[], int index, char type)
{
    // Get size
    int length = strlen(packet);

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
    printf("%02X", packet[0]);
    printf("%02X", packet[1]);
    cout << " ... ";
    printf("%02X", packet[length - 2]);
    printf("%02X\0", packet[length - 1]);
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
        char thekey[] = "thekey";

        // Gather information
        //cout << "Connect to IP address: ";
        //cin >> ip;
        //cout << "Port #: ";
        //cin >> port;
        //cout << "File to be sent: ";
        //cin >> sendFile;
        //cout << "Pkt size: ";
        //cin >> packetSize;
        //cout << "Enter encryption key: ";
        //cin >> encryptKey;

        // Client address initialization
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(9271);
        client_addr.sin_addr.s_addr = inet_addr("10.35.195.250");
        inet_pton(AF_INET, "10.35.195.250", &client_addr.sin_addr);

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
        FILE *pFile = fopen("/tmp/2M", "r");
        fseek(pFile, 0, SEEK_END);
        long fileSize = ftell(pFile);
        cout << "File Size: " << fileSize << " bytes." << endl;
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

        char packet[totalPackets][packetSize];

        cout << "================================" << endl;
        // Send packet size first
        cout << "Packet Size: " << packetSize << endl;
        write(sockfd, &packetSize, sizeof(packetSize));

        // Send Total Packets
        write(sockfd, &totalPackets, sizeof(totalPackets));
        cout << "Total Packets: " << totalPackets << endl;
        cout << "================================" << endl;

        for (int i = 0; i < totalPackets; i++)
        {
            int t = packetSize;
            if (i == totalPackets - 1 && leftOverPacket != 0)
            {
                t = leftOverPacket;
            }
            for (int j = 0; j < t; j++)
            {
                packet[i][j] = (char)fgetc(pFile);
            }

            // Encrypt Packet
            xorPacket(packet[i], thekey);

            // Print Packet
            printPacket(packet[i], numPackets, 's');

            // Write Packet
            write(sockfd, packet[i], t);

            numPackets++;
        }

        cout << "Send Success!" << endl;
        //write(sockfd, "", 0);

        // MD5 Hash
        cout << "MD5: " << endl;
        system("md5sum /tmp/2M");

        // Close the Socket
        close(sockfd);
    }

    // Server Mode
    if (mode[0] == 's')
    {
        // Server Settings
        int serve_sock = 9271;
        struct sockaddr_in serv_addr, client_addr;
        int addrlen = sizeof(client_addr);

        // Initialize variables
        char ip[20];
        char port[20];
        char saveFile[20];
        char encryptKey[20];
        int packetSize;
        int totalPackets;
        int leftOverPacket = 0;
        int valread;
        int numPackets = 0;
        char thekey[] = "thekey";

        // Gather information
        //cout << "Connect to IP address: ";
        //cin >> ip;
        //cout << "Port #: ";
        //cin >> port;
        //cout << "Save file to: ";
        //cin >> saveFile;
        //cout << "Enter encryption key: ";
        //cin >> encryptKey;

        // Server address initialization
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(serve_sock);

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

        cout << "================================" << endl;
        // Get Packet Size
        read(client_sock, &packetSize, 100);
        cout << "Packet Size: " << packetSize << endl;

        // Get Total Packets
        read(client_sock, &totalPackets, 100);
        cout << "Total Packets: " << totalPackets << endl;
        cout << "================================" << endl;

        // Open a file
        FILE *pFile;
        pFile = fopen("/tmp/pessman-2M", "w");

        // Get first packet
        char packet[totalPackets + 1][packetSize];
        valread = read(client_sock, packet[0], packetSize);

        // Print first packet
        printPacket(packet[0], numPackets, 'r');

        // Decrypt First Packet
        xorPacket(packet[0], thekey);

        // Write First Packet to File
        fwrite(packet[0], 1, packetSize, pFile);

        numPackets++;

        // Read all the packets
        while (totalPackets > 0)
        {
            // Get packet
            valread = read(client_sock, packet[numPackets], packetSize);

            // Print Packet
            printPacket(packet[numPackets], numPackets, 'r');

            // Decrypt the Packet
            xorPacket(packet[numPackets], thekey);

            // Write to file
            fwrite(packet[numPackets], 1, packetSize, pFile);

            numPackets++;
            totalPackets--;
        }

        // Recieve Success
        cout << "Recieve Success!" << endl;

        // Close file
        fclose(pFile);

        // MD5 Hash
        cout << "MD5: " << endl;
        system("md5sum /tmp/pessman-2M");

        // Close socket
        close(client_sock);
    }

    return 0;
}
