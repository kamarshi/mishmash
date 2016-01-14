#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#define OK  0

#define ERR -1


extern pthread_t thread_parser;

extern pthread_t thread_filler;

/*
 * Reader side API
 */
/*
 * Public API function
 */
extern void  init_buf_ctrl(int infd);


/*
 * Public API parser thread
 */
extern void*  gps_parse(void* param);


/*
 * Public API buf filler thread
 */
extern void*  buf_filler(void* param);


/*
 * Writer side API
 */
extern int  fill_pipe(int pfd, int ffd);

#endif // GPS_PARSER_H
