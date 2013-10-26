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
    "kannot in good conscience sign a nondisclosure agreement or a software license agreement.",
    "zo that I can continue to use computers without violating my principles,",
    "lave decided to put together a sufficient body of free software",
    "vhat I will be able to get along without any software that is not free.",
    "why I Must Write GNU",
    "consider that the golden rule requires that",
    "if I like a program I must share it with other people who like it.",
    "cannot in good conscience sign a nondisclosure agreement or a software license agreement.",
    "so that I can continue to use computers without violating my principles,",
    "have decided to put together a sufficient body of free software",
    "that I will be able to get along without any software that is not free.",
    "cf I like a program I must share it with other people who like it.",
    "how You Can Contribute"
};

void help (void) {
    printf ( "\nusage :\n" );
    printf ( "\tp -- print current cache.\n" );
    printf ( "\td -- hash dump current cache.\n" );
    printf ( "\ta -- access data.\n" );
    printf ( "\tr -- random access data. (access data).\n" );
    printf ( "\ti -- insert data.\n" );
    printf ( "\th -- show help message.\n" );
    printf ( "\tq -- exit.\n" );
    printf ( "\t\n" );
}

int random_access(lru_mgt *mgt) {
    // access.
    srand(mgt->total);

    char *ptr = (char *)mgt + sizeof(lru_mgt);
    for (int i=0; i<mgt->count; ++i) {
        node *n = (node *) (ptr + ((random()%mgt->total) * (sizeof(node) + MAX_DLEN)));
        if (n->hint == 0) continue;
        access_node(n);
        node_dump(n);
    }
    return 0;
}

int input_handle(lru_mgt *mgt, char c, char *data) {
    switch(c) {
        case 'p':
            lru_dump(mgt);
            break;
        case 'd':
            lru_hdump(mgt);
            break;
        case 'a':
            if (0 == strlen(data)) {
                printf ( "read data must not be empty.\n" );
                return -1;
            }
            node_dump(access_data(mgt, data));
            break;
        case 'r':
            random_access(mgt);
            break;
        case 'i':
            if (strlen(data) == 0) {
                printf ( "insert empty data\n" );
                return -1;
            }
            lru_add_data(mgt, data, strlen(data));
            break;
        case 'h':
            help();
            break;
        default:
            printf("Invalid command !\n");
            help();
    }

    return 0;
}



int main ( int argc, char *argv[] ) {
    // init
    lru_mgt *mgt = NULL;
    lru_buff_init(&mgt, 16);

    prepare_data(mgt, rand_str, 14);

    help();
    char x[2] = {0};
    char data[256] = {0};
    while(1) {
        memset(data, '\0', 256);
        printf("\n>> ");
        scanf("%1s", x);
        x[1] = '\0';
        if (x[0] == 'i' || x[0] == 'a' || x[0] == 'A' || x[0] == 'I') {
            printf ( "|\n>>>> " );
            getchar();
            scanf("%254[^\n]", data);
        } else if (x[0] == 'q' || x[0] == 'Q') {
            printf ( "exit success.\n" );
            break;
        }
        input_handle(mgt, x[0], data);
    }

    // destructor.
    lru_buff_destructor(&mgt);
    return EXIT_SUCCESS;
}

