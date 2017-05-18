/*
 * kernel_image.c
 * Brandon Azad
 *
 * Implementation of kernel_symbol and kernel_search based on libmemctl.
 */
#include "kernel_image.h"

#include "fail.h"

#include "memctl/kernel.h"

kaddr_t kernel_sym(const char *symbol) {
	kaddr_t kaddr;
	kext_result kr = kernel_symbol(symbol, &kaddr, NULL);
	if (kr != KEXT_SUCCESS) {
		FAIL("could not resolve kernel symbol %s", symbol);
	}
	return kaddr;
}
