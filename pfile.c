/*
 * =====================================================================================
 *
 *       Filename:  pfile.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013/12/03 14时18分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "pfile.h"

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

#define max_buf_length 4096
int freeze_data (char **buf, int count) {
    assert(buf);
    char *wbuf = malloc(max_buf_length);
    assert(wbuf);
    cahr *ptr = wbuf;
    memcpy(ptr, "FSDS", 4);
    ptr[4] = '\0';
    (int)*ptr = 0;
    
}
