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

#define FPATH "./metadata/plist.me"

int file_already_exist ( char *filename, int minisize) ;

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

int file_already_exist ( char *filename, int minisize) {
    if (NULL == filename) {
        p_err("no such file or dictionary\n");
        return -1;
    }
    struct stat buff;
    return (stat(filename, &buff) == 0 && buff.st_size > minisize);
}

int storaged(void) {
    // note : file-format : node-count + node-len + node-data + .... > 6 
    return file_already_exist(FPATH, 6);
}

