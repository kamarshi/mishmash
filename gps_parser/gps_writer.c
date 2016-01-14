#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "./gps_buffer.h"

// Simulating 4800 baud data rate
#define  BR_BUF_SIZE  600

/*
 * This is the child process main.  It reads from the
 * input file (ffd) and writes to the pipe (pfd)
 * Note!: This readbuf is in a different process
 * from the readbuf in parser code
 */
int  fill_pipe(int pfd, int ffd)
{
    int actual;

    int retval;

    while (1) {
        if ((actual = read(ffd, readbuf, BR_BUF_SIZE)) != 0) {
            printf("Bytes read from file: %d\n", actual);

            if ((retval = write(pfd, readbuf, actual)) < 0) {
                printf("Error writing pipe: %d\n", errno);

                exit(-1);
            }
            printf("Bytes written to pipe: %d\n", retval);

            sleep(1);
        }
        else
            break;
    }
    printf("Writer done writing to pipe\n");

    return 0;
}

