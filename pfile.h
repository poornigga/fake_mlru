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
#include <assert.h>

#include "util.h"

/* --storage-file-fmt--
 *
    FSDS 4 (char)  // start-tag
    4 int  // file_length 
    4 int  // node_count  
    node_len 2 int
    data 
    node_len 2 int
    data
    ......
    FSDS
 *
 */

typedef struct {
    char stag[4];
    char etag[4];
    u32  flen;
    u16  node_num;
} wfmt;



#endif
