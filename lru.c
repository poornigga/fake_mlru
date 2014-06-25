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


void _renew_node(node *n) ;
int _lru_revert_node(lru_mgt *mgt, node *n) ;
int _lru_rm_node (lru_mgt *mgt, node *n) ;

int curtime(void) {
    time_t t; time(&t);
    return (int)t;
}

/* a-z, A-Z*/
int hash_func(char s) {
    if (s < 'a')
        return abs(s - 'A') % 26;
    return (s - 'a' ) % 26;
}

void *flush_fn(void *arg) {
    p_info("flush thread : [%s]\n", "in flush fn");
    lru_mgt *mgt = (lru_mgt *) arg;
    while(1) {
        pthread_mutex_lock(&dirty_list_lock);
        if (cond == 0) {
            p_info("cond waiting  ...... ");
            pthread_cond_wait(&dirty_cond, &dirty_list_lock);
        }

        p_info("dirty Count: %d\n", mgt->dirty_count);
        node *n = mgt->dirty;
        while(n) {
            printf ( "thread [%ld] : flush dirty node to disk : \n", (long)pthread_self() );
            // ...........
            // add write disk logic.
            // ...........
            node_dump(n);
            n = n->next;
        }


        // move freshed node to mgt. update mgt state.
        pthread_rwlock_wrlock(&mgt->grwlock);
        node *new = mgt->dirty;
        mgt->dirty = NULL;
        while(new) {
            printf ( "move refreshed node to mgt.\n" );
            node *next = new->next;

            _renew_node(new);
            _lru_revert_node(mgt, new);
            new = next;
        }
        pthread_rwlock_unlock(&mgt->grwlock);

        // unlock 
        cond = 0;
        pthread_mutex_unlock(&dirty_list_lock);

        sleep(1);
    }
}

void _renew_node(node *n) {
    if (NULL == n) {
        return;
    }

    pthread_rwlock_wrlock(&n->rwlock);
    n->data[0] = '\0';
    n->dlen = 0;
    n->hint = 1;
    n->actime = 0;
    pthread_rwlock_unlock(&n->rwlock);
}

void flush_signal(void) {
    if (cond == 0) {
        pthread_mutex_lock(&dirty_list_lock);
        cond = 1;
        pthread_mutex_unlock(&dirty_list_lock);
        pthread_cond_signal(&dirty_cond);
    }
}

int lru_init(lru_mgt **mgt, size_t max_node) {
    if (max_node < 1) {
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
    pthread_rwlock_init(&mg->grwlock , NULL);

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
    pthread_create(&flush_thread, NULL, &flush_fn, *mgt);

    return 0;
}

void lru_destructor(lru_mgt **mgt) {

    pthread_cond_destroy(&dirty_cond);
    printf ( "destroy cond. [success]\n" );

    pthread_mutex_destroy(&dirty_list_lock);
    printf ( "destroy mutex. [success]\n" );

    // api join just waiting thread-self success return.
    // cancel exit the thread ..
    // pthread_join(flush_thread, NULL);
    pthread_cancel(flush_thread);

    printf ( "cancel flush thread. [success]\n" );

    if (NULL == mgt) return ;

    node *n = (node *)(*mgt)->head;
    pthread_rwlock_destroy(&n->rwlock);
    n = n->next;
    while(n != (*mgt)->head) {
        pthread_rwlock_destroy(&n->rwlock);
        n = n->next;
    }
    printf ( "destroy rwlocks. [success] \n" );

    pthread_rwlock_destroy(&(*mgt)->grwlock);
    printf ( "destroy mgt rwlocks. [success] \n" );

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


int _lru_revert_node(lru_mgt *mgt, node *n) {
    if (NULL == n) {
        return -1;
    }
    pthread_rwlock_wrlock(&mgt->grwlock);

    node *t = mgt->tail;
    n->next = t->next;
    t->next = n;
    n->prev = t;
    t->next->prev = n;
    mgt->tail = n;
    mgt->full = 0;
    mgt->dirty_count --;

    pthread_rwlock_unlock(&mgt->grwlock);

    return 0;
}

// remove node from mgt
// [add node to dirty chian]
int _lru_rm_node (lru_mgt *mgt, node *n) {
    if (NULL == n) {
        p_err("ptr null\n");
        return -1;
    }

    if (mgt->head == n) {
        mgt->head = n->next;
    }
    if (mgt->tail == n) {
        mgt->tail = n->prev;
    }
    if (mgt->cold == n) {
        mgt->cold = n->next;
    }
    
    n->prev->next = n->next;
    n->next->prev = n->prev;
    n->next = NULL;
    n->prev = NULL;

    // clear access-tag.
    n->hint = 1;

    if (_hash_del_node(mgt, n) == NULL){
        p_err("hash del node faild \n");
        return -1;
    }
    return  0;
}

int _node_mv_dirty(lru_mgt *mgt, node *n) {
    if (NULL == n) {
        p_err("add dirty chian node null\n");
        return -1;
    }

    if (_lru_rm_node(mgt, n) == -1) {
        p_err("lru mgt rm node error!\n");
        return -1;
    }

    n->next = mgt->dirty;
    mgt->dirty = n;
    mgt->count --;
    mgt->dirty_count ++;


    if (mgt->dirty_count > (mgt->total / 3)) {
        p_info("dirty node count > total/3\n automatic flush dirty chain\n");
        flush_signal();
    }

    return 0;
}

int lru_replace(lru_mgt *mgt, void *data, int dlen) {

    // if full, tail must be point to the last node.
    node *n = mgt->tail;

    while (n) {
        // if access time >=2; insert to hot-head and set hint to default;
        if (n->hint >= 2) {
            mgt->head = mgt->head->prev;
            mgt->tail = mgt->tail->prev;
            mgt->head->hint = 1;
            mgt->head->actime = curtime();

            n = mgt->tail;
            continue;
        } else if (n->hint == -1) {  
            // just only repalce node action triggered, do mv node to dirty chain;
            n = n->prev;
            _node_mv_dirty(mgt, n->next);
            continue;
        }

        // else, current node-data will be replace, add new node to cold-head;
        
        // node_dump(n);
        _hash_del_node(mgt, n);

        n->dlen = dlen;
        memcpy(n->data, data, dlen);
        n->data[dlen] = '\0';
        n->actime = curtime();
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
    node *n = mgt->tail;

    pthread_rwlock_wrlock(&n->rwlock);
    n->actime = curtime();
    n->hint = 1;
    n->dlen = dlen;
    memcpy(n->data, (char *)data, dlen);
    pthread_rwlock_unlock(&n->rwlock);

    _hash_add_node(mgt, n);

    mgt->tail = n->next;
    mgt->count ++;


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
        printf ( "replace cold node .....\n" );
        return lru_replace(mgt, data, dlen);
    } else {
        printf ( "append to tail...\n" );
        return lru_append(mgt, data, dlen);
    }

    return -1;
}

node *lru_idx_query(lru_mgt *mgt, int idx) {
    node *n = mgt->head;
    do {
        if (n->idx == idx) return n;
        n = n->next;
    } while (n != mgt->head);
    return NULL;
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
    printf ( "::: LRU buffer hash map dump : \n" );
    printf ( ":::  [ Total : <%d>,  Full : <%s> ]\n", mgt->total, mgt->full==1?"YES":"NO" );

    int i = 0;
    node *n = mgt->head;
    for (i=0; i<26; ++i) {
        n = mgt->map[i];
        if (n == NULL) {
            continue;
        }
        printf("\n+--[ %c ]--+ \n", (char)(i+'a') );
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
    printf ( "::: LRU buffer chain : \n" );
    printf ( "::: [ Total : <%d>, Used : <%d>  Full : <%s> ]\n", mgt->total, mgt->count,  mgt->full==1?"YES":"NO" );

    node *n = mgt->head;
    do {
        node_dump(n);
        n = n->next;
    // actime > 0 ; filter the unused node.
    } while(n != mgt->head && n->actime > 0); 

    // dirty chain
    printf ( "\n======================================\n" );
    printf ( "::: LRU buffer dirty chain :\n" );
    n = mgt->dirty;
    if (n == NULL) {
        printf ( "[ NULL ]\n" );
    }
    while (n) {
        node_dump(n);
        n= n->next;
    }
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

    return 0;
}

int manual_flush_dirty_data(lru_mgt *mgt) {
    if (NULL == mgt) {
        return -1;
    }

    p_info("current thread : [%ld]\n", (long)pthread_self());
    p_info("manual flush dirty chain, current node count: [%d]\n", mgt->dirty_count);
    if (mgt->dirty_count > 0) {
        flush_signal();
    }

    return 0;
}

void access_node(node *n) {
    if (NULL == n ) return;
    if (pthread_rwlock_wrlock(&n->rwlock) < 0) {
        p_err("pthread wrlock error\n");
        return ;
    }
    sleep(1);
    n->actime = curtime();
    n->hint ++;
    pthread_rwlock_unlock(&n->rwlock);
}

// edit node, just update node; 
// node rehash node ?
int edit_node (lru_mgt *mgt, node *n, char *post) {
    if (NULL == n  || NULL == post) {
        p_err("input ptr null \n");
        return -1;
    }
    if (pthread_rwlock_wrlock(&n->rwlock) < 0) {
        p_err("pthread wrlock error\n");
        return -1;
    }

    _hash_del_node(mgt, n);
    
    n->dlen = strlen(post);
    n->dlen = n->dlen < MAX_DLEN ? n->dlen :  (MAX_DLEN - 1) ;
    memcpy(n->data, post, n->dlen);
    n->data[n->dlen] = '\0';
    n->hint = -1; // updated node hint must be -1;
    n->actime = curtime();
    pthread_rwlock_unlock(&n->rwlock);


    _hash_add_node(mgt, n);
    return 0;
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

/*
   pm-file-format
   ---------------------
   node_count
   node_len node_data
   node_len node_data
   node_len node_data
   node_len node_data
 */
#define MAX_CACHE_LEN (1024*4)
int freeze_data (lru_mgt *mgt, int cnt) {
    node *n = mgt->head;
    int ret = -1, i, len;
    char *sptr;
    u16  *iptr;
    char *buff = malloc(MAX_CACHE_LEN);
    if (NULL == buff) {
        p_err("malloc error.\n");
        return ret;
    }

    iptr = (u16 *)buff;
    *iptr = mgt->count;
    iptr ++;
    sptr = (char *)iptr;
    for (i=0; i<mgt->count; ++i) {
        *iptr = strlen(n->data);
        iptr ++;
        sptr = (char *)iptr;
        memcpy(sptr, n->data, *(iptr-1));
        n = n->next;
        sptr += *(iptr-1);
        iptr = (u16 *) sptr;
    }

    len = sptr - buff;
    if ((ret = storage(buff, len)) != 0) {
        p_err("storage error.");
        safe_free(buff);
        return ret;
    }

    p_info("storage data. [success]");
    safe_free(buff);
    return 0;
}

int unfreeze_data (lru_mgt *mgt, int cnt) {
    if (NULL == mgt) return -1;
    
    int ret = -1, len, i;
    u16  *iptr;
    char *ptr;
    char *caches = malloc(MAX_CACHE_LEN);
    if (NULL == caches ) {
        p_err("malloc Error.\n");
        return ret;
    }

    ptr = caches;
    if ((ret = restore(ptr, MAX_CACHE_LEN)) != 0) {
        p_err("restore data error.\n");
        safe_free(caches);
        return ret;
    }

    iptr = (u16 *)ptr;
    len = *(u16 *)ptr;
    iptr ++;
    for (i=0; i<len; ++i) {
        ptr = (char *)(iptr + 1);
        lru_add_data(mgt, ptr,  *iptr);
        ptr += *iptr;
        iptr = (u16 *)ptr;
    }

    safe_free(caches);
    return 0;
}

