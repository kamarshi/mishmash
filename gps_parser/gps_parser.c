#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "./gps_parser.h"
#include "./gps_buffer.h"


/*
 * If parser thread goes to wait on an empty
 * readbuf, wake it up when this level is reached
 */
#define PARSE_THOLD 256

pthread_t thread_parser;

pthread_t thread_filler;


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
 * buffer.  While copying, if readbuf runs out,
 * parser thread goes to sleep, waiting for filler
 * thread to wake it up
 */
typedef struct bufCtrl_t {
    // input file handle
    int   infd;

    // buffer write pointer
    int   windx;

    // buffer read pointer
    int   rindx;

    /*
     * Lock to ensure reader and writer serialization
     */
    pthread_mutex_t bufLock;

    /*
     * Condition variable to wake up sleeping parser
     */
    pthread_cond_t  bufReady;

    // bytes left in buffer
    int   left;

    // Found end of data - parser
    int   peod;

    // Found end of data - filler
    int   feod;
} bufCtrl;


bufCtrl rbCtrl;


/*
 * Public API function
 */
void  init_buf_ctrl(int infd)
{
    rbCtrl.infd = infd;

    rbCtrl.windx = 0;

    rbCtrl.rindx = 0;

    pthread_cond_init(&rbCtrl.bufReady, NULL);

    pthread_mutex_init(&rbCtrl.bufLock, NULL);

    rbCtrl.left = 0;

    rbCtrl.peod = 0;

    rbCtrl.feod = 0;
}


/*
 * Following function executes in fillter thread
 * context. It reads bytes from read side of pipe
 * and fills readbuf.  If readbuf level crosses
 * PARSE_THOLD, a pthread_cond_signal is emitted
 * to wake up a potentially sleeping parser thread
 */
void* buf_filler(void* param)
{
    unsigned char nxt_byte = '\0';

    int  count;

    printf("Starting up buf filler thread\n");

    while (rbCtrl.feod == 0) {
        count = read(rbCtrl.infd, &nxt_byte, 1);

        pthread_mutex_lock(&rbCtrl.bufLock);

        if (count == 0) {
            printf("Warning!: Missing end-of-data marker?\n");

            nxt_byte = '\0';

            rbCtrl.feod = 1;
        }
        // If this happens also to be at PARSE_THOLD, two
        // bufReady signals will be generated.  This is okay,
        // as these condition variables don't keep state
        if (nxt_byte == END_OF_DATA) {
            pthread_cond_signal(&rbCtrl.bufReady);

            nxt_byte = '\0';

            rbCtrl.feod = 1;
        }
        readbuf[rbCtrl.rindx++] = (char) nxt_byte;

        rbCtrl.left++;

        if (rbCtrl.left == RD_BUF_SIZE) {
            printf("Error!: Buffer overflow - exiting\n");

            exit(-1);
        }
        if (rbCtrl.left == PARSE_THOLD)
            pthread_cond_signal(&rbCtrl.bufReady);

        pthread_mutex_unlock(&rbCtrl.bufLock);

        rbCtrl.rindx %= RD_BUF_SIZE;
    }
    printf("Buf filler thread exiting\n");
}


/*
 * All functions below execute in parser thread
 * context
 */
/*
 * Called by parser thread, which will go to
 * sleep if zero bytes are left
 */
char get_next_byte(void)
{
    char nxt_byte;

    pthread_mutex_lock(&rbCtrl.bufLock);

    if (rbCtrl.left) {

        rbCtrl.left--;

        nxt_byte = readbuf[rbCtrl.windx++];

        pthread_mutex_unlock(&rbCtrl.bufLock);

        rbCtrl.windx %= RD_BUF_SIZE;
    }
    else {
        // Buffer is empty, go to sleep until some bytes
        // are collected, or end-of-data
        // Atomically also unlocks mutex
        if (rbCtrl.feod == 1) {
            // No more data forthcoming, so quit
            rbCtrl.peod = 1;

            pthread_mutex_unlock(&rbCtrl.bufLock);

            return '\0';
        }
        pthread_cond_wait(&rbCtrl.bufReady, &rbCtrl.bufLock);

        // If here, buf is locked and we have data
        // even if it is end of data marker
        if (rbCtrl.left == 0) {
            printf("Buffer control error!\n");

            printf("rindx: %d; windx: %d; left: %d\n", rbCtrl.rindx, rbCtrl.windx, rbCtrl.left);

            exit(-1);
        }
        rbCtrl.left--;

        nxt_byte = readbuf[rbCtrl.windx++];

        pthread_mutex_unlock(&rbCtrl.bufLock);

        rbCtrl.windx %= RD_BUF_SIZE;
    }
    return nxt_byte;
}


int  is_eod()
{
    return rbCtrl.peod;
}


int  get_next_line(char* pLine)
{
    int  isCR  = 0;

    char  data;

    char* pLn = pLine;

    while (!is_eod()) {
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
    // must be eod
    return END_OF_DATA;
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
void*  gps_parse(void* param)
{
    char line[256];

    size_t length;

    int retval;

    locTime  lt;

    char*    pA;

    char*    pB;

    char*    pC;

    FILE*    outfp;

    sentID   sid;

    printf("Parser thread starting\n");

    outfp = (FILE*) param;

    sid = INVLD;

    while (1) {
        if ((retval = get_next_line(line)) == END_OF_DATA) {
            printf("Parser thread exiting at end of data\n");

            pthread_exit(NULL);
        }

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
