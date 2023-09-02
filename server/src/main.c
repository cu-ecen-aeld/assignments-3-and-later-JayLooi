/**
 * \file    main.c
 * \author  Looi Kian Seong
 * \date    September 3, 2023
 * \brief   Main logic implementation to create a socket server, 
 *          listen, accept and spawn new thread for incoming connections, 
 *          receive bytes from client sockets and return the full payload back. 
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>

#include "socket_server.h"
#include "conn_thread.h"
#include "resource_utils.h"

#define CLEAN_RETURN(collector, ret_code)       \
    cleanup(&collector);                        \
    return ret_code;

#define TIMESTAMP_PREFIX            "timestamp:"
#define TIMESTAMP_PREFIX_LEN        (sizeof(TIMESTAMP_PREFIX) - 1UL)

typedef enum
{
    SYSTEM_STATE_INIT, 
    SYSTEM_STATE_SOCK_CREATED, 
    SYSTEM_STATE_SOCK_START_LISTENING, 
    SYSTEM_STATE_SOCK_WAITING_CONN, 
} SystemState_t;

typedef struct ThreadNode
{
    pthread_t thread_id;
    ConnThreadParams_t *thread_params;
    SLIST_ENTRY(ThreadNode) node;
} ThreadNode_t;

typedef struct
{
    int output_fd;
    pthread_mutex_t *mutex;
} TimerThreadParams_t;

SLIST_HEAD(slisthead, ThreadNode);

const static char tempfile[] = "/var/tmp/aesdsocketdata";
const static int allocated_chunk_size = 4096;
const static char rfc2822_compliant_datetime_format[] = "%a, %d %b %Y %T %z\n";

static bool interrupt_signal_received = false;
static SystemState_t system_state = SYSTEM_STATE_INIT;

void signal_handler (int sig);
void timer_handler (union sigval sigval);
void handle_completed_threads (struct slisthead *thread_list_head);
void *socket_connection_thread (void *params);

int main (int argc, char *argv[])
{
    ResourcesCollector_t main_thread_res_collector;
    int open_file_fd[3] = { -1 , -1, -1 };
    bool run_as_daemon = false;
    char port[] = "9000";
    int sfd = -1;
    int cfd = -1;
    int output_fd = -1;
    int rc = 0;
    int error_code = 0;
    struct sockaddr client_addr = { 0 };
    socklen_t client_addrlen = sizeof(client_addr);
    struct sigaction sigact = { 0 };
    pthread_mutex_t output_file_mutex;
    struct slisthead thread_list_head;
    struct slisthead *thread_list_head_ptr = &thread_list_head;
    ThreadNode_t *last_thread_node = NULL;
    bool unexpected_error = false;
    struct sigevent sev;
    TimerThreadParams_t timer_thread_params;
    timer_t timer_id = 0;
    struct itimerspec its;

    openlog(NULL, 0, LOG_USER);

    initialize_resource_collector(&main_thread_res_collector, NULL, 0U, 
                                  open_file_fd, sizeof(open_file_fd) / sizeof(open_file_fd[0]));

    rc = pthread_mutex_init(&output_file_mutex, NULL);
    if (rc != 0)
    {
        syslog(LOG_ERR, "mutex init error: %s", strerror(rc));
        closelog();
        return 1;
    }
    
    SLIST_INIT(thread_list_head_ptr);

    output_fd = open(tempfile, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (output_fd == -1)
    {
        syslog(LOG_ERR, "open() %s file error: %s", tempfile, strerror(errno));
        closelog();
        return 1;
    }

    register_fd(&main_thread_res_collector, output_fd);

    sigact.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sigact, NULL) != 0)
    {
        syslog(LOG_ERR, "sigaction() error for SIGINT: %s", strerror(errno));
        closelog();
        return 1;
    }

    if (sigaction(SIGTERM, &sigact, NULL) != 0)
    {
        syslog(LOG_ERR, "sigaction() error for SIGTERM: %s", strerror(errno));
        closelog();
        return 1;
    }

    if ((argc > 1) && (strcmp(argv[1], "-d") == 0))
    {
        run_as_daemon = true;
    }
    
    memset(&sev, 0, sizeof(sev));
    timer_thread_params.mutex = &output_file_mutex;
    timer_thread_params.output_fd = output_fd;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = (void *)&timer_thread_params;
    sev.sigev_notify_function = timer_handler;

    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    while ((interrupt_signal_received == false) && (unexpected_error == false))
    {
        switch (system_state)
        {
        case SYSTEM_STATE_INIT:
            rc = create_socket_server(port, &sfd, &error_code);
            switch (rc)
            {
            case SOCKET_SERVER_SETUP_OK: 
                syslog(LOG_INFO, "Socket server created! ");
                register_fd(&main_thread_res_collector, sfd);
                system_state = SYSTEM_STATE_SOCK_CREATED;
                break;
            
            case SOCKET_SERVER_GET_ADDRINFO_FAILED: 
                syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(error_code));
                break;
            
            case SOCKET_SERVER_CREATE_FAILED: 
                syslog(LOG_ERR, "Socket creation error: %s", strerror(error_code));
                break;
            
            case SOCKET_SERVER_BIND_FAILED: 
                syslog(LOG_ERR, "socket binding error: %s", strerror(error_code));
                break;
            
            case SOCKET_SERVER_INVALID_PARAM: 
                syslog(LOG_ERR, "Invalid param for create_socket_server()");
                break;
            
            default:
                break;
            }
            break;
        
        case SYSTEM_STATE_SOCK_CREATED:
            if (run_as_daemon)
            {
                pid_t pid = fork();
                switch (pid)
                {
                case 0:
                    system_state = SYSTEM_STATE_SOCK_START_LISTENING;
                    break;

                case -1:
                    syslog(LOG_ERR, "fork() error: %s", strerror(errno));
                    break;
                
                default:
                    return 0;
                }
            }
            else
            {
                system_state = SYSTEM_STATE_SOCK_START_LISTENING;
            }

            if (system_state == SYSTEM_STATE_SOCK_START_LISTENING)
            {
                rc = timer_create(CLOCK_MONOTONIC, &sev, &timer_id);
                if (rc != 0)
                {
                    syslog(LOG_ERR, "timer creation failed: %s", strerror(errno));
                    cleanup(&main_thread_res_collector);
                    closelog();
                    return 1;
                }

                rc = timer_settime(timer_id, 0, &its, NULL);
                if (rc != 0)
                {
                    syslog(LOG_ERR, "timer creation failed: %s", strerror(errno));
                    timer_delete(timer_id);
                    cleanup(&main_thread_res_collector);
                    closelog();
                    return 1;
                }
            }
            break;
        
        case SYSTEM_STATE_SOCK_START_LISTENING:
            rc = listen(sfd, SOMAXCONN);
            if (rc == -1)
            {
                syslog(LOG_ERR, "listen() error: %s", strerror(errno));
            }
            else
            {
                system_state = SYSTEM_STATE_SOCK_WAITING_CONN;
            }
            break;
        
        case SYSTEM_STATE_SOCK_WAITING_CONN:
            rc = wait_connection(sfd, &cfd, &client_addr, &client_addrlen, &error_code);
            if (rc == SOCKET_SERVER_WAIT_CONN_OK)
            {
                char client_ipv4[16] = { 0 };
                pthread_t thread_id;
                rc = getnameinfo(&client_addr, client_addrlen, client_ipv4, sizeof(client_ipv4), NULL, 0, NI_NUMERICHOST);
                if (rc == -1)
                {
                    syslog(LOG_ERR, "getnameinfo() error: %s", strerror(errno));
                }
                else
                {
                    syslog(LOG_INFO, "Accepted connection from %s", client_ipv4);
                }

                handle_completed_threads(thread_list_head_ptr);

                ConnThreadParams_t *thread_params = NULL;
                if (spawn_connection_thread(&thread_id, socket_connection_thread, client_ipv4, cfd, 
                                            output_fd, &output_file_mutex, &thread_params, &error_code))
                {
                    ThreadNode_t *thread_node = (ThreadNode_t *)malloc(sizeof(ThreadNode_t));
                    if (thread_node == NULL)
                    {
                        syslog(LOG_ERR, "malloc for next thread id failed: %s", strerror(errno));
                        unexpected_error = true;
                    }
                    else
                    {
                        thread_node->thread_id = thread_id;
                        thread_node->thread_params = thread_params;
                        if (SLIST_EMPTY(thread_list_head_ptr))
                        {
                            SLIST_INSERT_HEAD(thread_list_head_ptr, thread_node, node);
                        }
                        else
                        {
                            SLIST_INSERT_AFTER(last_thread_node, thread_node, node);
                        }

                        last_thread_node = thread_node;
                    }
                }
                else
                {
                    syslog(LOG_ERR, "New thread creation failed: %s", strerror(error_code));
                    unexpected_error = true;
                }
            }
            else
            {
                syslog(LOG_ERR, "connection accept error: %s", strerror(error_code));
            }
            break;
        }
    }

    if (interrupt_signal_received)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
    }

    ThreadNode_t *tmp_node = thread_list_head_ptr->slh_first;
    while (tmp_node)
    {
        pthread_join(tmp_node->thread_id, NULL);
        free(tmp_node->thread_params);
        SLIST_REMOVE(thread_list_head_ptr, tmp_node, ThreadNode, node);
        free(tmp_node);
        tmp_node = thread_list_head_ptr->slh_first;
    }

    pthread_mutex_destroy(&output_file_mutex);
    cleanup(&main_thread_res_collector);
    timer_delete(timer_id);
    closelog();

    if (unexpected_error)
    {
        return 1;
    }

    return 0;
}

void signal_handler (int sig)
{
    if ((sig == SIGINT) || (sig == SIGTERM))
    {
        interrupt_signal_received = true;
    }
}

void timer_handler (union sigval sigval)
{
    char cur_timestamp[64] = TIMESTAMP_PREFIX;
    TimerThreadParams_t *thread_params = (TimerThreadParams_t *)sigval.sival_ptr;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    size_t n_byte = strftime(&cur_timestamp[TIMESTAMP_PREFIX_LEN], sizeof(cur_timestamp) - TIMESTAMP_PREFIX_LEN, 
                                rfc2822_compliant_datetime_format, tm);

    pthread_mutex_lock(thread_params->mutex);
    ssize_t written = write(thread_params->output_fd, cur_timestamp, strlen(cur_timestamp));
    pthread_mutex_unlock(thread_params->mutex);
}

void handle_completed_threads (struct slisthead *thread_list_head)
{
    if (thread_list_head == NULL)
    {
        return;
    }

    ThreadNode_t *node;
    ThreadNode_t *prev_node = thread_list_head->slh_first;
    SLIST_FOREACH(node, thread_list_head, node)
    {
        if (node->thread_params->done)
        {
            pthread_join(node->thread_id, NULL);
            free(node->thread_params);
            SLIST_REMOVE(thread_list_head, node, ThreadNode, node);
            free(node);
            node = prev_node;
        }
        else
        {
            prev_node = node;
        }
    }
}

void *socket_connection_thread (void *params)
{
    ConnThreadParams_t *thread_params = (ConnThreadParams_t *)params;
    ResourcesCollector_t conn_thread_res_collector;
    void *allocated_mem_container[] = { NULL, NULL };
    int open_file_fd[] = { -1 };
    bool connected = true;
    int error_code = 0;
    int available_space = 0;
    int total_byte_read = 0;
    char *buf = NULL;
    int cfd = thread_params->client_fd;
    int output_fd = thread_params->output_fd;
    char *client_ipv4 = thread_params->client_ipv4;
    pthread_mutex_t *mutex = thread_params->mutex;

    initialize_resource_collector(&conn_thread_res_collector, allocated_mem_container, 
                                  sizeof(allocated_mem_container) / sizeof(allocated_mem_container[0]), 
                                  open_file_fd, sizeof(open_file_fd) / sizeof(open_file_fd[0]));
    register_fd(&conn_thread_res_collector, cfd);
    
    while (connected)
    {
        if (available_space == 0)
        {
            buf = (char *)malloc_wrapper(&conn_thread_res_collector, buf, 
                                         (total_byte_read + allocated_chunk_size), 
                                         &error_code);
            if (buf == NULL)
            {
                if (error_code != 0)
                {
                    syslog(LOG_ERR, "malloc() for %d bytes failed with error: %s", 
                           (total_byte_read + allocated_chunk_size), strerror(error_code));
                }

                CLEAN_RETURN(conn_thread_res_collector, NULL);
            }

            available_space = allocated_chunk_size;
        }
        
        ssize_t n_read = recv(cfd, &buf[total_byte_read], available_space, MSG_DONTWAIT);

        if (n_read == 0)
        {
            connected = false;
            break;
        }

        if (n_read == -1)
        {
            syslog(LOG_ERR, "recv() error: %s", strerror(errno));
            continue;
        }

        total_byte_read += n_read;
        available_space -= n_read;

        if ((buf[total_byte_read - 1] != '\0') && (buf[total_byte_read - 1] != '\n'))
        {
            continue;
        }
        
        if (buf[total_byte_read - 1] == '\0')
        {
            buf[total_byte_read - 1] = '\n';
        }
        
        pthread_mutex_lock(mutex);
        ssize_t written = write(output_fd, buf, total_byte_read);
        pthread_mutex_unlock(mutex);
        if (written != total_byte_read)
        {
            if (written == -1)
            {
                syslog(LOG_ERR, "write() error: %s", strerror(errno));
            }
            else
            {
                syslog(LOG_ERR, "write() interrupted! ");
            }

            CLEAN_RETURN(conn_thread_res_collector, NULL);
        }

        free_wrapper(&conn_thread_res_collector, buf);
        available_space = 0;
        total_byte_read = 0;

        struct stat statebuf;
        pthread_mutex_lock(mutex);
        if (fstat(output_fd, &statebuf) != 0)
        {
            pthread_mutex_unlock(mutex);
            syslog(LOG_ERR, "fstat() error: %s", strerror(errno));
            CLEAN_RETURN(conn_thread_res_collector, NULL);
        }
        pthread_mutex_unlock(mutex);

        int n_byte = statebuf.st_size;
        buf = NULL;
        while (buf == NULL)
        {
            buf = (char *)malloc_wrapper(&conn_thread_res_collector, buf, n_byte, &error_code);
            if (buf == NULL)
            {
                if (error_code == ENOMEM)
                {
                    n_byte >>= 1;
                }
                else
                {
                    syslog(LOG_ERR, "malloc() error: %s", strerror(error_code));
                    CLEAN_RETURN(conn_thread_res_collector, NULL);
                }
            }

            if (n_byte == 0)
            {
                syslog(LOG_ERR, "System run out of memory to be allocated! ");
                CLEAN_RETURN(conn_thread_res_collector, NULL);
            }
        }

        pthread_mutex_lock(mutex);
        if (lseek(output_fd, 0, SEEK_SET) == -1)
        {
            pthread_mutex_unlock(mutex);
            syslog(LOG_ERR, "lseek() error: %s", strerror(errno));
            CLEAN_RETURN(conn_thread_res_collector, NULL);
        }

        int iteration = (statebuf.st_size + n_byte - 1) / n_byte;
        
        for (int i = 0; i < iteration; ++i)
        {
            ssize_t n_read = read(output_fd, buf, n_byte);
            ssize_t n_sent = send(cfd, buf, n_read, 0);

            if (n_sent != n_read)
            {
                pthread_mutex_unlock(mutex);
                if (n_sent == -1)
                {
                    syslog(LOG_ERR, "send() error: %s", strerror(errno));
                }
                else
                {
                    syslog(LOG_ERR, "send() interrupted! ");
                }

                CLEAN_RETURN(conn_thread_res_collector, NULL);
            }
        }

        if (lseek(output_fd, 0, SEEK_END) == -1)
        {
            pthread_mutex_unlock(mutex);
            syslog(LOG_ERR, "lseek() error: %s", strerror(errno));
            CLEAN_RETURN(conn_thread_res_collector, NULL);
        }
        pthread_mutex_unlock(mutex);

        free_wrapper(&conn_thread_res_collector, buf);
    }

    if (!connected)
    {
        syslog(LOG_INFO, "Closed connection from %s", client_ipv4);
    }

    thread_params->done = true;

    CLEAN_RETURN(conn_thread_res_collector, NULL);;
}
