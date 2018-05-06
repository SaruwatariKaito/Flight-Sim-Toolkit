/* Definitions and externals for lists */

#ifndef lists_h
#define lists_h

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct sList {
    struct sList *next, *prev;
    void *item;
} sList;

#define Null(L)  ((L == NULL) || (L == L->next))
#define Car(L)   (Null(L) ? NULL : L->next->item)
#define Map(M,L) for (M = L->next; M != L; M = M->next)

/* c.main */
extern void stop(char *, ...);

/* c.lists */
extern int   list_count;
extern void  init_lists(void);
extern sList* new_list(void);
extern sList* alloc_list(void);
extern void* list_first(sList *list);
extern void  list_remove(void *, sList *);
extern void* list_remove_first(sList *);
extern void  list_add(void *, sList *);
extern void  list_add_last(void *, sList *);
extern void  list_insert(void *, sList *);
extern void* list_next(void *item, sList *list);
extern void* list_prev(void *item, sList *list);
extern int   list_member(void *, sList *);
extern int   list_length(sList *list);
extern void  list_set_add(void *, sList *);
extern void  list_append(sList *l1, sList *l2);
extern void  list_unlink(sList *);
extern sList *copy_list(sList *list);
extern void  list_destroy(sList *);
extern void  list_destroy_all(sList *);

#endif
