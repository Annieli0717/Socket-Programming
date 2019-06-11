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
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * Client.cpp
 * A client, sends write and compute data to AWS server
 *
*/


/**
 * Named Constants
 */
#define LOCAL_HOST "127.0.0.1" // Host address
#define AWS_Client_TCP_PORT 24096 // AWS port number
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define FAIL -1 // socket fails if result = -1


/**
 * Defined global variables
 */
string operation; // Operation to do
string bandwidth, length, velocity, noise_power; // Data to write
string link_id, size, signal_power; // Data to compute
double t_tran, t_prop, end_to_end;

int sockfd_client_TCP; // Client socket
struct sockaddr_in aws_addr; // AWS server address
char write_buf[MAXDATASIZE]; // Store input to write (send to AWS)
char compute_buf[MAXDATASIZE]; // Store input to compute (send to AWS)
char write_result[MAXDATASIZE]; // Write result from AWS
char compute_result[MAXDATASIZE]; // Compute result from AWS

/**
 * Steps (defined functions):
 */

// 1. Create TCP socket
void create_client_socket_TCP();

// 2. Initialize TCP connection with AWS
void init_AWS_connection();

// 3. Send connection request to AWS server
void request_AWS_connection();

// 4. Send data to AWS (write / compute)

// 5. Get result back from AWS Server (write result / computed result)


/**
 * Step 1: Create client socket (TCP stream socket)
 */
void create_client_socket_TCP() {
    sockfd_client_TCP = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sockfd_client_TCP == FAIL) {
        perror("[ERROR] client: can not open client socket ");
        exit(1);
    }

}

/**
 * Step 2: Initial TCP connection info
 */
void init_AWS_connection() {

    // *** Beej’s guide to network programming - 9.24. struct sockaddr and pals ***
    // Initialize TCP connection between client and AWS server using specified IP address and port number
    memset(&aws_addr, 0, sizeof(aws_addr)); //  make sure the struct is empty
    aws_addr.sin_family = AF_INET; // Use IPv4 address family
    aws_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Source address
    aws_addr.sin_port = htons(AWS_Client_TCP_PORT); // AWS server port number
}

/**
 * Step 3: Send connection request to AWS server
 */
void request_AWS_connection() {
    connect(sockfd_client_TCP, (struct sockaddr *) &aws_addr, sizeof(aws_addr));

    /*if (connect(sockfd_client_TCP, (struct sockaddr *) &aws_addr, sizeof(aws_addr)) == FAIL) {
        perror("[ERROR] client: fail to connect with AWS server");
        close(sockfd_client_TCP);
        exit(1); // If connection failed, we cannot continue
    }*/

    // If connection succeed, display boot up message
    printf("The client is up and running \n");
}


int main(int argc, char *argv[]) {

    create_client_socket_TCP();
    init_AWS_connection();
    request_AWS_connection();
    operation = argv[1];

    // Check the validity of operation
    // Only two options: write / compute
    if (operation != "write" && operation != "compute") {
        printf("Please enter a valid operation: write / compute \n");
        exit(1);
    }


    /********************************************************************************************************/
    /*****************************************   Case 1: Write data     *************************************/
    /********************************************************************************************************/
    if (operation == "write") {

        // Check the validity of input data
        if (argc != 6) {
            perror("[ERROR] Please enter correct parameters in the order: <Bandwidth> <Length> <Velocity> <Noise Power> ");
            exit(1);
        }

        bandwidth = argv[2];
        length = argv[3];
        velocity = argv[4];
        noise_power = argv[5];

        // Put everything in char array, separate each parameter by a white space
        string str = operation + " " + bandwidth + " " + length + " " + velocity + " " + noise_power + " ";
        strncpy(write_buf, str.c_str(), MAXDATASIZE);

        /******    Step 4:  Send write data to AWS server    *******/
        if (send(sockfd_client_TCP, write_buf, sizeof(write_buf), 0) == FAIL) {
            perror("[ERROR] client: fail to send input data");
            close(sockfd_client_TCP);
            exit(1);
        }
        printf("The client sent write operation to AWS \n");

        /******    Step 5:  Get write result back from AWS Server    *******/
        if (recv(sockfd_client_TCP, write_result, sizeof(write_result), 0) == FAIL) {
            perror("[ERROR] client: fail to receive write result from AWS server");
            close(sockfd_client_TCP);
            exit(1);
        }
        char result = write_result[0];
        if (result == '1') {
            // If successfully receive from AWS server, display confirmation message
            printf("The write operation has been completed successfully \n");
        }
    }

        /********************************************************************************************************/
        /***************************************   Case 2: Compute data     *************************************/
        /********************************************************************************************************/
    else if (operation == "compute") {

        // Check the validity of input data
        if (argc != 5) {
            perror("[ERROR]Please enter correct parameters in the order: <Link ID> <Size> <Signal Power>");
            exit(1);
        }

        link_id = argv[2];
        size = argv[3];
        signal_power = argv[4];

        string str = operation + " " + link_id + " " + size + " " + signal_power + " ";
        strcpy(compute_buf, str.c_str());

        /******    Step 4:  Send compute data to AWS server    *******/
        if (send(sockfd_client_TCP, compute_buf, sizeof(compute_buf), 0) == FAIL) {
            perror("[ERROR] client: fail to send input data");
            close(sockfd_client_TCP);
            exit(1);
        }
        printf("The client sent ID=<%s>, size=<%s>, and power=<%s> to AWS \n", link_id.c_str(), size.c_str(),
               signal_power.c_str());


        /******    Step 5:  Get result back from AWS Server    *******/
        if (recv(sockfd_client_TCP, compute_result, sizeof(compute_result) + 1, 0) == FAIL) {
            perror("[ERROR] client: fail to receive result from AWS server");
            close(sockfd_client_TCP);
            exit(1);
        }

        // If compute result from AWS = f, then it indicates the link cannot be found
        // Else, the computed result will be returned

        /*cout << "computer result is " << compute_result << endl;
        cout << "size of result is  " << sizeof(compute_result) << endl;*/
        char result = compute_result[0];
        if (result == 'f') {
            printf("Link ID not found \n");
        } else {
            t_tran = atof(strtok(compute_result, " "));
            t_prop = atof(strtok(NULL, " "));
            end_to_end = atof(strtok(NULL, " "));

            // If successfully receive from AWS server, display final computing result
            printf("The delay for link <%s> is <%.2f>ms \n", link_id.c_str(), end_to_end);
        }
    }

    // Close the socket and tear down the connection after we are done using the socket
    close(sockfd_client_TCP);
    return 0;
}





