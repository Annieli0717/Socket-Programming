# USC-EE450-Socket-Programming
USC EE 450 (Computer Networks) final project


a. Name: Dunxuan Li (Annie)


b. USC NetID: 


c. Have done in the assignment
        In this project, I have implemented a model computational offloading where client can operate
    two functions: compute & write, and send operation request to AWS server. AWS server will reply to
    client depends on the function. For write function, AWS server will acknowledge client if input data
    has been written into database successfully; for compute data, AWS server will return the compute
    result for the end-to-end delay of the designated link.
        In the model, we have two backend servers: server A is a storage server, who is responsible for
    write data into database and search data entries for a specified link. Server B is a computing server,
    who is responsible for doing calculation. Besides, there is a monitor to display every step.


d. Code files:

        1. client.cpp
                (specifies which function to do with corresponding parameters)
                - Create TCP socket with AWS server
                - Connect with AWS server
                - Send write/compute data to AWS server
                - Get write/compute results from AWS server

        2. monitor.cpp
                (keep track of the whole process and print each step out)
                - Create TCP socket with AWS server
                - Connect with AWS server
                - Receive data entries from AWS
                - Receive results from AWS

        3. aws.cpp
                (central server that is responsible to forward messages to correct server)
                - Create TCP sockets with client and monitor
                - Create UDP socket
                - Receive data entries from client and forward it to monitor and server A
                - Receive write results from server A and forward it to client and monitor
                - Receive search results from server A and forward data to server B
                - Receive compute results from server B and forward to client and monitor

        4. serverA.cpp
                (storage server which writes data into database and searches for parameters for a specified link)
                - Create UDP socket
                - Write data into "database.txt"
                - Search data in database

        5. serverB.cpp
                (computing server which computes transmission delay, propagation delay and end-to-end delay)
                - Create UDP socket
                - Receive data from AWS server
                - Compute transmission delay, propagation delay, and ene-to-end, and forward to AWS.


e. The format of all the messages exchanged
        In general, when a host sends or receives a message, I put every parameter together in a string
   separated by a whitespace. Then store the message with all parameters in a char array.
        When I want to access data inside the message, use strtok() method to separate each parameter by
   whitespace.
        Belows are messages sent & received from each host with parameters inside buffer

        1. client:
                - data buffer for write data (write_buf): "write"  bandwidth  length  velocity  noise_power
                - data buffer for compute data (compute_buf): "compute"  link_ID  size  signal_power
                - returned write result from AWS (write_result): "1" if writing is successful
                - returned compute result from AWS: t_tran  t_prop  end-to-end / "f" if link is not found

        2. monitor
                - data buffer for input data (operation_buf): "write"  bandwidth  length  velocity  noise_power /
                                                              "compute"  link_ID  size  signal_power
                - returned write result from AWS (write_result): "1" if writing is successful
                - returned compute result from AWS: t_tran  t_prop  end-to-end / "f" if link is not found

        3. AWS
                - data buf sent by client (input_buf)
                - write result returned by server A (A_write_result)
                - search result returned by server A (A_compute_result)
                - compute result returned by server B (B_retu)rn_buf

        4. server A
                - data that need to be written into database / need to be searched (recv_buf)
                - returned write result (write_result): "1" if writing is successful
                - returned compute result from AWS: "f" if link is not found /
                                                    link_id  size  signal_power  bandwidth
                                                    length  velocity  noise_power

        5. server B
                - data buffer received from AWS of input data (data_buf): link_id  size  signal_power  bandwidth
                                                                          length  velocity  noise_power
                - computed data that is sent to AWS (result_buf): t_tran  t_prop  end_to_end


g. Idiosyncrasy
    Based on the test results run on given environment (Ubuntu), there was no idiosyncrasy found


h. Reused code:

    1. Beej's Code: http://www.beej.us/guide/bgnet/
            Create sockets (TCP / UDP);
            Bind a socket;
            Send & receive;

    2. Tutorials on Socket Programming with C++: http://c.biancheng.net
            Write data to database;
            Read data to database;
            Transfer files between server and client;







