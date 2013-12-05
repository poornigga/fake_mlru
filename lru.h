/*
 * =====================================================================================
 *
 *       Filename:  lru.h
 *
 *    Description:  header
 *
 *        Version:  1.0
 *        Created:  2013/10/24 11时28分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/* 
 *--- mgt intro : --
 *  + ---- + ---- + ---- + ---- + ---- + ---- + 
 *  |  a1  |  a2  |  a3  |  a4  |  a5  |  a6  |  
 *  + ---- + ---- + ---- + ---- + ---- + ---- + 
 *    ^                                    ^
 *    |                                    |
 *   head                                 tail
 *                  [cold] ----------     cold
 * ---------------------------------------------
 *   hash-table[func('c')] = [a1, a4]
 *   hash-table[func('x')] = [a3, a5, a6]
 *   hash-table .......
 */  

#ifndef _LRU_H
#define _LRU_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "ec.h"
#include "util.h"
#include "pfile.h"

#define MAX_NUM  8
#define MAX_DLEN 256

#define MAX_KEY  128

typedef int (*hfunc)(char s);
struct _node_ ;
#pragma pack(push)
#pragma pack(1)
typedef struct _lru_buffer_ {
    struct _node_ *head, *cold, *tail;
    struct _node_ *dirty; // dirty list; node->next
    u32 msize; // mem size
    u16 total; // total node.
    u16 count; // cur used node.
    u8 full;   // buff full or not.
    struct _node_ *map[26]; // a-z
    hfunc func;
} lru_mgt;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct _node_ {
    // cycle list.
    struct _node_ *next, *prev; // for store, access change the order. 
    // uncycle list.
    struct _node_ *hn, *hp;     // for fast query.
    pthread_rwlock_t rwlock; // rwlock
    u8  idx ;        // for test
    time_t actime;   // last access time.
    u8  hint;        // access times.
    u16 dlen;        // data length.
    char data[0];    // data.
} node;
#pragma pack(pop)


int lru_buff_init(lru_mgt **mgt, size_t max_node) ;
void lru_buff_destructor(lru_mgt **mgt) ;

int lru_add_data(lru_mgt *mgt, void *data, int dlen) ;
node *lru_query(lru_mgt *mgt, void *data, int dlen) ;
node *lru_idx_query(lru_mgt *mgt, int idx) ;

void lru_hdump(lru_mgt *mgt) ;
void lru_dump(lru_mgt *mgt) ;

void node_dump(node *n) ;
void access_node(node *n) ;
int edit_node (lru_mgt *mgt, node *n) ;

/* 4debug */
node *access_data(lru_mgt *mgt, char *query) ;
int prepare_fake_data(lru_mgt *mgt, char **data, int dcount) ;

int freeze_data (lru_mgt *mgt, int cnt) ;
int unfreeze_data (lru_mgt *mgt, int cnt) ;

#endif

