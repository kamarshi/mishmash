#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#define OK  0

#define ERR -1


/*
 * Public API function
 */
extern void  init_buf_ctrl(int infd);


/*
 * Public API function
 */
extern int  gps_parse(FILE* outfp);

#endif // GPS_PARSER_H
