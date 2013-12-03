/*
 * =====================================================================================
 *
 *       Filename:  util.c
 *
 *    Description:  utils
 *
 *        Version:  1.0
 *        Created:  2013/11/21 17时31分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "util.h"

static void p_do(FILE *stream,
                  const char *label,
                  const char *fmt,
                  va_list ap);

void p_info(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  p_do(stdout, "[info]:", fmt, ap);
  va_end(ap);
}

void p_warn(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  p_do(stderr, "[warn]:", fmt, ap);
  va_end(ap);
}

void p_err(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  p_do(stderr, "[error]:", fmt, ap);
  va_end(ap);
}

static void p_do(FILE *stream,
                  const char *label,
                  const char *fmt,
                  va_list ap) {
  char fmtbuf[1024];
  vsnprintf(fmtbuf, sizeof(fmtbuf), fmt, ap);

  fprintf(stream, "%s:%s: %s\n", 
#if defined(__APPLE__) && defined(__MACH__)
          getprogname(), 
#else
          _getprogname(), 
#endif
          label, fmtbuf);
}


#define FPATH "./metadata/plist.me"

int storage(char *buf, int len) {
    if (buf == NULL) {
        p_err("arg error [%s]\n" ,"prt null");
        return -1;
    }
    int fd = open (FPATH, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fd<0) return -1;
    int ret = write (fd, buf, len) ;
    p_info("writed : [%d], buflen : [%d]\n", ret, len );
    close(fd);
    return 0;
}

int restore(char *buf, int size) {
    if (buf == NULL) {
        p_err("arg error [%s]\n" ,"prt null");
        return -1;
    }
    int fd = open(FPATH, O_RDONLY);
    if (fd < 0) return -1;
    read(fd, buf, size);

    close(fd);

    return 0;
}

