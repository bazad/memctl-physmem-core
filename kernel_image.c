/*
 * kernel_image.c
 * Brandon Azad
 *
 * Implementation of kernel_symbol and kernel_search based on libmemctl.
 */
#include "kernel_image.h"

#include "fail.h"

#include "memctl/kernel.h"

kaddr_t kernel_symbol(const char *symbol) {
	kaddr_t kaddr;
	kext_result kr = kext_resolve_symbol(&kernel, symbol, &kaddr, NULL);
	if (kr != KEXT_SUCCESS) {
		FAIL("could not resolve kernel symbol %s", symbol);
	}
	return kaddr;
}

kaddr_t kernel_search(const void *data, size_t size) {
	kaddr_t kaddr;
	kext_result kr = kext_search_data(&kernel, data, size, VM_PROT_READ, &kaddr);
	if (kr != KEXT_SUCCESS) {
		FAIL("could not find data in kernel");
	}
	return kaddr;
}
