/**
 * \file    resource_utils.c
 * \author  Looi Kian Seong
 * \date    September 1, 2023
 * \brief   Functions implementation for utilities used for resources management
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
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "resource_utils.h"

static int is_allocated(ResourcesCollector_t *collector, void *ptr)
{
    if (((collector != NULL) && (collector->allocated_mem != NULL)) && 
        (ptr != NULL))
    {
        for (unsigned int i = 0U; i < collector->num_of_allocated_mem; ++i)
        {
            if (ptr == collector->allocated_mem[i])
            {
                return i;
            }
        }
    }

    return -1;
}

void initialize_resource_collector (ResourcesCollector_t *collector, 
                                    void **allocated_mem_container, 
                                    unsigned int allocated_mem_container_size, 
                                    int *open_file_fd_container, 
                                    unsigned int open_file_container_size)
{
    if ((collector == NULL))
    {
        return;
    }

    memset(collector, 0, sizeof(ResourcesCollector_t));
    
    collector->allocated_mem = allocated_mem_container;
    if (allocated_mem_container == NULL)
    {
        collector->max_num_of_allocated_mem = 0U;
    }
    else
    {
        collector->max_num_of_allocated_mem = allocated_mem_container_size;
    }

    collector->open_file_fd = open_file_fd_container;
    if (open_file_fd_container == NULL)
    {
        collector->max_num_of_open_file = 0U; 
    }
    else
    {
        collector->max_num_of_open_file = open_file_container_size;
    }
}

void *malloc_wrapper (ResourcesCollector_t *collector, void *buf_ptr, size_t size, int *error_code)
{
    unsigned int mem_index = 0;

    if (((collector == NULL) || (collector->allocated_mem == NULL)) || 
        (error_code == NULL))
    {
        return NULL;
    }

    *error_code = 0;

    if (buf_ptr == NULL)
    {
        mem_index = collector->num_of_allocated_mem;

        if (mem_index < collector->max_num_of_allocated_mem)
        {
            buf_ptr = malloc(size);
        }
    }
    else
    {
        mem_index = is_allocated(collector, buf_ptr);

        if (mem_index < collector->num_of_allocated_mem)
        {
            buf_ptr = realloc(buf_ptr, size);
        }
        else
        {
            buf_ptr = NULL;
        }
    }

    if (buf_ptr == NULL)
    {
        *error_code = errno;
    }
    else
    {
        collector->allocated_mem[mem_index] = buf_ptr;

        if (mem_index == collector->num_of_allocated_mem)
        {
            collector->num_of_allocated_mem++;
        }
    }

    return buf_ptr;
}

void free_wrapper (ResourcesCollector_t *collector, void *ptr)
{
    if (((collector == NULL) || (collector->allocated_mem == NULL)) || 
        (ptr == NULL))
    {
        return;
    }

    int mem_index = is_allocated(collector, ptr);

    if (mem_index != -1)
    {
        free(ptr);

        unsigned int last_mem_index = collector->num_of_allocated_mem - 1;
        for (unsigned int i = mem_index; i < last_mem_index; ++i)
        {
            collector->allocated_mem[i] = collector->allocated_mem[i + 1];
        }

        collector->allocated_mem[last_mem_index] = NULL;
        collector->num_of_allocated_mem--;
    }
}

bool register_fd (ResourcesCollector_t *collector, int fd)
{
    if ((collector == NULL) || (collector->open_file_fd == NULL))
    {
        return false;
    }

    if (collector->num_of_open_file < collector->max_num_of_open_file)
    {
        collector->open_file_fd[collector->num_of_open_file] = fd;
        collector->num_of_open_file++;
        return true;
    }

    return false;
}

void close_file_wrapper(ResourcesCollector_t *collector, int fd)
{
    unsigned int fd_index = 0U;
    if ((collector == NULL) || (collector->open_file_fd == NULL))
    {
        return;
    }

    if (collector->num_of_open_file < collector->max_num_of_open_file)
    {
        for (; fd_index < collector->num_of_open_file; ++fd_index)
        {
            if (collector->open_file_fd[fd_index] == fd)
            {
                break;
            }
        }

        if (fd_index < collector->num_of_open_file)
        {
            close(fd);

            unsigned int last_fd_index = collector->num_of_open_file - 1;
            for (unsigned int i = fd_index; i < last_fd_index; ++i)
            {
                collector->open_file_fd[i] = collector->open_file_fd[i + 1];
            }

            collector->open_file_fd[last_fd_index] = -1;
            collector->num_of_open_file--;
        }
    }
}

void cleanup (ResourcesCollector_t *collector)
{
    if ((collector == NULL) || (collector->allocated_mem == NULL) || (collector->open_file_fd == NULL))
    {
        return;
    }

    for (unsigned int i = 0U; i < collector->num_of_allocated_mem; ++i)
    {
        free(collector->allocated_mem[i]);
        collector->allocated_mem[i] = NULL;
    }

    for (unsigned int i = 0U; i < collector->num_of_open_file; ++i)
    {
        close(collector->open_file_fd[i]);
        collector->open_file_fd[i] = -1;
    }

    collector->num_of_allocated_mem = 0U;
    collector->num_of_open_file = 0U;
}
