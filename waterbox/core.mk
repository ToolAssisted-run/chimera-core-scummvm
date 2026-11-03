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

.PHONY: all clean deps scummvm
clean:
	rm -rf $(B)
