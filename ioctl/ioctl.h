#ifndef __IOCTL_H
#define __IOCTL_H

#include "linux/ioctl.h"

typedef struct{
	short size;//total size of file
	short avail;//free
	short len;//filled
}info_t;


#define FIFO_CLEAR _IO('x',1)
#define FIFO_INFO _IOR('x',2,info_t)
#define FIFO_RESIZE _IOW('x',3,long)

#endif
