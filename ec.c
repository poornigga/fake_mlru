/*
 * =====================================================================================
 *
 *       Filename:  ec.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013/11/29 16时10分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "ec.h"
#include "util.h"

char *encode(char *inbuf, int len) {
    if (NULL == inbuf ) {
        p_err("invalid arg [%s]\n", "ptr NULL");
        return NULL;
    }
    return inbuf;
}

char *decode(char *inbuf, int len) {
    if (NULL == inbuf ) {
        p_err("invalid arg [%s]\n", "ptr NULL");
        return NULL;
    }
    return inbuf;
}
