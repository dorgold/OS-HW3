#ifndef R_MW_W_LOCK_H_
#define R_MW_W_LOCK_H_

#include <stdlib.h>

//typedef void* Lock;

/////tmp-impl/////
typedef struct lock* Lock;
////end-of-impl////

Lock init_lock();
void get_read_lock(Lock l);
void get_may_write_lock(Lock l);
void get_write_lock(Lock l);
void upgrade_may_write_lock(Lock l);
void release_shared_lock(Lock l);
void release_exclusive_lock(Lock l);
void destroy_lock(Lock l);

#endif /* R_MW_W_LOCK_H_ */
