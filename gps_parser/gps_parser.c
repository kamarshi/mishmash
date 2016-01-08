#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "./gps_parser.h"

#define ENDOFDATA 1

#define RD_BUF_SIZE 1024

typedef enum {
    INVLD,
    GGA,
    GLL,
    RMC
} sentID;


char readbuf[RD_BUF_SIZE];


typedef struct locTime_t {
    char   utc[9];

    double lat;

    double lon;

    int    isValid;
} locTime;


/*
 * Input buffer - readbuf - is controlled by this
 * structure.  Buffer is filled when empty.  Lines'
 * worth of bytes are copied to caller supplied
 * buffer.  While copying, if readbuf runs out, more
 * data is automatically pulled in from infd
 */
typedef struct bufCtrl_t {
    // input file handle
    int   infd;

    // pointer to next byte in buffer
    char* pBuf;

    // bytes left in buffer
    int   left;

    // Found end of file
    int   eof;
} bufCtrl;


bufCtrl rbCtrl;


/*
 * Public API function
 */
void  init_buf_ctrl(int infd)
{
    rbCtrl.infd = infd;

    rbCtrl.pBuf = readbuf;

    rbCtrl.left = 0;

    rbCtrl.eof = 0;
}


void  clr_buf_ctrl(void)
{
    rbCtrl.pBuf = readbuf;

    rbCtrl.left = 0;
}


int  replenish_buf(void)
{
    if (rbCtrl.left != 0)
        return ERR;

    clr_buf_ctrl();

    int actual;

    actual = read(rbCtrl.infd, readbuf, RD_BUF_SIZE);

    rbCtrl.left = actual;

    return actual;
}


char get_next_byte(void)
{
    if (rbCtrl.left) {
        rbCtrl.left--;

        return *rbCtrl.pBuf++;
    }
    else {
        int actual = replenish_buf();

        if (actual) {
            rbCtrl.left--;

            return *rbCtrl.pBuf++;
        }
        else
            rbCtrl.eof = 1;
    }
    // [TODO] Need to figure out if this is a problem
    return '\0';
}


int  is_eof()
{
    return rbCtrl.eof;
}


int  get_next_line(char* pLine)
{
    int  isCR  = 0;

    char  data;

    char* pLn = pLine;

    while (!is_eof()) {
        data = get_next_byte();

        *pLn++ = data;

        // Not checking for 2 <CR> in a row
        // Not checking for lone <LF> either
        if (data == '\r')
            isCR = 1;
        else if ((isCR == 1) && (data == '\n')) {
            *pLn++ = '\0';

            return OK;
        }
    }
    // must be eof
    return ENDOFDATA;
}


/*
 * Check to see if checksum is correct
 */
int  checksum(char* pLine)
{
    char* pToken;

    char* pCs;

    if ((pToken = strchr(pLine, '*')) == NULL)
        return OK;

    pToken = strtok(pLine, "*");

    // Skipping $ sign
    pToken++;

    pCs = strtok(NULL, "\r");

    long int cs = 0;

    while (*pToken != '\0') {
        cs = cs ^ *pToken++;
    }
    long int ref = strtol(pCs, NULL, 16);

    if (cs != ref) {
        printf("Checksum error %lx:%lx\n", cs, ref);

        return ERR;
    }
    else
        return OK;
}


double lat_to_decimal(char* coord, char direction)
{
    char sdeg[3];

    // Covering non-standard fractional part
    char smins[11];

    strncpy(sdeg, coord, 2);

    strncpy(smins, coord+2, 10);

    // Just to be safe
    sdeg[2] = '\0';

    smins[10] = '\0';

    double deg = strtod(sdeg, NULL);

    double mins = strtod(smins, NULL);

    deg = deg + mins/60.0;

    if (direction == 'S')
        return -deg;
    else
        return deg;
}


double lon_to_decimal(char* coord, char direction)
{
    char sdeg[4];

    // Covering non-standard fractional part
    char smins[11];

    strncpy(sdeg, coord, 3);

    strncpy(smins, coord+3, 10);

    // Just to be safe
    sdeg[3] = '\0';

    smins[10] = '\0';

    double deg = strtod(sdeg, NULL);

    double mins = strtod(smins, NULL);

    deg = deg + mins/60.0;

    if (direction == 'W')
        return -deg;
    else
        return deg;
}


int  make_utc(char* pTime, char* pUtc)
{
    *pUtc++ = *pTime++;

    *pUtc++ = *pTime++;

    *pUtc++ = ':';

    *pUtc++ = *pTime++;

    *pUtc++ = *pTime++;

    *pUtc++ = ':';

    *pUtc++ = *pTime++;

    *pUtc++ = *pTime;

    *pUtc = '\0';
}


int  parse_sentence(sentID sid, char* pLine, locTime* pLT)
{
    char* pA;

    char* pB;

    pLT->isValid = 1;

    // [TODO] Could do some format checking here, if we
    // had an idea of the type of errors we would encounter
    switch (sid)
    {
    case RMC:
        // Discard
        pA = strtok(pLine, ",");

        pA = strtok(NULL, ",");

        make_utc(pA, pLT->utc);

        pA = strtok(NULL, ",");

        if (*pA != 'A')
            pLT->isValid = 0;

        pA = strtok(NULL, ",");

        pB = strtok(NULL, ",");

        pLT->lat = lat_to_decimal(pA, *pB);

        pA = strtok(NULL, ",");

        pB = strtok(NULL, ",");

        pLT->lon = lon_to_decimal(pA, *pB);

        break;

    case GLL:
        // Discard
        pA = strtok(pLine, ",");

        pA = strtok(NULL, ",");

        pB = strtok(NULL, ",");

        pLT->lat = lat_to_decimal(pA, *pB);

        pA = strtok(NULL, ",");

        pB = strtok(NULL, ",");

        pLT->lon = lon_to_decimal(pA, *pB);

        pA = strtok(NULL, ",");

        make_utc(pA, pLT->utc);

        pA = strtok(NULL, ",");

        if (*pA != 'A')
            pLT->isValid = 0;

        break;

    case GGA:
        // Discard
        pA = strtok(pLine, ",");

        pA = strtok(NULL, ",");

        make_utc(pA, pLT->utc);

        pA = strtok(NULL, ",");

        pB = strtok(NULL, ",");

        pLT->lat = lat_to_decimal(pA, *pB);

        pA = strtok(NULL, ",");

        pB = strtok(NULL, ",");

        pLT->lon = lon_to_decimal(pA, *pB);

        break;

    default:
        printf("Invalid sentence ID\n");

        return ERR;
    }
    return OK;
}


/*
 * Public API function
 */
int  gps_parse(FILE* outfp)
{
    char line[256];

    size_t length;

    int retval;

    locTime  lt;

    char*    pA;

    char*    pB;

    char*    pC;

    sentID   sid = INVLD;

    while (1) {
        if ((retval = get_next_line(line)) == ENDOFDATA)
            return OK;

        // Process line
        length = strlen(line);

        // Expecting at least $ and <CR><LF>
        if ((length < 3) || (length > 83)) {
            printf("Line has %lu bytes.  Skipping ...\n", length);

            continue;
        }

        retval = checksum(line);

        if (retval == ERR) {
            printf("Failed checksum - skipping ...\n");

            continue;
        }

        sid = INVLD;

        // Find if sid of interest
        pA = strstr(line, "GPGGA");

        pB = strstr(line, "GPGLL");

        pC = strstr(line, "GPRMC");

        if (pA != NULL)
            sid = GGA;
        else if (pB != NULL)
            sid = GLL;
        else if (pC != NULL)
            sid = RMC;

        if (sid != INVLD) {
            retval = parse_sentence(sid, line, &lt);

            if (lt.isValid == 1) {
                fprintf(outfp, "%s, %2.10f, %3.10f\n", lt.utc, lt.lat, lt.lon);
            }
        }
    }
}
