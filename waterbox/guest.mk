# guest.mk - core.wbx: the same libraries, ScummVM and core as native.mk,
# built with miniBox's musl/libstdc++ guest toolchain and linked at the guest
# base. Everything lands in build/guest; core.wbx is checked by miniBox's
# check-wbx.sh (no thread-local storage, no %fs, no red zone) before it
# counts as built.
#
# Needs miniBox built WITH the C++ guest toolchain:
#   meson setup <miniBox>/build/meson-cpp <miniBox> -Dguest_cpp=true && ninja -C <miniBox>/build/meson-cpp
#
# Usage: make -f guest.mk -j4 MB=<miniBox checkout>

.DEFAULT_GOAL := all
# TEST_ENGINES=no leaves out ScummVM's two test engines (testbed and
# Playground 3D): the published package's build, in its own folder
# (build/guest-release). The tests build and use the one with them.
TEST_ENGINES ?= yes
FLAVOR := guest$(if $(filter no,$(TEST_ENGINES)),-release,)
MB     ?= $(MINIBOX_DIR)
MBUILD := $(abspath $(MB))/build/meson-cpp
SR     := $(MBUILD)/guest-sysroot
GCCVER := $(shell gcc -dumpfullversion)
# the system's compilers over the guest sysroot, through its specs (musl's
# headers and start files, -mno-red-zone), as the DOSBox-X core's cross file
CC     := gcc -specs $(SR)/lib/musl-gcc.specs
CXX    := g++ -specs $(SR)/lib/musl-gcc.specs

# BizHawk waterbox's frozen guest flags, as miniBox's source/guest/meson.build
# gives them to a guest; no thread-local storage (the sandbox has none)
WBFLAGS := -fvisibility=hidden -mcmodel=large -mno-red-zone -mstack-protector-guard=global \
	-fno-stack-protector -fno-pic -fno-pie -fcf-protection=none -DCHIMERA_GUEST \
	-Dthread_local= -D_Thread_local= -D__thread=
CXXINCS := -I$(SR)/include/c++/$(GCCVER) -I$(SR)/include/c++/$(GCCVER)/x86_64-linux-musl

DEPS_CFLAGS := $(WBFLAGS)
SVM_CXXFLAGS := $(WBFLAGS) $(CXXINCS)
SVM_LDFLAGS := -static -no-pie
SVM_HOST := --host=x86_64-linux-musl
EMULIBC_INC := -I$(abspath $(MB))/extern/emulibc
CORE_C_EXTRA := $(WBFLAGS)
MESA_BUILD := build-guest2
MESA_SETUP := -m <miniBox>
include common.mk

$(SR)/lib/libstdc++.a:
	@echo "miniBox's C++ guest toolchain is missing: $(SR)" >&2
	@echo "build it: meson setup $(MBUILD) $(MB) -Dguest_cpp=true && ninja -C $(MBUILD)" >&2
	@false

# the guest kit's link recipe (the DOSBox-X core's): the large code model's
# --no-relax, the weak pthread pulls libgcc_eh needs, cxxglue for the
# unwinder; Mesa declares some pthread entry points weak too, which linked
# statically would be zero, so each is pulled in (the RSDK v4 core's list).
# Mesa's thread-local variables give the image a TLS segment, which miniBox
# gives the guest's own %fs (check-wbx.sh says so).
$(B)/core.wbx: $(SVM)/link.stamp $(CORE_C_OBJS) $(B)/core/data.o | $(SR)/lib/libstdc++.a
	$(MESA_CHECK)
	$(CXX) -static -no-pie -Wl,--eh-frame-hdr -Wl,-O2 -Wl,--no-relax -Wl,--strip-debug -Wl,-z,stack-size=8388608 \
		-T $(MB)/source/guest/linkscript.T \
		-Wl,-u,pthread_mutexattr_init -Wl,-u,pthread_mutexattr_settype -Wl,-u,pthread_mutexattr_destroy \
		-Wl,-u,pthread_once -Wl,-u,pthread_cond_wait -Wl,-u,pthread_cond_broadcast -Wl,-u,pthread_key_create \
		-o $@.tmp $(CORE_C_OBJS) $(B)/core/data.o $(SVM_LINK) $(MESA_LINK) \
		$(MBUILD)/source/guest/cxxglue.c.o $(MBUILD)/source/guest/emulibc.c.o $(SAME_WRAP_FLAGS) \
		-L$(SR)/lib -lstdc++ -lm -lgcc -lgcc_eh -lc
	sh $(MB)/source/guest/check-wbx.sh $@.tmp
	mv $@.tmp $@

all: $(B)/core.wbx
