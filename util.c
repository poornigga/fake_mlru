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
  p_do(stdout, "info", fmt, ap);
  va_end(ap);
}

void p_warn(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  p_do(stderr, "warn", fmt, ap);
  va_end(ap);
}

void p_err(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  p_do(stderr, "error", fmt, ap);
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

