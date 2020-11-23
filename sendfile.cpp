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

void xorPacket(char packet[], char key[]) {
    // Calculate length of packet
    int length = strlen(packet);
    int keylength = strlen(key);

    // Encrypt or decrypt
    for (int i = 0; i < length; i++) {
        packet[i] = packet[i] ^ key[i % keylength];
    }
    packet[length] = '\0';

    //return packet;
}

void printPacket(char packet[], int index,  char type) {
    // Get size
    int length = strlen(packet);
    
    // Sent packet
    if (type == 's') {
        cout << "Sent encrypted packet #" << index << " - encrypted as ";
    }

    // Recieve packet
    if (type == 'r') {
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

void packetToString(char packet[]) {
    cout << "[Packet Data]: " << packet << "\0" << endl;
}

int main () {
    char mode[1];

    cout << "Type 'c' for client and 's' for server: ";
    cin >> mode;

    // Client Mode
    if (mode[0] == 'c') {
        // Client Settings
        struct sockaddr_in client_addr;

        // Initialize variables
        char ip[20];
        char port[20];
        char sendFile[20];
        int packetSize = 20;
        char encryptKey[20];

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
        client_addr.sin_port = htons(9611);
        client_addr.sin_addr.s_addr = inet_addr("10.35.195.250");
        inet_pton(AF_INET, "10.35.195.250", &client_addr.sin_addr);

        // Open the socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Socket Creation");
            return 0;
        }

        cout << "Socket Open Complete!" << endl;

        // Connect socket
        if (connect(sockfd,(struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
            perror("Connect");
            return 0;
        }

        cout << "Socket Connected!" << endl;

        // Get File
        FILE *pFile = fopen("pessman.txt", "r");
        fseek(pFile, 0, SEEK_END);
        long fileSize = ftell(pFile);
        cout << "File Size: " << fileSize << " bytes." << endl;
        rewind(pFile);
        
        cout << "================================" << endl;
        // Send packet size first
        cout << "Packet Size: " << packetSize << endl;
        write(sockfd, &packetSize, sizeof(packetSize));

        // Setup packets
        int totalPackets = 0;
        int leftOverPacket = 0;
        while(fileSize >= packetSize) {
            totalPackets++;
            fileSize -= packetSize;
        }

        if (fileSize != 0) {
            leftOverPacket = fileSize;
            totalPackets++;
        }

        char packet[totalPackets][packetSize];
        char lastPacket[leftOverPacket];

        char thekey[] = "thekey";
        //char *packet = new char[packetSize];
        //int i = 0;
        int numPackets = 0;
    

        // Send Total Packets
        write(sockfd, &totalPackets, sizeof(totalPackets));
        cout << "Total Packets: " << totalPackets << endl;

        // Send Final Packet Size
        //write(sockfd, &leftOverPacket, sizeof(leftOverPacket));
        //cout << "Packet Extra: " << leftOverPacket << endl;

        cout << "================================" << endl;

        for (int i = 0; i < totalPackets; i++) {
            for (int j = 0; j < packetSize; j++) {
                packet[i][j] = (char) fgetc(pFile);
            }
            
            // Encrypt Packet
            xorPacket(packet[i], thekey);

            // Print Packet
            printPacket(packet[i], numPackets, 's');

            // Write Packet
            write(sockfd, packet[i], packetSize);
            
            numPackets++;
        }

        if (leftOverPacket != 0) {
            for (int i = 0; i < leftOverPacket; i++) {
                lastPacket[i] = (char) fgetc(pFile);
            }
            lastPacket[leftOverPacket] = '\0';

            // Encrypt Packet
            xorPacket(lastPacket, thekey);
            
            // Print Packet
            printPacket(lastPacket, numPackets, 's');

            // Write Packet
            write(sockfd, lastPacket, leftOverPacket);

            numPackets++;
        }

/*    
        int c;
        while ((c = fgetc(pFile)) != EOF) {
            packet[i] = (char) c;
            i++;

            if (i == packetSize) {
                // Print string
                //packetToString(packet);

                // Encrypt Packet
                packet = xorPacket(packet, thekey);

                // Print packet
                printPacket(packet, numPackets,  's');

                // Decrypt Packet
                packet = xorPacket(packet, thekey);
                //printPacket(packet, numPackets, 's');
                packetToString(packet);
                // Send Packet
                //write(sockfd, packet, packetSize);
                
                // Reset
                i = 0;
                numPackets++;
            }
        }

        // Check for last packet
        if (i != 0) {
            //packetToString(packet);

            // Encrypt last packet
            packet = xorPacket(packet, thekey);

            // Print packet
            printPacket(packet, numPackets, 's');

            // Send Packet
            //write(sockfd, packet, packetSize);
        }
*/
        cout << "Send Success!" << endl;
        write(sockfd, "", 0);
        // MD5 Hash
        cout << "MD5: " << endl;
        system("md5sum pessman.txt");

        // Close the Socket
        close(sockfd);
    }

    // Server Mode
    if (mode[0] == 's') {
        // Server Settings
        int serve_sock = 9611;
        struct sockaddr_in serv_addr, client_addr;
        int addrlen = sizeof(client_addr);
    
        // Initialize variables
        char ip[20];
        char port[20];
        char saveFile[20];
        char encryptKey[20];

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
    
        cout << "Socket Open Complete!" << endl;

        // Bind the socket
        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
            perror("Bind Failed");
            return 0;
        }

        cout << "Binding Complete!" << endl;

        // Set listen to 5 queued connections
        if (listen(sockfd, 5) < 0) {
            perror("Listen Failed");
            return 0;
        }

        cout << "Listen Complete!" << endl;

        // Accept a client connection
        int client_sock = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen);
        if (client_sock < 0) {
            perror("Accept Failed");
            return 0;
        }

        int packetSize;
        int totalPackets;
        int leftOverPacket = 0;
    
        cout << "================================" << endl;
        // Get Packet Size
        read(client_sock, &packetSize, 100);
        cout << "Packet Size: " << packetSize << endl;

        // Get Total Packets
        read(client_sock, &totalPackets, 100);
        cout << "Total Packets: " << totalPackets << endl;

        // Get Packet Extra
        //read(client_sock, &leftOverPacket, 100);
        //cout << "Packet Extra: " << leftOverPacket << endl;

        cout << "================================" << endl;

        // Open a file
//       FILE *pFile;
//       pFile = fopen("pessman-write.txt", "w");

        // Get first packet
        char data[totalPackets+1][packetSize];
        int valread;
        int numPackets = 0;
        char thekey[] = "thekey";

        valread = read(client_sock, data[0], packetSize);
        
        // Print first packet
        printPacket(data[0], numPackets, 'r');

        // Decrypt First Packet
        xorPacket(data[0], thekey);

        // Write First Packet to File
//        fwrite(data, sizeof(char), sizeof(data), pFile);
        
        numPackets++;

        // Read all the packets
        while(totalPackets > 0) {
            // Get packet
            valread = read(client_sock, data[numPackets], packetSize);
            
            // Print Packet
            printPacket(data[numPackets], numPackets, 'r');

            // Decrypt the data
            xorPacket(data[numPackets], thekey);

            // Write to file
//            fwrite(data, sizeof(char), sizeof(data), pFile);

            numPackets++;       
            totalPackets--;
        }

        // Recieve Success
        cout << "Recieve Success!" << endl;

        // Close file
//      fclose(pFile);
        
        // MD5 Hash
        cout << "MD5: " << endl;
        system("md5sum pessman-write.txt");

        // Close socket
//        close(client_sock);
    }

    return 0;
}
