/*
 * kernel_slide.h
 * Brandon Azad
 *
 * Find the kernel slide using the physmem exploit.
 */
#ifndef PHYSMEM_CORE__KERNEL_SLIDE_H_
#define PHYSMEM_CORE__KERNEL_SLIDE_H_

#include <stdint.h>

#include "memctl/kernel_slide.h"

/*
 * probe_kernel_slide
 *
 * Description:
 * 	Find the kernel slide.
 *
 * Dependencies:
 * 	libmemctl: kernel_init
 * 	physmem_init
 */
void probe_kernel_slide(void);

#endif
