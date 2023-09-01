/**
 * \file    socket_server.c
 * \author  Looi Kian Seong
 * \date    September 2, 2023
 * \brief   Functions implementation for socket server creation and 
 *          accepting connection
 * 
 * This project is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License as published by the Open Source
 * Initiative. See the LICENSE file or visit:
 * https://opensource.org/licenses/MIT
 * 
 * Copyright (c) 2023 Looi Kian Seong 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 */

#include <stddef.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "socket_server.h"

int create_socket_server (char *port, int *sfd, int *error_code)
{
    int rc = 0;
    int ret = SOCKET_SERVER_SETUP_OK;
    struct addrinfo *sockaddrinfo = NULL;
    struct addrinfo hint;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;

    if ((port == NULL) || (sfd == NULL) || (error_code == NULL))
    {
        ret = SOCKET_SERVER_INVALID_PARAM;
    }
    else
    {
        *error_code = 0;
        rc = getaddrinfo(NULL, port, &hint, &sockaddrinfo);

        if (rc != 0)
        {
            *error_code = rc;
            ret = SOCKET_SERVER_GET_ADDRINFO_FAILED;
        }

        *sfd = socket(sockaddrinfo->ai_family, sockaddrinfo->ai_socktype, 
                      sockaddrinfo->ai_protocol);
        
        if (*sfd == -1)
        {
            *error_code = errno;
            ret = SOCKET_SERVER_CREATE_FAILED;
        }
        
        rc = bind(*sfd, sockaddrinfo->ai_addr, sockaddrinfo->ai_addrlen);

        if (rc == -1)
        {
            *error_code = errno;
            ret = SOCKET_SERVER_BIND_FAILED;
        }

        freeaddrinfo(sockaddrinfo);
    }

    // close socket fd on error that happened after socket creation
    if (ret > SOCKET_SERVER_CREATE_FAILED)
    {
        close(*sfd);
    }

    return ret;
}

int wait_connection (int sfd, int *cfd, struct sockaddr *client_addr, socklen_t *client_addrlen, int *error_code)
{
    int ret = SOCKET_SERVER_WAIT_CONN_OK;

    if ((cfd == NULL) || (client_addr == NULL) || (client_addrlen == NULL) || (error_code == NULL))
    {
        ret = SOCKET_SERVER_INVALID_PARAM;
    }
    else
    {
        *client_addrlen = sizeof(*client_addr);
        *cfd = accept(sfd, client_addr, client_addrlen);

        if (*cfd == -1)
        {
            *error_code = errno;
            ret = SOCKET_SERVER_WAIT_CONN_FAILED;
        }
    }

    return ret;
}
