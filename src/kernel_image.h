/*
 * kernel_image.h
 * Brandon Azad
 *
 * Implementation of kernel_symbol and kernel_search based on libmemctl.
 */
#ifndef PHYSMEM_CORE__KERNEL_IMAGE_H_
#define PHYSMEM_CORE__KERNEL_IMAGE_H_

#include <stdint.h>

#include "memctl/memctl_types.h"

/*
 * kernel_symbol
 *
 * Description:
 * 	Resolve a kernel symbol to its address.
 *
 * 	If kernel_slide is 0, then the static (binary) address is returned. Otherwise, the runtime
 * 	(in-memory) address is returned.
 */
kaddr_t kernel_symbol(const char *symbol);

/*
 * kernel_search
 *
 * Description:
 * 	Resolve the address of the given byte sequence in the kernel.
 *
 * 	See kernel_symbol.
 */
kaddr_t kernel_search(const void *data, size_t size);

#endif
