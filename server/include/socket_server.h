/**
 * \file    socket_server.h
 * \author  Looi Kian Seong
 * \date    September 2, 2023
 * \brief   Function prototypes for socket server creation and 
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

#ifndef SOCKET_SERVER_H_
#define SOCKET_SERVER_H_

#include <sys/socket.h>

#define SOCKET_SERVER_SETUP_OK                      (0)
#define SOCKET_SERVER_GET_ADDRINFO_FAILED           (1)
#define SOCKET_SERVER_CREATE_FAILED                 (2)
#define SOCKET_SERVER_BIND_FAILED                   (3)

#define SOCKET_SERVER_WAIT_CONN_OK                  (0)
#define SOCKET_SERVER_WAIT_CONN_FAILED              (1)

#define SOCKET_SERVER_INVALID_PARAM                 (-1)

int create_socket_server (char *port, int *sfd, int *error_code);

int wait_connection (int sfd, int *cfd, struct sockaddr *client_addr, socklen_t *client_addrlen, int *error_code);

#endif  /* SOCKET_SERVER_H_ */
