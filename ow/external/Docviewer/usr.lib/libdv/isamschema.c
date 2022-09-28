#include "isam.h"

#include "isamschema.h"

/*------------------ ISAMREC -------------------------*/

struct isfldmap ISAMREC_f [] = {
    { "id"          ,  0, (int) ((struct ISAMREC *)0)->id,  0, 47 },
    { "title"       , 47, (int) ((struct ISAMREC *)0)->title,  0,127 },
    { "label"       ,174, (int) ((struct ISAMREC *)0)->label,  0, 47 },
    { "view_method" ,221, (int) ((struct ISAMREC *)0)->view_method,  0, 63 },
    { "print_method",284, (int) ((struct ISAMREC *)0)->print_method,  0, 63 },
    { "flags"       ,347, (int) &((struct ISAMREC *)0)->flags,  2,  4 },
    { "first_page"  ,351, (int) &((struct ISAMREC *)0)->first_page,  1,  2 },
    { "last_page"   ,353, (int) &((struct ISAMREC *)0)->last_page,  1,  2 },
    { "parent_rec"  ,355, (int) &((struct ISAMREC *)0)->parent_rec,  2,  4 },
    { "next_sibling_rec",359, (int) &((struct ISAMREC *)0)->next_sibling_rec,  2,  4 },
    { "prev_sibling_rec",363, (int) &((struct ISAMREC *)0)->prev_sibling_rec,  2,  4 },
    { "first_child_rec",367, (int) &((struct ISAMREC *)0)->first_child_rec,  2,  4 },
    { "last_child_rec",371, (int) &((struct ISAMREC *)0)->last_child_rec,  2,  4 },
    { "spare_long_1",375, (int) &((struct ISAMREC *)0)->spare_long_1,  2,  4 },
    { "spare_long_2",379, (int) &((struct ISAMREC *)0)->spare_long_2,  2,  4 },
    { "spare_char63_1",383, (int) ((struct ISAMREC *)0)->spare_char63_1,  0, 63 },
    { "spare_char63_2",446, (int) ((struct ISAMREC *)0)->spare_char63_2,  0, 63 }
};

struct keydesc ISAMREC_k [] = {
    {  0, 1, {{0,47,0}}}
};

char *ISAMREC_kn [] = {"ISAMREC_MASTER"};

struct istableinfo ISAMREC_t = { "ISAMREC", 509, 17, ISAMREC_f, 1, ISAMREC_k, ISAMREC_kn };


