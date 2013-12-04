/*
 * =====================================================================================
 *
 *       Filename:  util.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013/11/26 15时01分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned char     u8;
typedef unsigned short    u16;
typedef unsigned int      u32;
typedef unsigned long     size_t;


void p_info (const char *fmt, ...) ;
void p_warn (const char *fmt, ...) ;
void p_err  (const char *fmt, ...) ;

#endif
