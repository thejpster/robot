/* TODO HEADER COMMENT, LICENSING AND ORGANISATION */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "line_follower/server.h"

#define RD_BUFFER_LEN ((2 * PACKET_LEN) - 1)
#define PACKET_HEADER (0xFF)

static int server_socket = 0, client_socket = 0;
static char recv_data[RD_BUFFER_LEN];

/* PRIVATE FUNCTION PROTOTYPES */
static int receive(char * p_data, int buffer_len, int * p_len);

/* Public Functions */

int lf_init(char * path)
{
    unsigned int t;
    int len;
    struct sockaddr_un local, remote;

    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("server_socket ");
        return 1;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(server_socket , (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_socket, 5) == -1) {
        perror("listen");
        return 1;
    }

    printf("Waiting for a connection...\n");
    t = sizeof(remote);
    if ((client_socket = accept4(server_socket , (struct sockaddr *)&remote, &t, SOCK_NONBLOCK)) == -1) {
        perror("accept");
        return 1;
    }
    printf("Client Socket connected.\n");
    return 0;
}

int lf_send(char * p_data, int len)
{
    if(!client_socket)
    {
        perror("Client Socket is unitialised");
        return 1;
    }
    
   if(send(client_socket, p_data, len, 0) < 0)
   {
       perror("send");
       return 1;
    }
    return 0;
}

int lf_close(void)
{
    if(!client_socket)
    {
        close(client_socket);
        return 1;
    }
    return 1;
}

int lf_receive(char * p_data, int * got_data)
{
    int data_len, data_counter, header_pos, ret;

    memset(recv_data, 0, RD_BUFFER_LEN);
    *got_data = 0;
    
    data_counter = 0;
    while (data_counter < PACKET_LEN)
    { 
        ret = receive((char *)&recv_data, PACKET_LEN-data_counter, &data_len);
        if(ret!=0)
        {
            return -1;
        }
        data_counter += data_len;
    }
    
    header_pos = 0;
    while(data_counter <= RD_BUFFER_LEN)
    {
        if(recv_data[header_pos] == PACKET_HEADER)
        {
            memcpy((void *) p_data, (void *) &recv_data[header_pos], PACKET_LEN);
            *got_data = 1;
            break;
        }
        else if (data_counter >= RD_BUFFER_LEN)
        {
            break;
        }

        ret = receive((char *)&recv_data[data_counter], 1, &data_len);
        if(ret!=0)
        {
            return -1;
        }
        if(data_len)
        {
            data_counter += data_len;
            header_pos++;
        }
    }
    
    return 0;
}

/* Private Functions */

static int receive(char * p_data, int buffer_len, int * p_len)
{
    ssize_t len;
    if(!client_socket)
    {
        perror("Client Socket is unitialised");
        return 1;
    }
    
    len = recv(client_socket, p_data, buffer_len, 0); /* Todo handle magic numbers */
    if (len <= 0) 
    {
        *p_len = 0;
        /* We see error codes of -1 when there is no data to be received.
             We still want to keep listening so don't return an error. */
        if (len < -1)
        {
            perror("recv");
            return 1;
        }
    }
    else
    {
        //printf("Got something\n");
        *p_len = (int) len;
    }
    return 0;
}

