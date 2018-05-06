/* Definitions and externals for lists */

#ifndef lists_h
#define lists_h

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct List {
    struct List *next, *prev;
    void *item;
} List;

#define Null(L)  ((L == NULL) || (L == L->next))
#define Car(L)   (Null(L) ? NULL : L->next->item)
#define Map(M,L) for (M = L->next; M != L; M = M->next)

/* c.lists */
extern List* new_list(void);
extern List* alloc_list(void);
extern void  list_remove(void *, List *);
extern void* list_remove_first(List *);
extern void  list_add(void *, List *);
extern void  list_add_last(void *, List *);
extern void  list_insert(void *, List *);
extern int   list_member(void *, List *);
extern int   list_length(List *list);
extern void  list_set_add(void *, List *);
extern void  list_unlink(List *);
extern void  list_destroy(List *);
extern void  list_destroy_all(List *);

extern void* list_nth(List *list, int n);

extern void list_insert_after(void *item, void *after, List *list);
extern void copy_list(List *copy, List *list);

#endif
