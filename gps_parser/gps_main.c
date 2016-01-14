#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "./gps_parser.h"


main(int argc, char**argv)
{
    int c;

    char* infname;

    char* outfname;

    int inpfd;

    int infd[2];

    pid_t child_pid;

    FILE* outfp;

    opterr = 0;

    while ((c = getopt(argc, argv, "i:o:")) != -1) {
        switch (c)
        {
        case 'i':
            infname = optarg;

            break;

        case 'o':
            outfname = optarg;

            break;

        default:
            printf("Invocation format: \"%s -i <bin_file> -o <txt_file>\"\n", argv[0]);
        }
    }

    if ((inpfd = open(infname, O_RDONLY)) < 0) {
        printf("Failed to open file %s for reading\n", infname);

        exit(0);
    }

    if ((outfp = fopen(outfname, "w+")) == NULL) {
        printf("Failed to open file %s for writing\n", outfname);

        exit(0);
    }

    // infd[0] is reader, infd[1] is writer
    pipe(infd);

    if ((child_pid = fork()) == -1) {
        perror("fork error");

        exit(-1);
    }

    if (child_pid == 0) {
        // This is the child, close reader fd
        close(infd[0]);

        return fill_pipe(infd[1], inpfd);
    }
    else {
        // This is the parent, close writer fd
        close(infd[1]);

        init_buf_ctrl(infd[0]);

        gps_parse(outfp);
    }


    // close files
}
