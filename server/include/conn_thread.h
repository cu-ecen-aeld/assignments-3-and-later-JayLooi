/**
 * \file    conn_thread.h
 * \author  Looi Kian Seong
 * \date    September 2, 2023
 * \brief   Function prototype for socket server connection thread creation
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

#ifndef CONN_THREAD_H_
#define CONN_THREAD_H_

#include <pthread.h>

typedef struct
{
    char client_ipv4[16];
    int client_fd;
    int output_fd;
    pthread_mutex_t *mutex;
    bool done;
} ConnThreadParams_t;

bool spawn_connection_thread (pthread_t *thread_id, void *(*func)(void* params), char client_ipv4[16], 
                              int cfd, int output_fd, pthread_mutex_t *mutex, 
                              ConnThreadParams_t **thread_params_ptr, int *error_code);

#endif  /* CONN_THREAD_H_ */
