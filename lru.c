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

static pthread_t flush_thread;
static pthread_mutex_t dirty_list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t dirty_cond = PTHREAD_COND_INITIALIZER;
static u8 cond = 0;


/* a-z */
int hash_func(char s) {
    return (s - 'a' ) % 26;
}

void *flush_fn(void *arg) {
    p_info("flush thread : [%s]\n", "in flush fn");
    while(1) {
        pthread_mutex_lock(&dirty_list_lock);
        if (cond == 0) {
            p_info("cond faiid, wiat in while...[%s]", "+++");
            pthread_cond_wait(&dirty_cond, &dirty_list_lock);
        }
        p_info("thread - [%d] : working .....", flush_thread);
        cond = 0;
        pthread_mutex_unlock(&dirty_list_lock);
        sleep(1);
    }
}

void flush_signal(void) {
    if (cond == 0) {
        pthread_mutex_lock(&dirty_list_lock);
        cond = 1;
        pthread_mutex_unlock(&dirty_list_lock);
        pthread_cond_signal(&dirty_cond);
    }
}

int lru_buff_init(lru_mgt **mgt, size_t max_node) {
    if (max_node <= 0) {
        return -1;
    }

    u16 node_len = sizeof(node) + MAX_DLEN;

    lru_mgt *mg = NULL;
    int memsize = sizeof(lru_mgt) + node_len * max_node;
    char *ptr;
    char *buf = malloc(memsize);
    if (NULL == buf) {
        printf ( "alloc Error. \n");
        return -1;
    }
    memset(buf, 0, memsize);
    ptr = buf;

    mg = (lru_mgt *)ptr;
    mg->total = max_node;
    mg->count = 0;
    mg->full = 0;
    mg->msize = memsize;

    ptr += sizeof(lru_mgt);
    mg->head = (node *)ptr;
    mg->tail = mg->head; // tail point to the last un-empty node.
    mg->cold = mg->tail;
    mg->func = &hash_func;

    ptr += node_len;
    for (int i=1; i<max_node-1; ++i) {
        node *nd = (node *)ptr;
        pthread_rwlock_init(&nd->rwlock, NULL);
        nd->idx = i;
        nd->prev = (node *)((char *)ptr - node_len);
        nd->next = (node *)((char *)ptr + node_len);
        ptr += node_len;
    }

    // the last node.
    node *n = (node *)ptr;
    n->next = mg->head;
    n->idx = max_node-1;
    pthread_rwlock_init(&n->rwlock, NULL);
    n->prev = (node *)((char *)ptr - node_len);

    // the first node.
    node *last = n; // save the first node ptr.
    mg->head->prev = last;
    mg->head->idx  = 0;
    pthread_rwlock_init(&last->rwlock, NULL);
    mg->head->next = (node *)((char *)mg->head + node_len);

    *mgt = mg;

    // create background flush thread;
    pthread_create(&flush_thread, NULL, &flush_fn, NULL);

    return 0;
}

void lru_buff_destructor(lru_mgt **mgt) {

    pthread_join(flush_thread, NULL);

    pthread_cond_destroy(&dirty_cond);

    pthread_mutex_destroy(&dirty_list_lock);

    if (NULL == mgt) return ;

    node *n = (node *)(*mgt)->head;
    pthread_rwlock_destroy(&n->rwlock);
    n = n->next;
    while(n != (*mgt)->head) {
        pthread_rwlock_destroy(&n->rwlock);
        n = n->next;
    }

    free(*mgt);
    *mgt = NULL;
}

node *_hash_del_node(lru_mgt *mgt, node *n) {
    if (NULL == n) return NULL;
    int idx = mgt->func((char)n->data[0]);

    if (n->hp == NULL) {
        mgt->map[idx] = n->hn;
        if (n->hn != NULL ) {
            n->hn->hp = NULL;
        }
        n->hn = NULL;
    } else {
        n->hp->hn = n->hn;
        if (n->hn != NULL) {
            n->hn->hp = n->hp;
            n->hn = NULL;
        }
        n->hp = NULL;
    }
    return n;
}

int _hash_add_node(lru_mgt *mgt, node *n) {
    if (NULL == n) {
        return -1;
    }

    int idx = mgt->func((char)n->data[0]);
    node *t = mgt->map[idx];
    if (t == NULL) {
        mgt->map[idx] = n;
        return 0;
    }

    while (t->hn != NULL) {
        t = t->hn;
    }

    t->hn = n;
    n->hp = t;

    return 0;
}


int lru_replace(lru_mgt *mgt, void *data, int dlen) {

    time_t ctime ; time(&ctime);
    // if full, tail must be point to the last node.
    node *n = mgt->tail;
    //  if all-node-hint > 2, what we can do now ?????
    // node *minimum_hint = mgt->tail;

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

        // else, current node-data will be replace, add new node to cold-head;
        
        // node_dump(n);
        _hash_del_node(mgt, n);

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

        _hash_add_node(mgt, n);

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
    mgt->count ++;

    // 4 test
    if (mgt->count % 5 == 0) {
        p_info("cond simulator : [%d]", mgt->count) ;
        flush_signal();
    }

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

/* hash-dump */
void lru_hdump(lru_mgt *mgt) {
    printf ( "\n====================================\n" );
    printf ( "::: LRU buffer hash dump : \n" );
    printf ( "::: < total : [%d],  isFull : [%s] >\n", mgt->total, mgt->full==1?"True":"False" );

    int i = 0;
    node *n = mgt->head;
    for (i=0; i<26; ++i) {
        n = mgt->map[i];
        if (n == NULL) {
            continue;
        }
        printf("\n+ map <%c> + \n", (char)(i+'a') );
        while(n != NULL) {
            printf("\t  |-> %s\n", n->data);
            n = n->hn;
        }
    }
    printf ( "\n======================================\n" );
}

/* list-dump */
void lru_dump(lru_mgt *mgt) {
    printf ( "\n====================================\n" );
    printf ( "::: LRU buffer list dump : \n" );
    printf ( "::: < total : [%d],  isFull : [%s] >\n", mgt->total, mgt->full==1?"True":"False" );

    node *n = mgt->head;
    do {
        printf ( "idx : %.3d,\thint : %d,\ttime : %ld,\tdata : [%s]\n", n->idx, n->hint, n->actime, n->data );
        n = n->next;
    } while(n != mgt->head && n->hint > 0) ;
    printf ( "\n======================================\n" );
}

/* debug fake-data */
int prepare_fake_data(lru_mgt *mgt, char **data, int dcount) {
    if (NULL == mgt || NULL == data) {
        p_err("%s:%s\n", "[error]:", "null ptr.");
        return -1;
    }
    int ct = dcount >= mgt->total ? mgt->total : dcount;
    for ( int i=0; i<ct; ++i ) {
        lru_add_data (mgt, data[i], strlen(data[i]));
    }

    storage(mgt, mgt->msize);

    return 0;
}

void access_node(node *n) {
    if (NULL == n ) return;
    sleep(1);
    time_t t; time(&t);
    n->actime = t;
    n->hint ++;
}

node *access_data(lru_mgt *mgt, char *query) {
    if (NULL == query) return NULL;

    node *n = lru_query(mgt, (void *)query, strlen(query));
    if (NULL == n) {
        printf ( "no data be found. [%s]\n", query );
        // add insert logic.
        return NULL;
    }
    access_node(n);
    return n;
}

void node_dump(node *n) {
    if (NULL == n) {
        return;
    }
    printf ( "idx : %.3d,\thint : %d,\ttime : %ld,\tdata : [%s]\n", n->idx, n->hint, n->actime, n->data );
}


