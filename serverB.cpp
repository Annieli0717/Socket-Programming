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
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <sstream>

using namespace std;

/**
 * serverB.cpp
 * A computing server, computes the transmission delay,
 * the propagation delay, and the end-to-end delay of
 * link based on input sent from AWS server
 *
*/

/**
 * Named Constants
 */
#define LOCAL_HOST "127.0.0.1" // Host IP address
#define ServerB_UDP_PORT 22096 // Server B port number (listen for incoming data)
#define MAXDATASIZE 1024 // max number of bytes we can get at once (10KB)
#define FAIL -1

/**
 * Defined global variables
 */
int link_id; // Integer input data
double size, signal_power, bandwidth, length, velocity, noise_power; // Floating point input data
double S, N, capacity;
double t_tran, t_prop, end_to_end; // Compute data

int sockfd_serverB_UDP; // Server B datagram socket
struct sockaddr_in serverB_addr, aws_addr; // AWS as a server & as a client

char data_buf[MAXDATASIZE]; // Data received from AWS that needed to compute



/**
 * Steps: Defined functions
 */

// 1. Create UDP socket
void create_serverB_socket();

// 2. Initialize connection with AWS server
void init_AWS_connection();

// 3. Bind a socket
void bind_socket();

// 4. Receive data from AWS server

// 5. Compute the data

// 6. Send the computed data to AWS server

/**
 * Step 1: Create server B UDP sockect
 */
void create_serverB_socket() {
    sockfd_serverB_UDP = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP datagram socket
    if (sockfd_serverB_UDP < 0) {
        perror("[ERROR] server B: can not open socket");
        exit(1);
    }
}

/**
 * Step 2: Create sockaddr_in struct
 */

void init_AWS_connection() {

    // Server B side information
    // Initialize server B IP address, port number
    memset(&serverB_addr, 0, sizeof(serverB_addr)); //  make sure the struct is empty
    serverB_addr.sin_family = AF_INET; // Use IPv4 address family
    serverB_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    serverB_addr.sin_port = htons(ServerB_UDP_PORT); // Server B port number
}


/**
 * Step 3: Bind socket with specified IP address and port number
 */
void bind_socket() {
    if (::bind(sockfd_serverB_UDP, (struct sockaddr *) &serverB_addr, sizeof(serverB_addr)) < 0) {
        perror("[ERROR] Server B: fail to bind AWS UDP socket");
        exit(1);
    }

    printf("The Server B is up and running using UDP on port <%d> \n", ServerB_UDP_PORT);
}


int main() {
    /*string str = "5 1000 10 25 3 20000 10 ";
    strcpy(data_buf, str.c_str());*/

    /******    Step 1: Create server B socket (UDP)  ******/
    create_serverB_socket();
    /******    Step 2: Create sockaddr_in struct  ******/
    init_AWS_connection();
    /******    Step 3: Bind socket with specified IP address and port number  ******/
    bind_socket();

    while (true) {

        /******    Step 4: Receive data from AWS  ******/
        socklen_t aws_addr_size = sizeof(aws_addr);
        if (::recvfrom(sockfd_serverB_UDP, data_buf, MAXDATASIZE, 0, (struct sockaddr *) &aws_addr,
                       &aws_addr_size) == FAIL) {
            perror("[ERROR] Server B: fail to receive data from AWS server");
            exit(1);
        }
        /*cout << "data buf is " << data_buf << endl;
        cout << "size of data buf is " << sizeof(data_buf) << endl;
*/
        char data_to_compute[MAXDATASIZE];
        strncpy(data_to_compute, data_buf, MAXDATASIZE);

        // Data received from AWS server
        // stod() method does not work for our compiler => use atoi() instead
        // Convert received data as string and convert & separate them into numbers
        cout << "!!!!!! Data from AWS are !!!!!!!" << data_to_compute << endl;

        int link_id = atoi(strtok(data_to_compute, " ")); // Convert string to int
        double size = atof(strtok(NULL, " ")); // Convert string to double
        double signal_power = atof(strtok(NULL, " "));

        double bandwidth = atof(strtok(NULL, " "));
        double length = atof(strtok(NULL, " "));
        double velocity = atof(strtok(NULL, " "));
        double noise_power = atof(strtok(NULL, " "));

        /*// Compute data
        cout << "link_id = " << link_id << endl;
        cout << "size = " << size << endl;
        cout << "signal = " << signal_power << endl;

        //  data from server A
        cout << "bandwidth = " << bandwidth << endl;
        cout << "length =  " << length << endl;
        cout << "velocity =  " << velocity << endl;
        cout << "noise power = " << noise_power << endl;*/

        // Print float point number in two decimal place
        printf("The Server B received link information: link <%d>, file size <%.2f>, and signal power <%.2f> \n",
               link_id, size, signal_power);


        /******    Step 5: Compute data  ******/
        // Unit Conversion: Signal power [Watt] = 10^{(Signal Power[dBm] - 30) / 10}
        S = pow(10, (signal_power - 30) / 10);
        // Unit Conversion: Noise power [Watt] = 10^{(Noise Power[dBm] - 30) / 10}
        N = pow(10, (noise_power - 30) / 10);
        // Capacity = Blog2(1+S/N)  [Shannon-Hartley Theorem]
        // Bandwidth is in MHz => turn it to Hz
        capacity = (bandwidth * 1000000) * log2(1 + S / N);

        // T_tran [msec]= packet size / bit rate
        t_tran = size / capacity * 1000;
        // T_prop [msec] = link length / link rate
        t_prop = length / velocity * 1000;
        // End-to-end delay [sec] = transmission delay + propagation delay
        end_to_end = t_tran + t_prop;

        t_tran = roundf(t_tran * 100) / 100;
        t_prop = roundf(t_prop * 100) / 100;
        end_to_end = roundf(end_to_end * 100) / 100;

        printf("The Server B finished the calculation for link <%d> \n", link_id);


        /******    Step 6: Send computed data to AWS server   ******/
        stringstream t_t, t_p, t_end;
        t_t << t_tran;
        t_p << t_prop;
        t_end << end_to_end;
        string result = t_t.str() + " " + t_p.str() + " " + t_end.str() + " ";
        char result_buf[MAXDATASIZE];
        strcpy(result_buf, result.c_str());
        if (sendto(sockfd_serverB_UDP, result_buf, sizeof(result_buf), 0, (struct sockaddr *) &aws_addr,
                   aws_addr_size) == FAIL) {
            perror("[ERROR] Server B: fail to send computed data to AWS server");
            exit(1);
        }

        printf("The Server B finished sending the output to AWS \n");
    }

    close(sockfd_serverB_UDP);
    return 0;
}

