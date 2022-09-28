/*------------------ ISAMREC -------------------------*/
#define ISAMREC_RECLEN 509

struct ISAMREC {
    char      id[48];
    char      title[128];
    char      label[48];
    char      view_method[64];
    char      print_method[64];
    long      flags;
    short     first_page;
    short     last_page;
    long      parent_rec;
    long      next_sibling_rec;
    long      prev_sibling_rec;
    long      first_child_rec;
    long      last_child_rec;
    long      spare_long_1;
    long      spare_long_2;
    char      spare_char63_1[64];
    char      spare_char63_2[64];
};

extern struct isfldmap ISAMREC_f [];
extern struct keydesc ISAMREC_k [];
#define ISAMREC_MASTER (&ISAMREC_k [0])
extern char *ISAMREC_kn [];
extern struct istableinfo ISAMREC_t;


