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

using namespace std;

/**
 * monitor.cpp
 * A stream socket server, records results of every steps and print them out
 *
*/

/**
 * Named Constants
 */
#define LOCAL_HOST "127.0.0.1" // Host address
#define AWS_Monitor_TCP_PORT 25096 // AWS port number
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define FAIL -1

/**
 * Defined global variables
 */

string operation;
int link_id; // Integer input data
double size, signal_power, bandwidth, length, velocity, noise_power; // Floating point input data
double t_tran, t_prop, end_to_end; // Compute data

int sockfd_monitor_TCP; // Monitor stream socket
struct sockaddr_in aws_addr; // AWS server address

char operation_buf[MAXDATASIZE]; // First data received from AWS server: write / compute
char write_result[MAXDATASIZE]; // Write result from AWS
char compute_result[MAXDATASIZE]; // Compute result from AWS

/**
 * Steps: Defined functions
 */

// 1. Create TCP socket
void create_monitor_socket_TCP();

// 2. Initialize TCP connection with AWS
void init_AWS_connection();

// 3. Send connection request to AWS server
void request_AWS_connection();

// 4. Receive the first response from AWS, need to determine it is for writing or computing

// 6. For compute,receive the data from AWS

// 7. For compute,receive the result from AWS

/**
 * Step 1: Create monitor socket (TCP stream socket)
 */
void create_monitor_socket_TCP() {
    sockfd_monitor_TCP = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sockfd_monitor_TCP == FAIL) {
        perror("[ERROR] monitor: can not open monitor socket ");
        exit(1);
    }

}

/**
 * Step 2: Initial TCP connection info
 */
void init_AWS_connection() {

    // *** Beej’s guide to network programming - 9.24. struct sockaddr and pals ***
    // Initialize TCP connection between monitor and AWS server using specified IP address and port number
    memset(&aws_addr, 0, sizeof(aws_addr)); //  make sure the struct is empty
    aws_addr.sin_family = AF_INET; // Use IPv4 address family
    aws_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Source address
    aws_addr.sin_port = htons(AWS_Monitor_TCP_PORT); // AWS server port number
}

/**
 * Step 3: Send connection request to AWS server
 */
void request_AWS_connection() {
    connect(sockfd_monitor_TCP, (struct sockaddr *) &aws_addr, sizeof(aws_addr));

    /*if (connect(sockfd_monitor_TCP, (struct sockaddr *) &aws_addr, sizeof(aws_addr)) == FAIL) {
        perror("[ERROR] monitor: fail to connect with AWS server");
        close(sockfd_monitor_TCP);
        exit(1); // If connection failed, we cannot continue
    }*/

    // If connection succeed, display boot up message
    printf("The monitor is up and running \n");
}


int main() {

    /******    Step 1: Create client socket (TCP)  ******/
    create_monitor_socket_TCP();
    /******    Step 2: Initialize connection with AWS server     *******/
    init_AWS_connection();
    /******    Step 3: Send connection request to AWS server     *******/
    request_AWS_connection();

    while (true) {

        /******    Step 4. Receive the first response from AWS server     *******/

        if (recv(sockfd_monitor_TCP, operation_buf, sizeof(operation_buf), 0) < 0) {
            perror("[ERROR] monitor: fail to receive operation data from AWS server");
            close(sockfd_monitor_TCP);
            exit(1);
        }

        operation = strtok(operation_buf, " ");

        /********************************************************************************************************/
        /*****************************************   Case 1: Write data     *************************************/
        /********************************************************************************************************/

        if (operation == "write") {
            bandwidth = atof(strtok(NULL, " "));
            length = atof(strtok(NULL, " "));
            velocity = atof(strtok(NULL, " "));
            noise_power = atof(strtok(NULL, " "));

            // Each parameter is separated by a white space in char array
            printf("The monitor received BW = <%.2f>, L = <%.2f>, V = <%.2f> and P = <%.2f> from the AWS \n", bandwidth,
                   length, velocity, noise_power);

            /******    Step 6. For write,receive the result from AWS server     *******/
            if (recv(sockfd_monitor_TCP, write_result, sizeof(write_result), 0) < 0) {
                perror("[ERROR] monitor: fail to receive write result from AWS server");
                close(sockfd_monitor_TCP);
                exit(1);
            }
            // If data has been wrote successfully, server A will return 1
            char result = write_result[0];
            if (result == '1') {
                printf("The write operation has been completed successfully \n");
            }
        }

            /********************************************************************************************************/
            /***************************************   Case 2: Compute data     *************************************/
            /********************************************************************************************************/
        else if (operation == "compute") {

            link_id = atoi(strtok(NULL, " "));
            size = atof(strtok(NULL, " "));
            signal_power = atof(strtok(NULL, " "));

            printf("The monitor received link ID=<%d>, size=<%.2f>, and power=<%.2f> from the AWS \n", link_id, size,
                   signal_power);

            /******    Step 7. For compute,receive the result from AWS server     *******/
            if (recv(sockfd_monitor_TCP, compute_result, sizeof(compute_result) + 1, 0) < 0) {
                perror("[ERROR] monitor: fail to receive compute result from AWS server");
                close(sockfd_monitor_TCP);
                exit(1);
            }

            //If compute result from AWS = f, then it indicates the link cannot be found
            // Else, the computed result will be returned
            // cout << "computer result is " << compute_result << endl;

            char result = compute_result[0];
            if (result == 'f') {
                printf("Link ID not found \n");
            }else{
                t_tran = atof(strtok(compute_result, " "));
                t_prop = atof(strtok(NULL, " "));
                end_to_end = atof(strtok(NULL, " "));
                printf("The result for link <%d>: \n Tt = <%.2f>ms \n Tp = <%.2f>ms \n Delay = <%.2f>ms \n", link_id,
                       t_tran, t_prop, end_to_end);
            }
        }
    }

    // Close the socket and tear down the connection after we are done using the socket
    close(sockfd_monitor_TCP);

    return 0;
}



