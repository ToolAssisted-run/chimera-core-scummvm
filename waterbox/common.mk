# common.mk - what native.mk and guest.mk share: where things are, the
# flags stamp, the patch series, and the rules that build the libraries
# (deps.mk), ScummVM itself (scummvm.mk) and the core around it (core.mk).
# Included, not run. Each build sets FLAVOR, CC, CXX and the flags before
# including this.

ROOT := $(abspath ..)
MB   ?= $(MINIBOX_DIR)
ifeq ($(MB),)
$(error give miniBox's checkout: MB=<chimera>/extern/chimera-common-minibox or MINIBOX_DIR)
endif
MB := $(abspath $(MB))
B  := $(ROOT)/build/$(FLAVOR)

# Every object depends on the flags it was built with: a change to them
# rebuilds it.
define flags_stamp
$(shell mkdir -p $(1); printf '%s\n' '$(2)' | cmp -s - $(1)/flags || printf '%s\n' '$(2)' > $(1)/flags)
endef

# the patch series goes onto the submodule before anything of ScummVM is
# configured or built
PATCH_STAMP := $(ROOT)/build/patches.stamp
$(PATCH_STAMP): $(wildcard $(ROOT)/patches/*.patch) apply-patches.sh
	sh apply-patches.sh
	@mkdir -p $(dir $@)
	@touch $@

include deps.mk
include scummvm.mk
include core.mk

all: deps scummvm
