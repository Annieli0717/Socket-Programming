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
#include <vector>

using namespace std;

/**
 * aws.cpp
 * Amazon Web Server
 *
*/

/**
 * Named Constants
 */

#define LOCAL_HOST "127.0.0.1" // Host address
#define AWS_UDP_PORT 23096 // AWS port number
#define AWS_Client_TCP_PORT 24096 // AWS port number
#define AWS_Monitor_TCP_PORT 25096 // AWS port number
#define ServerA_PORT 21096
#define ServerB_PORT 22096
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define BACKLOG 10 // max number of connections allowed on the incoming queue
#define FAIL -1

/**
 * Defined global variables
 */
string operation;
int link_id; // Integer input data
double size, signal_power, bandwidth, length, velocity, noise_power; // Floating point input data
double t_tran, t_prop, end_to_end; // Compute data

int sockfd_client_TCP, sockfd_monitor_TCP, sockfd_UDP; // Parent socket for client & for monitor & UDP socket
int child_sockfd_client, child_sockfd_monitor; // Child socket for connection with client and monitor
struct sockaddr_in aws_client_addr, aws_monitor_addr, aws_UDP_addr;
struct sockaddr_in dest_client_addr, dest_monitor_addr, dest_serverA_addr, dest_serverB_addr; // When AWS works as a client

char input_buf[MAXDATASIZE]; // Input data from client
char A_write_result[MAXDATASIZE]; // Write result returned from server A
char A_compute_result[MAXDATASIZE]; // Compute data returned from server A
char B_return_buf[MAXDATASIZE]; // Computed data returned by server B


/**
 * Defined functions
 */

// 1. Create TCP socket w/ client & bind socket
void create_TCP_client_socket();

// 2. Create TCP socket w/ monitor & bind socket
void create_TCP_monitor_socket();

// 3. Create UDP socket
void create_UDP_socket();

// 4. Listen for client
void listen_client();

// 5. Listen for monitor
void listen_monitor();

void init_connection_serverA();

void init_connection_serverB();


/**
 * Step 1: Create TCP socket for client & bind socket
 */
void create_TCP_client_socket() {
    sockfd_client_TCP = socket(AF_INET, SOCK_STREAM, 0); // Create TCP socket
    if (sockfd_client_TCP == FAIL) {
        perror("[ERROR] AWS server: fail to create socket for client");
        exit(1);
    }

    // Initialize IP address, port number
    memset(&aws_client_addr, 0, sizeof(aws_client_addr)); //  make sure the struct is empty
    aws_client_addr.sin_family = AF_INET; // Use IPv4 address family
    aws_client_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    aws_client_addr.sin_port = htons(AWS_Client_TCP_PORT); // Port number for client

    // Bind socket for client with IP address and port number for client
    if (::bind(sockfd_client_TCP, (struct sockaddr *) &aws_client_addr, sizeof(aws_client_addr)) == FAIL) {
        perror("[ERROR] AWS server: fail to bind client socket");
        exit(1);
    }

}

/**
 * Step 2: Create TCP socket for monitor & bind socket
 */

void create_TCP_monitor_socket() {

    sockfd_monitor_TCP = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (sockfd_monitor_TCP == FAIL) {
        perror("[ERROR] AWS server: fail to create socket for monitor");
        exit(1);
    }

    // Initialize IP address, port number
    memset(&aws_monitor_addr, 0, sizeof(aws_monitor_addr)); //  make sure the struct is empty
    aws_monitor_addr.sin_family = AF_INET; // Use IPv4 address family
    aws_monitor_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    aws_monitor_addr.sin_port = htons(AWS_Monitor_TCP_PORT); // Port number for monitor

    // Bind socket
    if (::bind(sockfd_monitor_TCP, (struct sockaddr *) &aws_monitor_addr, sizeof(aws_monitor_addr)) == FAIL) {
        perror("[ERROR] AWS server: fail to bind monitor socket");
        exit(1);
    }
}


/**
 * Step 3: Create UDP socket and bind socket
 */
void create_UDP_socket() {
    sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0); // UDP datagram socket
    if (sockfd_UDP == FAIL) {
        perror("[ERROR] AWS server: fail to create UDP socket");
        exit(1);
    }

    // Initialize IP address, port number
    memset(&aws_UDP_addr, 0, sizeof(aws_UDP_addr)); //  make sure the struct is empty
    aws_UDP_addr.sin_family = AF_INET; // Use IPv4 address family
    aws_UDP_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    aws_UDP_addr.sin_port = htons(AWS_UDP_PORT); // Port number for client

    // Bind socket
    if (::bind(sockfd_UDP, (struct sockaddr *) &aws_UDP_addr, sizeof(aws_UDP_addr)) == FAIL) {
        perror("[ERROR] AWS server: fail to bind UDP socket");
        exit(1);
    }
}

/**
 * Step 4: Listen for incoming connection from client
 */
void listen_client() {
    if (listen(sockfd_client_TCP, BACKLOG) == FAIL) {
        perror("[ERROR] AWS server: fail to listen for client socket");
        exit(1);
    }
}

/**
 * Step 5:  Listen for incoming connection from monitor
 */

void listen_monitor() {
    if (listen(sockfd_monitor_TCP, BACKLOG) == FAIL) {
        perror("[ERROR] AWS server: fail to listen for monitor socket");
        exit(1);
    }
}

void init_connection_serverA() {
    // Info about server A
    memset(&dest_serverA_addr, 0, sizeof(dest_serverA_addr));
    dest_serverA_addr.sin_family = AF_INET;
    dest_serverA_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverA_addr.sin_port = htons(ServerA_PORT);
}

void init_connection_serverB() {
    // Info about server B
    memset(&dest_serverB_addr, 0, sizeof(dest_serverB_addr));
    dest_serverB_addr.sin_family = AF_INET;
    dest_serverB_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverB_addr.sin_port = htons(ServerB_PORT);
}


int main() {

    create_TCP_client_socket();
    listen_client();
    create_TCP_monitor_socket();
    listen_monitor();
    create_UDP_socket();
    printf("The AWS is up and running.\n");

    /******    Step 7: Accept connection from monitor using child socket   ******/
    socklen_t monitor_addr_size = sizeof(dest_monitor_addr);
    child_sockfd_monitor = ::accept(sockfd_monitor_TCP, (struct sockaddr *) &dest_monitor_addr, &monitor_addr_size);
    if (child_sockfd_monitor == FAIL) {
        perror("[ERROR] AWS server: fail to accept connection with monitor");
        exit(1);
    }

    while (true) {

        /******    Step 6: Accept connection from client using child socket   ******/
        socklen_t client_addr_size = sizeof(dest_client_addr);
        // Accept listening socket (parent)
        child_sockfd_client = ::accept(sockfd_client_TCP, (struct sockaddr *) &dest_client_addr, &client_addr_size);
        if (child_sockfd_client == FAIL) {
            perror("[ERROR] AWS server: fail to accept connection with client");
            exit(1);
        }

        /******    Step 8: Receive input from client: write / compute   ******/
        /******    Beej's Notes  ******/
        // Receive through child socket
        int recv_client = recv(child_sockfd_client, input_buf, MAXDATASIZE, 0);
        if (recv_client == FAIL) {
            perror("[ERROR] AWS server: fail to receive input data from client");
            exit(1);
        }

        // Same data from client, but send to monitor and server A
        char data_buf[MAXDATASIZE];
        strncpy(data_buf, input_buf, strlen(input_buf));

        // Determine the operation to do
        operation = strtok(input_buf, " ");
        printf("The AWS received operation <%s> from the client using TCP over port <%d> \n", operation.c_str(),
               AWS_Client_TCP_PORT);

        // Send to monitor
        if (sendto(child_sockfd_monitor, data_buf, sizeof(data_buf), 0, (struct sockaddr *) &dest_monitor_addr,
                   sizeof(dest_monitor_addr)) == FAIL) {
            perror("[ERROR] AWS: fail to send input data to monitor");
            exit(1);
        }
        printf("The AWS sent operation <%s> and arguments to the monitor using TCP over port <%d> \n",
               operation.c_str(), AWS_Monitor_TCP_PORT);


        // Send to server A
        init_connection_serverA();
        if (sendto(sockfd_UDP, data_buf, sizeof(data_buf), 0, (const struct sockaddr *) &dest_serverA_addr,
                   sizeof(dest_serverA_addr)) == FAIL) {
            perror("[ERROR] AWS: fail to send input data to server A");
            exit(1);
        }
        printf("The AWS sent operation <%s> to Backend-Server A using UDP over port <%d> \n", operation.c_str(),
               AWS_UDP_PORT);


        /********************************************************************************************************/
        /*****************************************   Case 1: Write data     *************************************/
        /********************************************************************************************************/

        if (operation == "write") {
            bandwidth = atof(strtok(NULL, " "));
            length = atof(strtok(NULL, " "));
            velocity = atof(strtok(NULL, " "));
            noise_power = atof(strtok(NULL, " "));

            // Receive from Server A: write result
            socklen_t dest_serverA_size = sizeof(dest_serverA_addr);
            if (::recvfrom(sockfd_UDP, A_write_result, 1, 0, (struct sockaddr *) &dest_serverA_addr,
                           &dest_serverA_size) == FAIL) {
                perror("[ERROR] AWS: fail to receive writing result from Server A");
                exit(1);
            }
            printf("The AWS received response from Backend-Server A for writing using UDP over port <%d> \n",
                   AWS_UDP_PORT);

            char write_result[MAXDATASIZE];
            strncpy(write_result, A_write_result, sizeof(A_write_result));
            // Send write response to monitor
            if (sendto(child_sockfd_monitor, write_result, 1, 0,
                       (struct sockaddr *) &dest_monitor_addr,
                       sizeof(dest_monitor_addr)) == FAIL) {
                perror("[ERROR] AWS: fail to send write response from Server A to monitor");
                exit(1);
            }
            printf("The AWS sent write response to the monitor using TCP over port <%d> \n", AWS_Monitor_TCP_PORT);

            // Send write response to client
            if (sendto(child_sockfd_client, write_result, 1, 0, (struct sockaddr *) &dest_client_addr,
                       sizeof(dest_client_addr)) == FAIL) {
                perror("[ERROR] AWS: fail to send write response from Server A to client");
                exit(1);
            }
            printf("The AWS sent result to client for operation <write> using TCP over port <%d> \n",
                   AWS_Client_TCP_PORT);
        }


            /********************************************************************************************************/
            /***************************************   Case 2: Compute data     *************************************/
            /********************************************************************************************************/
        else if (operation == "compute") {
            link_id = atoi(strtok(NULL, " "));
            size = atof(strtok(NULL, " "));
            signal_power = atof(strtok(NULL, " "));


            // Receive from Server A: compute data
            socklen_t dest_serverA_size = sizeof(dest_serverA_addr);

            if (::recvfrom(sockfd_UDP, A_compute_result, MAXDATASIZE, 0, (struct sockaddr *) &dest_serverA_addr,
                           &dest_serverA_size) == FAIL) {
                perror("[ERROR] AWS: fail to receives return data from Server A");
                exit(1);
            }

            char result = A_compute_result[0];

            // If did not find link: no match
            if (result == 'f') {
                printf("Link ID not found \n");

                char no_match[MAXDATASIZE];
                strncpy(no_match, A_compute_result, sizeof(A_compute_result) + 1);

                // Send failed result to monitor
                if (sendto(child_sockfd_monitor, no_match, 2, 0, (struct sockaddr *) &dest_monitor_addr,
                           sizeof(dest_monitor_addr)) == FAIL) {
                    perror("[ERROR] AWS: fail to send compute response from Server A to monitor");
                    exit(1);
                }

                // Send failed result to client
                if (sendto(child_sockfd_client, no_match, 2, 0, (struct sockaddr *) &dest_client_addr,
                           sizeof(dest_client_addr)) == FAIL) {
                    perror("[ERROR] AWS: fail to send compute response from Server A to client");
                    exit(1);
                }
            } else {
                printf("The AWS received link information from Backend-Server A using UDP over port <%d> \n",
                       AWS_UDP_PORT);

                char compute_result_A[MAXDATASIZE];
                strncpy(compute_result_A, A_compute_result, sizeof(A_compute_result));

                //cout << "!!!!!!!!!! Data from server A !!!!!!!!!!!" << A_compute_result << endl;

                // Send data to Server B
                init_connection_serverB();
                if (sendto(sockfd_UDP, compute_result_A, MAXDATASIZE, 0, (const struct sockaddr *) &dest_serverB_addr,
                           sizeof(dest_serverB_addr)) == FAIL) {
                    perror("[ERROR] AWS: fail to send input data to server B");
                    exit(1);
                }

                printf("The AWS sent link ID=<%d>, size=<%.2f>, power=<%.2f>, and link information to Backend-Server B using UDP over port <%d> \n",
                       link_id, size, signal_power, AWS_UDP_PORT);


                // Receive computed result from Server B
                socklen_t dest_serverB_size = sizeof(dest_serverB_addr);
                if (::recvfrom(sockfd_UDP, B_return_buf, MAXDATASIZE, 0, (struct sockaddr *) &dest_serverB_addr,
                               &dest_serverB_size) == FAIL) {
                    perror("[ERROR] AWS: fail to receive computed result from Server B");
                    exit(1);
                }
                printf("The AWS received outputs from Backend-Server B using UDP over port <%d> \n", AWS_UDP_PORT);

                char compute_result_B[MAXDATASIZE];
                strncpy(compute_result_B, B_return_buf, sizeof(B_return_buf));

                // Send compute result to client
                if (sendto(child_sockfd_client, compute_result_B, sizeof(B_return_buf),
                           0,(struct sockaddr *) &dest_client_addr,
                        sizeof(dest_client_addr)) == FAIL) {
                    perror("[ERROR] AWS: fail to send compute result from Server B to client");
                    exit(1);
                }
                printf("The AWS sent result to client for operation <%s> using TCP over port <%d> \n",
                       operation.c_str(),
                       AWS_Client_TCP_PORT);


                // Send computed result to monitor
                if (sendto(child_sockfd_monitor, compute_result_B, sizeof(B_return_buf), 0,
                           (struct sockaddr *) &dest_monitor_addr,
                           sizeof(dest_monitor_addr)) == FAIL) {
                    perror("[ERROR] AWS: fail to send compute result from Server B to monitor");
                    exit(1);
                }
                printf("The AWS sent compute results to the monitor using TCP over port <%d> \n", AWS_Monitor_TCP_PORT);
            }
        }
    }

    // Close parent socket
    close(sockfd_client_TCP);
    close(sockfd_monitor_TCP);
    close(sockfd_UDP);
    return 0;
}

