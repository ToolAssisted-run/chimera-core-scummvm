# native.mk - the native reference: the libraries (deps.mk), ScummVM
# (scummvm.mk) and the core (core.mk) built for this computer, with the same
# sources and defines as guest.mk, plus the two harnesses (run-native drives
# the exports directly; run-wbx drives core.wbx through the miniBox host as
# Chimera does). Everything lands in build/native.
#
# Usage: make -f native.mk -j4 MB=<miniBox checkout>

.DEFAULT_GOAL := all
FLAVOR := native
CC  := gcc
CXX := g++
DEPS_CFLAGS :=
SVM_CXXFLAGS :=
SVM_LDFLAGS :=
EMULIBC_INC := -I$(abspath native-shim)
CORE_C_EXTRA :=
include common.mk

# the sandbox's math functions, natively (make-native-libm.sh)
LIBM := $(ROOT)/build/native-libm
$(LIBM)/libchimera-m.a: make-native-libm.sh
	bash make-native-libm.sh $(MB)

# natively the C library's clocks say what the sandbox's do (run-native.c)
NATIVE_WRAPS := clock_gettime gettimeofday time
NATIVE_WRAP_FLAGS := $(foreach w,$(NATIVE_WRAPS),-Wl,--wrap=$(w))

$(B)/core/run-native.o: run-native.c gate-harness.h input-names.h $(B)/core/flags
	@mkdir -p $(dir $@)
	gcc -O2 -Wall -c -o $@ run-native.c

$(B)/run-native: $(SVM)/link.stamp $(CORE_C_OBJS) $(B)/core/data.o $(B)/core/run-native.o $(LIBM)/libchimera-m.a
	g++ -no-pie -o $@ $(B)/core/run-native.o $(CORE_C_OBJS) $(B)/core/data.o $(SVM_LINK) \
		$(LIBM)/libchimera-m.a $(SAME_WRAP_FLAGS) $(NATIVE_WRAP_FLAGS) $$(cat $(LIBM)/wrap-flags) -lm

# run-wbx links the miniBox host library
MBHOST := $(MB)/build/meson-linux/source/host
$(B)/run-wbx: run-wbx.c gate-harness.h input-names.h
	@mkdir -p $(dir $@)
	gcc -O2 -Wall -I. -I$(MB)/source/host -o $@ run-wbx.c $(MBHOST)/libminiboxhost.so -Wl,-rpath,$(MBHOST)

all: $(B)/run-native $(B)/run-wbx
