# Makefile

MEMCTL_INC_DIR = .

SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = .

ERRFLAGS = -Wall -Wpedantic -Wno-gnu -Werror
CFLAGS   = -O3 -I$(SRC_DIR) -I$(MEMCTL_INC_DIR) $(ERRFLAGS)
ARFLAGS  = r

CORE_LIB = $(LIB_DIR)/libmemctl_core.a

PHYSMEM_SRCS = core.c \
	       kernel_image.c \
	       kernel_slide.c \
	       physmem.c \
	       syscall_hook.c \
	       syscall_hook_dispatch.s

PHYSMEM_HDRS = fail.h \
	       kernel_image.h \
	       kernel_slide.h \
	       physmem.h \
	       syscall_code.h \
	       syscall_hook.h

PHYSMEM_OBJS := $(patsubst %.c,%.o,$(filter %.c,$(PHYSMEM_SRCS))) \
	        $(patsubst %.s,%.o,$(filter %.s,$(PHYSMEM_SRCS)))
PHYSMEM_OBJS := $(PHYSMEM_OBJS:%=$(OBJ_DIR)/%)

vpath % $(SRC_DIR)

all: $(CORE_LIB)

$(OBJ_DIR)/%.o: %.c $(PHYSMEM_HDRS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.s $(PHYSMEM_HDRS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(CORE_LIB): $(PHYSMEM_OBJS)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -rf -- $(OBJ_DIR) $(CORE_LIB)
