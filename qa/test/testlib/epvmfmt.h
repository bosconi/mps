/* epvmfmt.h
   Format for objects in the EPVM pool.
*/

#ifndef epvmfmt_h
#define epvmfmt_h

#include "mps.h"

extern int alloccomments;

/* the counters are visible so that I can check whether things
   get moved etc
*/

/* the object format is visible so tests that want to
   can hack around with it
*/


typedef struct psobj {
 mps_addr_t *obj;
 mps_word_t size;
} psobj;

extern struct mps_fmt_A_s fmtepvm;

psobj *allocepvm(mps_ap_t ap, int words);

void splatepvm(psobj *obj);

mps_bool_t issplatepvm(psobj *obj);

#endif

