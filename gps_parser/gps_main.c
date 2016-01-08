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

    int infd;

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

    if ((infd = open(infname, O_RDONLY)) < 0) {
        printf("Failed to open file %s for reading\n", infname);

        exit(0);
    }

    if ((outfp = fopen(outfname, "w+")) == NULL) {
        printf("Failed to open file %s for writing\n", outfname);

        exit(0);
    }

    init_buf_ctrl(infd);

    gps_parse(outfp);

    // close files
}
