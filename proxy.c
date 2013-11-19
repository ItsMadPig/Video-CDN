/******************************************************************************
* lisod.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.                               *
*                                                                             *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "helper.h"
#include "http_parser.h"
//#include "http_reply.h"

#define DEBUG 1

int create_response(int sock, struct buf *bufp){
	printf("create_response\n");
    int locate_ret;
    
    if (bufp->res_fully_created == 1)
	return bufp->buf_size; // no more reply to created, just send what left in buf

    //if ((locate_ret = locate_file(bufp)) == -1) {
	//dbprintf("create_response: file not located, create msg404\n");

	//push_error(bufp, MSG404);
	//return bufp->buf_size; 
    //} else if (locate_ret == 0){
	//dbprintf("create_response: static file located, path:%s\n", bufp->path);

	//if (create_line_header(bufp) == -1) return bufp->buf_size; // msg404
	//if (create_res_body(bufp) == -1) return bufp->buf_size; // msg 500, only works if it's GET method

	dbprintf("create_response: bufp->size:%d\n", bufp->buf_size);
	//return bufp->buf_size;
    //} 
    return bufp->buf_size;  
}

int send_response(int sock, struct buf *bufp){
	printf("send_response\n");
    int sendret;

    if (bufp->buf_size == 0) {
	dbprintf("send_response: buffer is empty, sending finished\n");
	return 0;
    }

	sendret = send(sock, bufp->rbuf_head, bufp->buf_size, 0);

    if (sendret == -1) {
	perror("Error! send_response: send");
	return -1;
    }


    bufp->buf_head += sendret;
    bufp->buf_size -= sendret;
    
    dbprintf("send_response: %d bytes are sent\n", sendret);

    // whole buf is sent, reset it for possible more respose
    if (bufp->buf_size == 0) {
		reset_buf(bufp);
		dbprintf("send_response: whole buf is sent, reset it\n");
    }


    return sendret;
}



int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

void clear_buf_array(struct buf *buf_pts[], int maxfd);

int main(int argc, char* argv[])
{
    if (argc < 3) {
	fprintf(stderr, "Usage: ./lisod <HTTP port> ...\n");
	return EXIT_FAILURE;
    }

    int master_sock, client_sock, server_sock;
    int recv_ret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr, serv_addr;
    //    char buf[BUF_SIZE];
    struct buf* buf_pts[MAX_SOCK]; // array of pointers to struct buf

    char clientIP[INET6_ADDRSTRLEN];
    fd_set read_fds, write_fds;
    fd_set master_read_fds, master_write_fds;
    int maxfd, i, isClient, temp;


    const int LISTEN_PORT = atoi(argv[1]);
    char FAKE_IP[sizeof(argv[2])];
    strcpy(FAKE_IP, argv[2]);

    printf("Entered Listen_port: %d, ", LISTEN_PORT);
    printf("Entered Fake_ip: %s\n",FAKE_IP);
    fprintf(stdout, "----- Echo Server -----\n");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTEN_PORT);



	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(FAKE_IP);
	serv_addr.sin_port = htons(8080);





    /* all networked programs must create a socket */
    if ((master_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating client socket.\n");
        return EXIT_FAILURE;
    }

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(master_sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(master_sock);
        fprintf(stderr, "Failed binding client socket.\n");
        return EXIT_FAILURE;
    }

    if (listen(master_sock, 10))
    {
        close_socket(master_sock);
        fprintf(stderr, "Error listening on client socket.\n");
        return EXIT_FAILURE;
    }

    printf("master_sock: %d\n", master_sock);

    /* make copies of read_fds and write_fds */
    FD_ZERO(&master_write_fds);
    FD_ZERO(&master_read_fds);

    FD_SET(master_sock, &master_read_fds);
    
    maxfd = master_sock;

    /* run until coming across errors */
    while (1){

	/* reset read_fds and write_fds  */
	read_fds = master_read_fds;
	write_fds = master_write_fds;
	//read_server_fds = master_read_server_fds;
	//write_server_fds = master_write_server_fds;

	if (select(maxfd+1, &read_fds, &write_fds, NULL, NULL) == -1) {
	    perror("Error! select");
	    close_socket(master_sock);
	    clear_buf_array(buf_pts, maxfd);
	    return EXIT_FAILURE;
	} 
	
	/* check file descriptors in read_fds and write_fds*/
	for (i = 0; i <= maxfd; i++) {

	    /* check fd in read_fds */
	    if (FD_ISSET(i, &read_fds)) {

		if (i == master_sock) {
		    /* listinging sockte is ready, server receives new connection */

		    if ((client_sock = accept(master_sock, (struct sockaddr *)&cli_addr, &cli_size)) == -1){
			perror("Error! accept error! ignore it");
		    } else {

				dbprintf("Server: received new connection from %s, ", inet_ntop(AF_INET, &(cli_addr.sin_addr), clientIP, INET6_ADDRSTRLEN)); 
				dbprintf("socket %d to web browser is created\n", client_sock); // debug print

				/* alloc buffer only if the client_sock is smaller than MAX_SOCK  */
				if (!is_2big(client_sock)) {


					/*server socket*/
				    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
				        fprintf(stderr, "Failed creating server socket.\n");
				        return EXIT_FAILURE;
				    }
				    printf("server_sock is %d\n", server_sock);

			        
					if (connect(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
					    perror("ERROR! connection error\n");
					    close_socket(client_sock);
					    return EXIT_FAILURE;
				    }

					//allocate and initialize for socket to browser
				    buf_pts[client_sock] = (struct buf*) calloc(1, sizeof(struct buf));
				    init_buf(buf_pts[client_sock]); // initialize struct buf
				    dbprintf("buf_pts[%d] allocated, rbuf_free_size:%d\n", client_sock, buf_pts[client_sock]->rbuf_free_size);
				    printf("Socket %d to web server is created\n", server_sock);

				    buf_pts[client_sock]->client_sock = client_sock;
				    buf_pts[client_sock]->server_sock = server_sock;

				    /* track maxfd */
				    if (client_sock > maxfd)
						maxfd = client_sock;
					
					if (server_sock > maxfd)
						maxfd = server_sock;

					

				    FD_SET(client_sock, &master_read_fds);


				} else {
					// send error
				    close_socket(client_sock);
				}	    

		    }
		    
		} else {
			/* conneciton socket is ready, read */

		    /* recv_ret -1: recv error; 0: recv 0; 1: recv some bytes */
		    recv_ret = recv_request(i, buf_pts[i]); 
		    dbprintf("Server: recv_request from sock %d, recv_ret is %d\n", i, recv_ret);

			isClient = isClientSock(buf_pts,i,maxfd);
			printf("isClient returned: %d\n", isClient);
			if (isClient == -1){
				printf("Cannot find socket when reading");
				return EXIT_FAILURE;
			}else if (isClient == 1){
				////////////////////////
				//income req from client (browser)
				////////////////////////
				if (recv_ret == 1){
					temp = buf_pts[i]->server_sock;    // temp is server sock

					dbprintf("Incoming req from Client: parse request from sock %d\n", i);
					parse_request(buf_pts[i]); //set req_count, and push request into req_queue
					print_queue(buf_pts[i]->req_queue_p);

					// if there is request in req_queue, dequeue one and open connection to web server and send
					if (buf_pts[i]->req_queue_p->req_count > 0) {

					    FD_SET(temp, &master_write_fds);    //write to server sock
					    dbprintf("Setting %d into master_write_fds\n", temp);
					}

			    } else {
			    	//if recv not successful

					if (recv_ret == -1) {
					    perror("Error! recv_ret -1 when receiving from Client\n");
					} else if ( recv_ret == 0) { 
					    dbprintf("Client_sock %d closed\n",i);
					}

					/* clear up  */
					FD_CLR(i, &master_read_fds);
					FD_CLR(i, &master_write_fds);

					free(buf_pts[i]);
					close_socket(i);
					close_socket(temp);
			    }
			}else{
				///////////////////////////
				//income data from web server
				//////////////////////////
				if (recv_ret == 1){
					temp = serverSock2ClientSock(buf_pts, i, maxfd);    //temp is client sock
					dbprintf("Incoming data from Server: parse data from sock %d\n", i);
					parse_request(buf_pts[temp]); //set req_count, and push request into req_queue
					print_queue(buf_pts[temp]->req_queue_p);

					// if there is request in req_queue, dequeue one and open connection to web server and send
					if (buf_pts[i]->req_queue_p->req_count > 0) {
					    FD_SET(temp, &master_write_fds);    //write to client sock
					    dbprintf("Setting %d into master_write_fds\n", temp);
					}

			    } else {
			    	//if recv not successful

					if (recv_ret == -1) {
					    perror("Error! recv_ret -1 when receiving from Server\n");
					} else if ( recv_ret == 0) { 
					    dbprintf("Server_sock %d closed\n",i);
					}

					/* clear up  */
					FD_CLR(temp, &master_read_fds);
					FD_CLR(temp, &master_write_fds);

					free(buf_pts[temp]);
					close_socket(i);
					close_socket(temp);
			    }
			}

		    
		} // end i == socket
		} // end FD_ISSET read_fds
	    
	    /* check fd in write_fds  */
	    if (FD_ISSET(i, &write_fds)) {
	    	isClient = isClientSock(buf_pts, i, maxfd);

	    	if (isClient == -1){
				printf("Cannot find socket when writing");
				return EXIT_FAILURE;
	    	}else if (isClient == 1){
				//////////////////////////
				//outgoing data to client (browser)
				//////////////////////////
	    		temp = buf_pts[i]->server_sock;  //temp is server sock
	    		dbprintf("creating response to client sock %d\n", i);
				if (create_response(i, buf_pts[i]) > 0) {
					// have some content in the buffer to send
				    dbprintf("to client: buf is not empty, send response\n");
				    send_response(temp, buf_pts[i]);
				    
				} else {
				    dbprintf("to client: no more content to create, stop sending, reset buf\n");
				    
				    // clear up
				    buf_pts[i]->res_fully_sent = 1;
				    reset_buf(buf_pts[i]); // do not free the buf, keep it for next read

				    if (buf_pts[i]->req_queue_p->req_count == 0) {
				    	FD_CLR(i, &master_write_fds);
				    	reset_buf(buf_pts[temp]);
						dbprintf("Server: req_count reaches 0, FD_CLR %d from write_fds\n\n",i);
				    }
				}
	    	}else{
	    		//////////////////////////
				//outgoing req to web server
				//////////////////////////
				temp = serverSock2ClientSock(buf_pts, i, maxfd);  //temp is client sock
	    		dbprintf("creating request to server sock %d\n", i);
				if (create_response(temp, buf_pts[temp]) > 0) {
					// have some content in the buffer to send
				    dbprintf("to server: buf is not empty, send response\n");
				    send_response(i, buf_pts[temp]);
				    
				} else {
				    dbprintf("to server: no more content to create, stop sending, reset buf\n");
				    // clear up
				    buf_pts[temp]->res_fully_sent = 1;
				    reset_buf(buf_pts[temp]); // do not free the buf, keep it for next read

				    if (buf_pts[temp]->req_queue_p->req_count == 0) {
				    	FD_CLR(i, &master_write_fds);
						reset_buf(buf_pts[temp]);
						dbprintf("Server: req_count reaches 0, FD_CLR %d from write_fds\n\n",i);
				    }
				}
	    	}
	    	//					FD_SET(server_sock, &master_read_fds);

		    
	    } // end FD_ISSET write_fds

	}// end for i


    }//while

    /* clear up  */
    close_socket(master_sock);
    clear_buf_array(buf_pts, maxfd);

    return EXIT_SUCCESS;
}


void clear_buf_array(struct buf *buf_pts[], int maxfd){

    int i;

    for (i = 0; i <= maxfd; i++ ) {
	if (buf_pts[i]->allocated == 1) {
	    dbprintf("before clear_buf send, alloc:%d\n", buf_pts[i]->allocated);
	    //	    clear_buf(buf_pts[i]);
	    free(buf_pts[i]);
	    dbprintf("before clear_buf send, alloc:%d\n", buf_pts[i]->allocated);
	}
    }

}

