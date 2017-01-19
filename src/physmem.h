/*
 * physmem.h
 * Brandon Azad
 *
 * An exploit for CVE-2016-1825 and CVE-2016-7617 that allows reading and writing arbitrary
 * physical addresses on macOS.
 */
#ifndef PHYSMEM_CORE__PHYSMEM_H_
#define PHYSMEM_CORE__PHYSMEM_H_

#include <stdint.h>

#include "memctl/memctl_types.h"

/*
 * physmem_init
 *
 * Description:
 * 	Establish a connection to the user client we will use for phys_read and phys_write.
 */
void physmem_init(void);

/*
 * physmem_deinit
 *
 * Description:
 * 	Close the connection to the user client.
 */
void physmem_deinit(void);

/*
 * kern_read
 *
 * Description:
 * 	Read the width-byte integer at the given kernel address. If the address is not within the
 * 	kernel image, the result may not be accurate.
 */
kword_t kern_read(kaddr_t kaddr, unsigned width);

/*
 * kern_write
 *
 * Description:
 * 	Write the value as a width-byte integer to the given kernel address. If the address is not
 * 	within the kernel image, the data may be written to an arbitrary location.
 */
void kern_write(kaddr_t kaddr, kword_t value, unsigned width);

#endif
