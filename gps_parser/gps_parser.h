#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#define OK  0

#define ERR -1


/*
 * Reader side API
 */
/*
 * Public API function
 */
extern void  init_buf_ctrl(int infd);


/*
 * Public API function
 */
extern int  gps_parse(FILE* outfp);

/*
 * Writer side API
 */
extern int  fill_pipe(int pfd, int ffd);

#endif // GPS_PARSER_H
