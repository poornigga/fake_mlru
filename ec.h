/*
 * =====================================================================================
 *
 *       Filename:  ec.h
 *
 *    Description:  ecrypt data api
 *
 *        Version:  1.0
 *        Created:  2013/11/29 16时04分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef _EC_H_
#define _EC_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *encode(char *inbuf, int len);
char *decode(char *inbuf, int len);

#endif
