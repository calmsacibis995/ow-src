/* 
 * base.h
 * contains typedefs for iconedit - should have been done a long time ago
 */

typedef struct undo_type { /* struct to keep image and state, COLOR or MONO */
  XImage *image;
  int state;
  int edited;
  int height;
  int width;
} Undo_struct;


typedef struct resource_thingie_type { /* struct to hold those resources */
  int save_height;
  int save_width;
  int save_format;
} resource_thingie_struct;
