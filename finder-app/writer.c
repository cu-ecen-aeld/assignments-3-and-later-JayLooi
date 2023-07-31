#include <stdio.h>
#include <errno.h>
#include <syslog.h>

const char *ARGS[] =
{
    "",         // placeholder for the executable filename during invocation
    "writefile", 
    "writestr", 
};

const int N_ARGS = sizeof(ARGS) / sizeof(ARGS[0]);

int main(int argc, char **argv)
{
    openlog(NULL, 0, LOG_USER);

    if (argc < N_ARGS)
    {
        syslog(LOG_ERR, "Not enough positional arguments");
        for (int i = argc; i < N_ARGS; ++i)
        {
            syslog(LOG_ERR, "Missing argument %d : %s", i, ARGS[i]);
        }

        closelog();

        return 1;
    }

    FILE *fp = fopen(argv[1], "w");
    int error = errno;
    if (fp == NULL)
    {
        syslog(LOG_ERR, "Unexpected error occurred: %s", strerror(error));
        closelog();
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
    int written = fprintf(fp, argv[2]);

    if (written < 0)
    {
        error = errno;
        syslog(LOG_ERR, "Writing to file failed: %s", strerror(error));
        closelog();
        return 1;
    }

    fclose(fp);

    closelog();

    return 0;
}

