#include "tcpd.h"
#include "troll.h"

main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage(server): %s -s, or\n", argv[0]);
        printf("Usage(client): %s -c\n", argv[0]);
        exit(1);
    }
    if (strcmp(argv[1], "-s") == 0) {
        tcpd_server(argc, argv);
    } else if (strcmp(argv[1], "-c") == 0) {
        tcpd_client(argc, argv);
    }

}

void tcpd_server(int argc, char **argv) {
    TCPD_MSG           tcpd_msg;
    NetMessage         troll_msg;
    int                sock, srv_sock;
    struct sockaddr_in my_addr, server_addr;
    char               buff[MAXBUF];

    printf("Setting up socket...\n");
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error openting datagram socket");
        exit(1);
    }
    printf("Server socket initialized \n");

    //Copying socket to send to ftps
    srv_sock = sock;

    //Constructing socket name for receiving
    my_addr.sin_family      = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;            //Listen to any IP address
    my_addr.sin_port        = htons(TCPD_PORT);

    //Binding socket name to socket
    printf("Binding socket to socket name...\n");
    if (bind(sock, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding stream socket");
        exit(1);
    }
    printf("Socket name binded, waiting...\n");

    //To hold the length of my_addr
    int len = sizeof(my_addr);

    //Counter to count number of datagrams forwarded
    int count = 0;

    //Always keep on listening and sending
    while (1) {
        //Receiving from troll
        int rec = recvfrom(sock, &troll_msg, sizeof(troll_msg), 0, (struct sockaddr *) &my_addr, &len);

        if (rec < 0) {
            perror("Error receiving datagram");
            close(sock);
            close(srv_sock);
            exit(1);
        }

        bcopy(&troll_msg.msg_contents, &tcpd_msg, rec - 16);
        server_addr = tcpd_msg.header;
	    server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        ////Sending to ftps
        int s = sendto(srv_sock, tcpd_msg.contents, rec - 32, 0, (struct sockaddr *) &server_addr,
                       sizeof(server_addr));

        //puts(tcpd_msg.contents);
        if (s < 0) {
            perror("Error sending datagram");
            close(sock);
            close(srv_sock);
            exit(1);
        }

        printf("Received and sent --> %d\n", count);

        //Incrementing counter
        count++;

    }
}

void tcpd_client(int argc, char **argv) {
    TCPD_MSG           message;                                //Packet format accepted by troll
    NetMessage         troll_message;
    int                sock, troll_sock;                               //Initial socket descriptors
    struct sockaddr_in troll, my_addr, ftps_addr;                    //Structures for server and tcpd socket name setup

    //If there are more or less than 3 arguments show error
    //First argument: exec file         Second argument: local tcpd port number
    //Third argument: local troll port number

    //Initialize socket for UDP in linux
    printf("Setting up socket...\n");
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error openting datagram socket");
        exit(1);
    }
    printf("Socket initialized \n");

    //Copying socket to send to troll
    troll_sock = sock;

    //Constructing socket name for receiving
    my_addr.sin_family      = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;            //Listen to any IP address
    my_addr.sin_port        = htons(TCPD_PORT);

    //Constructing socket name of the troll to send to
    troll.sin_family      = AF_INET;
    troll.sin_port        = htons(TROLL_PORT);
    troll.sin_addr.s_addr = inet_addr("127.0.0.1");

    //Binding socket name to socket
    printf("Binding socket to socket name...\n");
    if (bind(sock, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding stream socket");
        close(sock);
        close(troll_sock);
        exit(1);
    }
    printf("Socket name binded, waiting...\n");

    //To hold the length of my_addr
    int len = sizeof(my_addr);

    //Counter to count number of datagrams forwarded
    int count = 0;

    bzero(troll_message.msg_contents, MAXBUF + 16);

    //Always keep on listening and sending
    while (1) {

        //Receiving from ftpc
        int rec = recvfrom(sock, &message, sizeof(message), 0, (struct sockaddr *) &my_addr, &len);
        ftps_addr = message.header;
        ftps_addr.sin_port       = htons(TCPD_PORT);
        troll_message.msg_header = ftps_addr;

        if (rec < 0) {
            perror("Error receiving datagram");
            close(sock);
            close(troll_sock);
            exit(1);
        }

        printf("Received data, sending to troll --> %d\n", count);

        bcopy((char *) &message, &troll_message.msg_contents, rec);
        //puts(message.contents);
        //Sending to troll
        int s = sendto(troll_sock, &troll_message, rec + 16, 0, (struct sockaddr *) &troll, sizeof(troll));

        if (s < 0) {
            perror("Error sending datagram");
            close(sock);
            close(troll_sock);
            exit(1);
        }

        //Incrementing counter
        count++;

    }
}
