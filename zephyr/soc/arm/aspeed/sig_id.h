#ifndef __SIG_ID_H__
#define __SIG_ID_H__

/* Pin ID */
enum {
	SIG_NONE = -1,
#define GPIO_SIG_DEFINE(sig, ...)
#define SIG_DEFINE(sig, ...) sig,
    #include "sig_def_list.h"
#undef SIG_DEFINE
#undef GPIO_SIG_DEFINE
	SIG_GPIO,
	MAX_SIG_ID,
};

#endif /* #ifndef __SIG_ID_H__ */
