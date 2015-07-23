#ifndef WEBC_H
#define WEBC_H

#include "event.h"

#define ROOT_PATH "/tmp/webc/www/"
#define THREAD_POOL_MIN 2
#define THREAD_POOL_MAX 10

extern int event_active;
extern struct webc_netinf *netinf;

#endif //WEBC_H