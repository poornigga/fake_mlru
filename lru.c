/*
 * =====================================================================================
 *
 *       Filename:  mlru.c
 *
 *    Description:  lru alog .
 *
 *        Version:  1.0
 *        Created:  2013/10/21 11时13分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "lru.h"

int hash_func(char s) {
    return (s - 'a' ) % 26;
}

int lru_buff_init(lru_mgt **mgt, size_t max_node) {
    if (max_node <= 0) {
        return -1;
    }

    u16 node_len = sizeof(node) + MAX_DLEN;

    lru_mgt *mg = NULL;
    int memsize = sizeof(lru_mgt) + node_len * max_node;
    char *ptr;
    char *buf = (char *)malloc(memsize);
    if (NULL == buf) {
        printf ( "alloc Error. \n");
    }
    memset(buf, 0, memsize);
    ptr = buf;

    mg = (lru_mgt *)ptr;
    mg->total = max_node;
    mg->memsize = memsize;
    mg->full = 0;

    ptr += sizeof(lru_mgt);
    mg->head = (node *)ptr;
    mg->tail = mg->head; // tail point to the last un-empty node.
    mg->cold = mg->tail;
    mg->func = &hash_func;

    ptr += node_len;
    for (int i=1; i<max_node-1; ++i) {
        node *nd = (node *)ptr;
        nd->idx = i;
        nd->prev = (node *)((char *)ptr - node_len);
        nd->next = (node *)((char *)ptr + node_len);
        ptr += node_len;
    }

    // the last node.
    node *n = (node *)ptr;
    n->next = mg->head;
    n->idx = max_node-1;
    n->prev = (node *)((char *)ptr - node_len);

    // the first node.
    node *last = n; // save the first node ptr.
    mg->head->prev = last;
    mg->head->idx  = 0;
    mg->head->next = (node *)((char *)mg->head + node_len);

    *mgt = mg;
    return 0;
}

void lru_buff_destructor(lru_mgt **mgt) {
    if (NULL == mgt) return ;
    free(*mgt);
    *mgt = NULL;
}

int _hash_add_node(lru_mgt *mgt, node *n) {
    if (NULL == n) {
        return -1;
    }

    int idx = mgt->func((char)n->data[0]);
    node *hn = mgt->map[idx];
    if (hn == NULL) {
        mgt->map[idx] = n;
        n->hp = NULL;
        n->hn = NULL;
        return 0;
    }

    while (hn->hn != NULL) {
        hn = hn->hn;
    }

    hn->hn = n;
    n->hp = hn;
    n->hn = NULL;

    return 0;
}

// not use
int _lru_node_exchage(node *a, node *b) {
    if (NULL == a || NULL == b) return -1;

    node *tp = a->prev, *tn = a->next;
    a->next = b->next; a->prev = b->prev;
    b->prev = tp; b->next = tn;

    return 0;
}

int lru_replace(lru_mgt *mgt, void *data, int dlen) {

    time_t ctime ; time(&ctime);
    // if full, tail must be point to the last node.
    node *n = mgt->tail;

    while (n) {
        // if access time >=2; insert to hot-head and set hint to default;
        if (n->hint >= 2) {
            mgt->head = mgt->head->prev;
            mgt->tail = mgt->tail->prev;
            mgt->head->hint = 1;
            mgt->head->actime = ctime;

            n = mgt->tail;
            continue;
        }

        // node_dump(n);
        // else, current node-data will be replace, add new node to cold-head;
        n->dlen = dlen;
        memcpy(n->data, data, dlen);
        n->data[dlen] = '\0';
        n->actime = ctime;
        n->hint = 1;

        mgt->tail = n->prev;
        mgt->tail->next = mgt->head;
        mgt->head->prev = mgt->tail;

        node *p = mgt->cold->prev;
        p->next = n;
        n->prev = p;
        n->next = mgt->cold;
        mgt->cold->prev = n;

        mgt->cold = n;

        break;
    }
    return 0;
}

int _lru_set_cold(lru_mgt *mgt) {
    if (NULL == mgt) return -1;

    int jump = (int)((mgt->total + 1) / 2) ;

    while(jump > 0) {
        mgt->cold = mgt->cold->next;
        jump--;
    }
    return 0;
}

int lru_append(lru_mgt *mgt, void *data, int dlen) {
    time_t t ; time(&t);

    node *n = mgt->tail;

    n->actime = t;
    n->hint = 1;
    n->dlen = dlen;
    memcpy(n->data, (char *)data, dlen);
    _hash_add_node(mgt, n);

    mgt->tail = n->next;

    // if last node, tail-pointer not need move to next;
    if (mgt->tail == mgt->head) {
        mgt->full = 1;
        mgt->tail = n;
        // set cold point to the middle of list.
        _lru_set_cold(mgt);
    } 
    return 0;
}

int lru_add_data (lru_mgt *mgt, void *data, int dlen) {
    if (NULL == mgt || data == NULL ) return -1;

    if(mgt->full) {
        printf ( "lru eliminate node .....\n" );
        return lru_replace(mgt, data, dlen);
    } else {
        return lru_append(mgt, data, dlen);
    }

    return -1;
}


/* query using hash-ptr */
node *lru_query(lru_mgt *mgt, void *data, int dlen) {
    if (NULL == data || dlen == 0) return NULL;

    char *ptr = (char *)data;
    u8 idx = mgt->func(ptr[0]);
    if (idx < 0) {
        printf ( "mgt-func not found\n" );
        return NULL;
    }

    node *n = mgt->map[idx];
    if (n->hn == NULL)
        return n;

    n = n->hn;
    while(n) {
       if (strncmp(n->data, (char *)data, dlen) == 0)
           return n;

       n = n->hn;
    }

    printf ( "finally, not found.\n" );
    return NULL;
}

void lru_hdump(lru_mgt *mgt) {
    printf ( "\n====================================\n" );
    printf ( "::: LRU buffer dump : \n" );
    printf ( "== total : %d\tisFull : %d\n", mgt->total, mgt->full );

    int i = 0;
    node *n = mgt->head;
    printf ( ":::hash order :\n" );
    for (i=0; i<26; ++i) {
        n = mgt->map[i];
        printf("++ map[%c] : \n", (char)(i+'a') );
        while(n != NULL) {
            printf("\t>>> %s\n", n->data);
            n = n->hn;
        }
    }
    printf ( "\n======================================\n" );
}

void lru_dump(lru_mgt *mgt) {
    printf ( "\n====================================\n" );
    printf ( "::: LRU buffer dump : \n" );
    printf ( "== total : %d\tisFull : %d\n", mgt->total, mgt->full );

    node *n = mgt->head;
    printf ( ":::list order :\n" );
    do {
        printf ( "idx : %.3d,\thint : %d,\ttime : %ld,\tdata : [%s]\n", n->idx, n->hint, n->actime, n->data );
        n = n->next;
    } while(n != mgt->head) ;
    
    printf ( "\n======================================\n" );
}

int prepare_data(lru_mgt *mgt, char **data, int dcount) {
    if (NULL == mgt || NULL == data) {
        return -1;
    }
    int ct = dcount >= mgt->total ? mgt->total : dcount;
    for ( int i=0; i<ct; ++i ) {
        lru_add_data (mgt, data[i], strlen(data[i]));
    }

    return 0;
}

node *access_data(lru_mgt *mgt, char *query) {
    if (NULL == query) return NULL;

    sleep(1);

    time_t t; time(&t);

    node *n = lru_query(mgt, (void *)query, strlen(query));
    if (NULL == n) {
        printf ( "no data be found. [%s]\n", query );
        // add insert logic.
        return NULL;
    }

    n->actime = t;
    n->hint++;
    return n;
}

void node_dump(node *n) {
    if (NULL == n) {
        return;
    }
    printf ( "idx : %.3d,\thint : %d,\ttime : %ld,\tdata : [%s]\n", n->idx, n->hint, n->actime, n->data );
}


