# core.mk - the core around ScummVM: the C parts (the coroutine, the C
# library calls both builds answer the same way), ScummVM's data files built
# into the core, and the link. The core's C++ (backend/, wbx-entry.cpp) is
# compiled by ScummVM's own make with ScummVM's flags (scummvm-objs.mk).
# Included by common.mk.

# C library calls answered by the core in both builds (compat/libc-same.c)
SAME_WRAPS := qsort bsearch rand srand
SAME_WRAP_FLAGS := $(foreach w,$(SAME_WRAPS),-Wl,--wrap=$(w))

CORE_CFLAGS := -std=gnu11 -O2 -Wall $(CORE_C_EXTRA)
$(call flags_stamp,$(B)/core,$(CC) | $(CORE_CFLAGS))

$(B)/core/%.o: %.c $(B)/core/flags
	@mkdir -p $(dir $@)
	$(CC) $(CORE_CFLAGS) -c -o $@ $<

CORE_C_OBJS := $(B)/core/backend/coroutine.o $(B)/core/compat/libc-same.o

# ScummVM's data files, one zip (stored, in name order, every date the same),
# built into the core and read from its memory (data.S)
DATA_ZIP := $(ROOT)/build/engine-data.zip
$(SVM)/datafiles.txt: $(SVM)/link.stamp ;
$(DATA_ZIP): $(SVM)/datafiles.txt make-data.py
	python3 make-data.py $(SVM)/datafiles.txt $@

$(B)/core/data.o: data.S $(DATA_ZIP) $(B)/core/flags
	@mkdir -p $(dir $@)
	$(CC) $(CORE_CFLAGS) '-DDATA_ZIP="$(DATA_ZIP)"' -c -o $@ data.S

# ScummVM's objects and libraries, and the core's C++, as scummvm-objs.mk
# listed them (read when the link runs: they exist only after ScummVM's make)
SVM_LINK = $$(sed 's\#^\#$(SVM)/\#' $(SVM)/core-objs.txt) -Wl,--start-group $$(sed 's\#^\#$(SVM)/\#' $(SVM)/objs.txt) -Wl,--end-group $$(cat $(SVM)/libs.txt)

# Mesa (setup-mesa.sh; MESA_BUILD is the flavour's folder of it): its
# archives refer to each other both ways, so they are linked in a group,
# with the osmesa target's own object beside them (the shared library it
# belongs to cannot be linked for a guest). Mesa's own copy of zlib is left
# out: deps.mk's, which ScummVM uses, serves both.
MESA_DIR := $(ROOT)/build/mesa/$(MESA_BUILD)
MESA_TARGET = $(shell find $(MESA_DIR)/src/gallium/targets/osmesa -name target.c.o 2>/dev/null | head -1)
MESA_LIBS = $(shell find $(MESA_DIR) -name '*.a' ! -path '*/subprojects/zlib*' 2>/dev/null | sort)
MESA_LINK = $(MESA_TARGET) -Wl,--start-group $(MESA_LIBS) $(DEPS)/lib/libz.a -Wl,--end-group
MESA_CHECK = @[ -n "$(MESA_TARGET)" ] || { echo "no Mesa at $(MESA_DIR): run waterbox/setup-mesa.sh ($(MESA_SETUP))" >&2; false; }

.PHONY: all clean deps scummvm
clean:
	rm -rf $(B)
