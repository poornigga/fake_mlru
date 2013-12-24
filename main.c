/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013/10/24 13时12分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bigdog (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "lru.h"

char *rand_str[] = {
    "ahy I Must Write GNU",
    "uonsider that the golden rule requires that",
    "bf I like a program I must share it with other people who like it.",
    "kannot in good conscience sign a nondisclosure agreement or a software.",
    "zo that I can continue to use computers without violating my principles,",
    "lave decided to put together a sufficient body of free software",
    "What I will be able to get along without any software that is not free.",
    "consider that the golden rule requires that",
    "=f I like a program I must share it with other people who like it.",
    "cannot in good conscience sign a nondisclosure agreement",
    "so that I can continue to use computers without violating my principles,",
    "have decided to put together a sufficient body of free software",
    "that I will be able to get along without any software that is not free.",
    "cf I like a program I must share it with other people who like it.",
    "dns will be able to get along with free software",
    "hello free software, no backdoors?",
};

typedef struct {
    u8   cnum;
    char cx[2];
    int  cy;
#define INPUT_BUF_LEN 256
    char data[INPUT_BUF_LEN];
} controller;

void help (void) {
    printf ( "\nusage :\n" );
    printf ( "\tp -- print current cache.\n" );
    printf ( "\td -- hash dump current cache.\n" );
    printf ( "\ta -- access data.\n" );
    printf ( "\tu -- update data.\n" );
    printf ( "\tr -- random access data. (access data).\n" );
    printf ( "\ti -- insert data.\n" );
    printf ( "\tf -- flush dirty data.\n" );
    printf ( "\th -- show help message.\n" );
    printf ( "\tq -- exit.\n" );
    printf ( "\t\n" );
}

int random_access(lru_mgt *mgt) {
    // access.
    srand(mgt->count);

    char *ptr = (char *)mgt + sizeof(lru_mgt);
    for (int i=0; i<mgt->count; ++i) {
        node *n = (node *) (ptr + ((random()%mgt->total) * (sizeof(node) + MAX_DLEN)));
        if (n->hint == 0) continue;
        access_node(n);
        node_dump(n);
    }
    return 0;
}

int input_handle(lru_mgt *mgt, controller *ctl) {
    switch(clower(ctl->cx[0])) {
        case 'p':
            lru_dump(mgt);
            break;
        case 'd':
            lru_hdump(mgt);
            break;
        case 'a':
            if (0 == strlen(ctl->data)) {
                printf ( "read data must not be empty.\n" );
                return -1;
            }
            node_dump(access_data(mgt, ctl->data));
            break;
        case 'r':
            random_access(mgt);
            break;
        case 'f':
            manual_flush_dirty_data(mgt) ;
            break;
        case 'u':
            if (ctl->cnum < 2) {
                p_err("invalid args. idx arg missed\n");
                break;
            }
            p_info("update node data. idx : [%d]\n", ctl->cy);
            edit_node(mgt, lru_idx_query(mgt, ctl->cy), ctl->data);
            break;
        case 'i':
            if (strlen(ctl->data) == 0) {
                printf ( "insert empty data\n" );
                return -1;
            }
            lru_add_data(mgt, ctl->data, strlen(ctl->data));
            break;
        case 'h':
            help();
            break;
        default:
            printf("\nInvalid command !\n");
            help();
    }

    return 0;
}

void flush_output(char *title, int pass) {
    printf("\x1b[H\x1b[2J");    /*  Cursor home, clear screen. */
    printf("%s [%d]\n", title, pass); /*  Print title. */
    fflush(stdout);
}


int main ( int argc, char *argv[] ) {
    // init
    controller ctl = {0};

    lru_mgt *mgt = NULL;
    lru_buff_init(&mgt, 16);

    if (storaged()) {
        unfreeze_data(mgt, 16);
    } else {
        prepare_fake_data(mgt, rand_str, 16);
    }

    // clear screen.
    flush_output("simulator of lru, buffer size : ", 16);

    help();

    freeze_data(mgt, 16);

   // p_info(" <file>:%s, func:%s line:%d\n ", __FILE__, __FUNCTION__, __LINE__);

    while(1) {
        memset(&ctl, '\0', sizeof(controller));
        printf("\n>> ");
        scanf("%1s", ctl.cx);
        ctl.cx[1] = '\0';
        if (ctl.cx[0] == 'i' || ctl.cx[0] == 'I' || 
            ctl.cx[0] == 'a' || ctl.cx[0] == 'A' ){
            printf ( "|\n>>>> " );
            getchar();
            scanf("%254[^\n]", ctl.data);
            ctl.cnum = 1;
        } else if ( ctl.cx[0] == 'u' || ctl.cx[0] == 'U' )  {
            printf ( "| input update idx : \n>>> " );
            scanf("%d", &ctl.cy);
            printf ( "|\n>>>> " );
            getchar();
            scanf("%254[^\n]", ctl.data);
            ctl.cnum = 2;
        } else if (ctl.cx[0] == 'q' || ctl.cx[0] == 'Q') {
            printf ( "\nBye.\n\n" );
            break;
        }
        input_handle(mgt, &ctl);
    }

    // destructor.
    lru_buff_destructor(&mgt);
    return EXIT_SUCCESS;
}


