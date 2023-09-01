/**
 * \file    conn_thread.c
 * \author  Looi Kian Seong
 * \date    September 2, 2023
 * \brief   Function implementation for socket server connection thread creation
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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "conn_thread.h"

bool spawn_connection_thread (pthread_t *thread_id, void *(*func)(void* params), char client_ipv4[16], 
                              int cfd, int output_fd, pthread_mutex_t *mutex, 
                              ConnThreadParams_t **thread_params_ptr, int *error_code)
{
    if ((thread_id == NULL) || (func == NULL) || (mutex == NULL) || 
        (thread_params_ptr == NULL) || (error_code == NULL))
    {
        return false;
    }

    ConnThreadParams_t *thread_params = (ConnThreadParams_t *)malloc(sizeof(ConnThreadParams_t));

    if (thread_params == NULL)
    {
        return false;
    }

    memcpy(thread_params->client_ipv4, client_ipv4, sizeof(thread_params->client_ipv4));
    thread_params->client_fd = cfd;
    thread_params->output_fd = output_fd;
    thread_params->mutex = mutex;
    thread_params->done = false;

    *error_code = pthread_create(thread_id, NULL, func, (void *)thread_params);

    if (*error_code != 0)
    {
        free(thread_params);
        return false;
    }

    *thread_params_ptr = thread_params;

    return true;
}
