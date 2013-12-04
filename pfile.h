/*
 * =====================================================================================
 *
 *       Filename:  pfile.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013/12/03 14时18分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _PFILE_H_
#define _PFILE_H_
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "util.h"

#define safe_free(b)  \
    if (b!=NULL) free(b); b = NULL; 

#define safe_close(fp) \
    if (fp != NULL) close(fp); fp = NULL;
    

int storage(char *buf, int len ) ;
int restore(char *buf, int size) ;
int storaged(void);


#endif
