# Makefile

MEMCTL_INC_DIR = ../include

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
	       physmem.c

PHYSMEM_HDRS = fail.h \
	       kernel_image.h \
	       kernel_slide.h \
	       physmem.h

PHYSMEM_SRCS := $(PHYSMEM_SRCS:%=$(SRC_DIR)/%)
PHYSMEM_HDRS := $(PHYSMEM_HDRS:%=$(SRC_DIR)/%)
PHYSMEM_OBJS := $(PHYSMEM_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(CORE_LIB)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(PHYSMEM_HDRS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(CORE_LIB): $(PHYSMEM_OBJS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -rf -- $(OBJ_DIR) $(CORE_LIB)
