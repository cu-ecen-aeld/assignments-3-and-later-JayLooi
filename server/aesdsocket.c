#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

// String manipulation
#include <string.h>

// Error handling and logging
#include <syslog.h>
#include <errno.h>

// File operations
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// Signal handling
#include <signal.h>

// Socket operations
#include <sys/socket.h>
#include <netdb.h>

#include <poll.h>

#define CLEANUP_BEFORE_RETURN(ret)      \
    cleanup();                          \
    return ret;

static bool isInterruptingSignalReceived = false;
static struct addrinfo *addr_info = NULL;
static int nfd = 0;
static int fds[3] = { -1 , -1, -1 };
static void *allocated_mem = NULL;
static bool isSyslogOpen = false;
const static char tempfile[] = "/var/tmp/aesdsocketdata";
const static int allocated_chunk_size = 4096;

void signal_handler (int sig);
void *malloc_wrapper (size_t size, bool reallocating, int *error_code);
void free_wrapper (void *ptr);
bool register_fd (int fd);
void cleanup (void);

int main (int argc, char *argv[])
{
    bool isDaemon = false;
    if ((argc > 1) && (strcmp(argv[1], "-d") == 0))
    {
        isDaemon = true;
    }

    char port[] = "9000";
    int error_code = 0;
    bool connected = false;

    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;

    struct addrinfo *sockaddrinfo;

    openlog(NULL, 0, LOG_USER);
    isSyslogOpen = true;

    error_code = getaddrinfo(NULL, port, &hint, &sockaddrinfo);

    if (error_code != 0)
    {
        syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(error_code));
        CLEANUP_BEFORE_RETURN(1);
    }

    addr_info = sockaddrinfo;

    int sfd = socket(sockaddrinfo->ai_family, sockaddrinfo->ai_socktype, 
                    sockaddrinfo->ai_protocol);
    
    if (sfd == -1)
    {
        syslog(LOG_ERR, "Socket creation failed! ");
        CLEANUP_BEFORE_RETURN(1);
    }

    register_fd(sfd);

    error_code = bind(sfd, sockaddrinfo->ai_addr, sockaddrinfo->ai_addrlen);

    if (error_code == -1)
    {
        syslog(LOG_ERR, "bind() error: %s", strerror(errno));
        CLEANUP_BEFORE_RETURN(1);
    }

    if (isDaemon)
    {
        pid_t pid = fork();

        switch (pid)
        {
            case 0:
                break;

            case -1:
                syslog(LOG_ERR, "fork() error: %s", strerror(errno));
                CLEANUP_BEFORE_RETURN(1);
                break;
            
            default:
                return 0;
        }
    }

    error_code = listen(sfd, 1);

    if (error_code == -1)
    {
        syslog(LOG_ERR, "listen() error: %s", strerror(errno));
        CLEANUP_BEFORE_RETURN(1);
    }
    
    struct sigaction sigact = { 0 };
    sigact.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sigact, NULL) != 0)
    {
        syslog(LOG_ERR, "sigaction() error for SIGINT: %s", strerror(errno));
        CLEANUP_BEFORE_RETURN(1);
    }

    if (sigaction(SIGTERM, &sigact, NULL) != 0)
    {
        syslog(LOG_ERR, "sigaction() error for SIGTERM: %s", strerror(errno));
        CLEANUP_BEFORE_RETURN(1);
    }

    int outfiled = open(tempfile, O_CREAT | O_RDWR);
    if (outfiled == -1)
    {
        syslog(LOG_ERR, "open() %s file error: %s", tempfile, strerror(errno));
        CLEANUP_BEFORE_RETURN(1);
    }

    register_fd(outfiled);
    
    while (isInterruptingSignalReceived == false)
    {
        struct sockaddr client_addr = { 0 };
        socklen_t client_addrlen = sizeof(client_addr);
        int cfd = accept(sfd, &client_addr, &client_addrlen);

        if (cfd == -1)
        {
            syslog(LOG_ERR, "accept() error: %s", strerror(errno));
            CLEANUP_BEFORE_RETURN(1);
        }

        register_fd(cfd);
        char client_ipv4[16] = { 0 };
        error_code = getnameinfo(&client_addr, client_addrlen, client_ipv4, sizeof(client_ipv4), NULL, 0, NI_NUMERICHOST);
        if (error_code == -1)
        {
            syslog(LOG_ERR, "getnameinfo() error: %s", strerror(errno));
            CLEANUP_BEFORE_RETURN(1);
        }

        syslog(LOG_INFO, "Accepted connection from %s", client_ipv4);
        connected = true;
        int available_space = 0;
        int total_byte_read = 0;
        char *buf = NULL;

        while (connected)
        {
            if (available_space == 0)
            {
                buf = (char *)malloc_wrapper((total_byte_read + allocated_chunk_size), true, &error_code);
                if (buf == NULL)
                {
                    if (error_code != 0)
                    {
                        syslog(LOG_ERR, "malloc() for %d bytes failed with error: %s", 
                               (total_byte_read + allocated_chunk_size), strerror(error_code));
                    }
                    else
                    {
                        syslog(LOG_ERR, "Please free previously allocated memory before allocating for new buffer");
                    }

                    CLEANUP_BEFORE_RETURN(1);
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
                if (errno == EAGAIN)
                {
                    continue;
                }
                
                syslog(LOG_ERR, "recv() error: %s", strerror(errno));
                CLEANUP_BEFORE_RETURN(1);
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
            
            ssize_t written = write(outfiled, buf, total_byte_read);
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

                CLEANUP_BEFORE_RETURN(1);
            }

            free_wrapper(buf);

            struct stat statebuf;
            if (fstat(outfiled, &statebuf) != 0)
            {
                syslog(LOG_ERR, "fstat() error: %s", strerror(errno));
                CLEANUP_BEFORE_RETURN(1);
            }

            int n_byte = statebuf.st_size;
            buf = NULL;
            while (buf == NULL)
            {
                buf = (char *)malloc_wrapper(n_byte, true, &error_code);
                if (buf == NULL)
                {
                    if (error_code == ENOMEM)
                    {
                        n_byte >>= 1;
                    }
                    else
                    {
                        syslog(LOG_ERR, "malloc() error: %s", strerror(error_code));
                        CLEANUP_BEFORE_RETURN(1);
                    }
                }

                if (n_byte == 0)
                {
                    syslog(LOG_ERR, "System run out of memory to be allocated! ");
                    CLEANUP_BEFORE_RETURN(1);
                }
            }

            if (lseek(outfiled, 0, SEEK_SET) == -1)
            {
                syslog(LOG_ERR, "lseek() error: %s", strerror(errno));
                CLEANUP_BEFORE_RETURN(1);
            }

            int iteration = (statebuf.st_size + n_byte - 1) / n_byte;
            
            for (int i = 0; i < iteration; ++i)
            {
                ssize_t n_read = read(outfiled, buf, n_byte);
                ssize_t n_sent = send(cfd, buf, n_read, 0);

                if (n_sent != n_read)
                {
                    if (n_sent == -1)
                    {
                        syslog(LOG_ERR, "send() error: %s", strerror(errno));
                    }
                    else
                    {
                        syslog(LOG_ERR, "send() interrupted! ");
                    }

                    CLEANUP_BEFORE_RETURN(1);
                }
            }

            free_wrapper(buf);
        }

        if (!connected)
        {
            syslog(LOG_INFO, "Closed connection from %s", client_ipv4);
        }
    }

    CLEANUP_BEFORE_RETURN(0);
}

void signal_handler (int sig)
{
    if ((sig == SIGINT) || (sig == SIGTERM))
    {
        isInterruptingSignalReceived = true;
    }
}

void *malloc_wrapper (size_t size, bool reallocating, int *error_code)
{
    void *buf = NULL;

    if (error_code == NULL)
    {
        return NULL;
    }

    *error_code = 0;

    if (allocated_mem == NULL)
    {
        buf = malloc(size);
    }
    else if (reallocating)
    {
        buf = realloc(allocated_mem, size);
    }
    else
    {
        return NULL;
    }

    if (buf == NULL)
    {
        *error_code = errno;
    }
    else
    {
        allocated_mem = buf;
    }

    return buf;
}

void free_wrapper (void *ptr)
{
    if (ptr != NULL)
    {
        free(ptr);
        allocated_mem = NULL;
    }
}

bool register_fd (int fd)
{
    if (nfd < (sizeof(fds) / sizeof(fds[0])))
    {
        fds[nfd] = fd;
        ++nfd;
        return true;
    }

    return false;
}

void cleanup (void)
{
    if (isInterruptingSignalReceived)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
    }

    if (nfd > 0)
    {
        for (int i = 0; i < nfd; ++i)
        {
            if (fds[i] != -1)
            {
                if (close(fds[i]) != 0)
                {
                    syslog(LOG_WARNING, 
                           "File with fd %d couldn't be closed. Error message: %s", 
                           fds[i], strerror(errno));
                }

                fds[i] = -1;
            }
        }
    }

    nfd = 0;

    if (access(tempfile, F_OK) == 0)
    {
        if (remove(tempfile) != 0)
        {
            syslog(LOG_WARNING, "%s couldn't be removed. Error message: %s", 
                tempfile, strerror(errno));
        }
    }

    if (allocated_mem != NULL)
    {
        free_wrapper(allocated_mem);
    }

    if (addr_info != NULL)
    {
        freeaddrinfo(addr_info);
        addr_info = NULL;
    }

    if (isSyslogOpen)
    {
        closelog();
        isSyslogOpen = false;
    }
}
