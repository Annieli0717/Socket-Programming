#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>

using namespace std;

/**
 * serverA.cpp
 * A storage server, possesses a database file database.txt
 * in which attribute values regarding information of links
 * are stored
 *
*/

/**
 * Named Constants
 */
#define LOCAL_HOST "127.0.0.1" // Host address
#define ServerA_UDP_PORT 21096 // Server A port number
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define FAIL -1
#define MAX_FILE_SIZE 1000

/**
 * Defined global variables
 */
string operation;
// string link_id, size, signal_power, bandwidth, length, velocity, noise_power; // Floating point input data

int sockfd_serverA_UDP; // Server A datagram socket
struct sockaddr_in serverA_addr, aws_addr; // AWS address as a server & as a client

fstream database;
int link_num = 0;

char recv_buf[MAXDATASIZE]; // Data sent by client: write / compute
char write_result[MAXDATASIZE]; // Get from server A Send to AWS



/**
 * Defined functions
 */

// 1. Create UDP socket
void create_serverA_socket();

// 2. Initialize connection with AWS server
void init_AWS_connection();

// 3. Bind a socket
void bind_socket();

// 4. Receive data from AWS

// 5. Write the data into database (database.txt)

// 6. Search request data and send result to AWS server

/**
 * Step 1: Create server A UDP sockect
 */
void create_serverA_socket() {
    sockfd_serverA_UDP = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP socket
    if (sockfd_serverA_UDP == FAIL) {
        perror("[ERROR] server A: can not open socket");
        exit(1);
    }
}

/**
 * Step 2: Create sockaddr_in struct
 */

void init_AWS_connection() {

    // Server A side information
    // Initialize server A IP address, port number
    memset(&serverA_addr, 0, sizeof(serverA_addr)); //  make sure the struct is empty
    serverA_addr.sin_family = AF_INET; // Use IPv4 address family
    serverA_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    serverA_addr.sin_port = htons(ServerA_UDP_PORT); // Server A port number
}


/**
 * Step 3: Bind socket with specified IP address and port number
 */
void bind_socket() {
    if (::bind(sockfd_serverA_UDP, (struct sockaddr *) &serverA_addr, sizeof(serverA_addr)) == FAIL) {
        perror("[ERROR] Server A: fail to bind AWS UDP socket");
        exit(1);
    }

    printf("The Server A is up and running using UDP on port <%d>. \n", ServerA_UDP_PORT);
}

int main() {

    /******    Step 1: Create server A socket (UDP)  ******/
    create_serverA_socket();
    /******    Step 2: Create sockaddr_in struct  ******/
    init_AWS_connection();
    /******    Step 3: Bind socket with specified IP address and port number  ******/
    bind_socket();

    // Part of codes is from http://c.biancheng.net
    while (true) {

        /******    Step 4: Receive data from AWS  ******/
        socklen_t aws_addr_size = sizeof(aws_addr);
        if (::recvfrom(sockfd_serverA_UDP, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &aws_addr,
                       &aws_addr_size) == FAIL) {
            perror("[ERROR] Server A: fail to receive data from AWS server");
            exit(1);
        }

        operation = strtok(recv_buf, " ");

        /********************************************************************************************************/
        /*****************************************   Case 1: Write data     *************************************/
        /********************************************************************************************************/

        if (operation == "write") {

            string bandwidth = strtok(NULL, " ");
            string length = strtok(NULL, " ");
            string velocity = strtok(NULL, " ");
            string noise_power = strtok(NULL, " ");

             cout << "bandwidth is " << bandwidth << endl;
             cout << "length is " << length << endl;
             cout << "velocity is " << velocity << endl;
             cout << "noise_power is " << noise_power << endl;

            printf("The Server A received input for writing \n");

            // Write data into database.txt
            FILE *fp = fopen("database.txt", "a");
            if (fp == NULL) {
                perror("[ERROR] Server A: database.txt not found");
                exit(1);
            }

            // Write data into database.txt
            link_num += 1; // Each time write new data, link number will increase by 1
            stringstream ss;
            ss << link_num;
            string str = ss.str() + " " + bandwidth + " " + length + " " + velocity + " " + noise_power + " " + "\n";
            /*char file_name[MAX_FILE_SIZE + 1];
            bzero(file_name, MAX_FILE_SIZE + 1);
            strncpy(file_name, str.c_str(), strlen(file_name));*/
            fputs(str.c_str(), fp);

            string result = "1";
            strncpy(write_result, result.c_str(), 1);
            socklen_t aws_addr_size = sizeof(aws_addr);
            if (sendto(sockfd_serverA_UDP, write_result, MAXDATASIZE, 0, (struct sockaddr *) &aws_addr,
                       aws_addr_size) < 0) {
                perror("[ERROR] Server A: fail to send write result to AWS server");
                exit(1);
            }
            fclose(fp);
        }

            /********************************************************************************************************/
            /***************************************   Case 2: Compute data     *************************************/
            /********************************************************************************************************/
        else if (operation == "compute") {

            string link_id = strtok(NULL, " ");
            string size = strtok(NULL, " ");
            string signal_power = strtok(NULL, " ");
            printf("The Server A received input <%s> for computing \n", link_id.c_str());

            /*cout << "Link ID is " << link_id << endl;
            cout << "Size is " << size << endl;
            cout << "Signal power is " << signal_power << endl;*/

            ifstream database;
            database.open("database.txt");

            // If database file does not exit, report an error
            if (!database) {
                perror("[ERROR] Server A: database.txt not found");
                exit(1);
            }
            char temp_buf[MAXDATASIZE];

            // http://c.biancheng.net
            while (true) {
                memset(temp_buf, 0, MAXDATASIZE);
                database.getline(temp_buf, sizeof(database));
                // << "temp buf is " << temp_buf << endl;

                string link_in_database;
                if (temp_buf[0] != 0) {
                    link_in_database = strtok(temp_buf, " ");
                }
                //cout << "link in database is " << link_in_database << endl;

                if (link_in_database == link_id) {

                    string bandwidth = strtok(NULL, " ");
                    string length = strtok(NULL, " ");
                    string velocity = strtok(NULL, " ");
                    string noise_power = strtok(NULL, " ");

                    /*cout << "Bandwidth is " << bandwidth << endl;
                    cout << "Length is " << length << endl;
                    cout << "Velocity is " << velocity << endl;
                    cout << "noise power is " << noise_power << endl;*/

                    string str = link_id + " " + size + " " + signal_power + " " + bandwidth + " " + length + " " +
                                 velocity + " " + noise_power + " ";

                    char compute_result[sizeof(str) + 1];

                    strncpy(compute_result, str.c_str(), strlen(str.c_str()));

                    socklen_t aws_addr_size = sizeof(aws_addr);
                    if (sendto(sockfd_serverA_UDP, compute_result, MAXDATASIZE, 0,
                               (struct sockaddr *) &aws_addr,
                               aws_addr_size) < 0) {
                        perror("[ERROR] Server A: fail to send computed data to AWS server");
                        exit(1);
                    }
                    //cout << "Data sent from server A " << compute_result << endl;
                    printf("The Server A finished sending the search result to AWS \n");

                    memset(temp_buf, 0, MAXDATASIZE);
                    database.close();
                    break;

                }

                // If requested link is not found, return 'f' to AWS
                if (database.eof()) {
                    char compute_result[] = {'f'};
                    if (sendto(sockfd_serverA_UDP, compute_result, sizeof(compute_result), 0,
                               (struct sockaddr *) &aws_addr,
                               aws_addr_size) < 0) {
                        perror("[ERROR] Server A: fail to send computed data to AWS server");
                        exit(1);
                    }
                    printf("Link ID not found \n");
                    database.close();
                    break;
                }
                memset(temp_buf, 0, sizeof(temp_buf));
            }
        }
    }
    close(sockfd_serverA_UDP);
    return 0;
}

