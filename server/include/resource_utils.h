/**
 * \file    resource_utils.h
 * \author  Looi Kian Seong
 * \date    September 1, 2023
 * \brief   Function prototypes for utilities used for resources management
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

#ifndef RESOURCE_UTILS_H_
#define RESOURCE_UTILS_H_

#include <stdbool.h>
#include <netdb.h>

typedef struct
{
    void **allocated_mem;
    unsigned int num_of_allocated_mem;
    unsigned int max_num_of_allocated_mem;

    int *open_file_fd;
    unsigned int num_of_open_file;
    unsigned int max_num_of_open_file;
} ResourcesCollector_t;

void initialize_resource_collector (ResourcesCollector_t *collector, 
                                    void **allocated_mem_container, 
                                    unsigned int allocated_mem_container_size, 
                                    int *open_file_fd_container, 
                                    unsigned int open_file_container_size);

void *malloc_wrapper (ResourcesCollector_t *collector, void *buf_ptr, size_t size, int *error_code);

void free_wrapper (ResourcesCollector_t *collector, void *ptr);

bool register_fd (ResourcesCollector_t *collector, int fd);

void close_file_wrapper(ResourcesCollector_t *collector, int fd);

void cleanup (ResourcesCollector_t *collector);

#endif  /* RESOURCE_UTILS_H_ */
