#ifndef _NEWS_PROCESSES_H
#define _NEWS_PROCESSES_H

#ident "@(#)processes.h	1.2 06/11/93 NEWS SMI"



/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


struct pq_header {
    struct execution_environment *head;
    struct execution_environment *tail;
};

struct pq_element {
    struct execution_environment *next;
    struct execution_environment *prev;
};

#define pq_clear(Q) { \
    Q.head = NULL; \
    Q.tail = NULL; \
}

#define pq_insert_head(Q,EE,L) { \
    assert(EE->L.next == NULL); \
    assert(EE->L.prev == NULL); \
    if (Q.head != NULL) { \
        Q.head->L.prev = EE; \
    } else { \
        Q.tail = EE; \
    } \
    EE->L.next = Q.head; \
    Q.head = EE; \
}
   
#define pq_insert_tail(Q,EE,L) { \
    assert(EE->L.next == NULL); \
    assert(EE->L.prev == NULL); \
    if (Q.tail != NULL) { \
        Q.tail->L.next = EE; \
    } else { \
        Q.head = EE; \
    } \
    EE->L.prev = Q.tail; \
    Q.tail = EE; \
}
   
#define pq_remove(Q,EE,L) { \
    if (Q.head == EE) { \
        Q.head = EE->L.next; \
    } \
    if (Q.tail == EE) { \
        Q.tail = EE->L.prev; \
    } \
    if (EE->L.next != NULL) { \
        EE->L.next->L.prev = EE->L.prev; \
    } \
    if (EE->L.prev != NULL) { \
        EE->L.prev->L.next = EE->L.next; \
    } \
    EE->L.next = NULL; \
    EE->L.prev = NULL; \
}

#define pq_head(Q) \
    Q.head

#define pq_tail(Q) \
    Q.tail

#define pq_next(EE,L) \
    EE->L.next

#define pq_prev(EE,L) \
    EE->L.prev

#endif /* _NEWS_PROCESSES_H */
