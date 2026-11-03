# scummvm-objs.mk - read by ScummVM's own Makefile after it (make -f Makefile
# -f scummvm-objs.mk chimera-objs CHIMERA_DIR=... CHIMERA_INCS=..., in a
# configured build folder). Builds everything ScummVM would link into its
# program but the null backend, and the core's own C++ (the backend and the
# exports) with exactly ScummVM's flags; writes, for the core's link:
#
#   objs.txt       ScummVM's objects and libraries, in its own link order
#   libs.txt       the libraries configure found (deps.mk's)
#   core-objs.txt  the core's objects
#   link.stamp     touched when any of those objects changed
#   datafiles.txt  ScummVM's data files for the engines built (each .dat,
#                  the fonts, the GUI's themes and translations), which the
#                  core carries in its memory

CHIMERA_OBJS := $(DETECT_OBJS) $(filter-out backends/platform/null/%,$(OBJS))

CORE_SRCS := backend/chimera-machine.cpp backend/chimera-fs.cpp backend/chimera-graphics.cpp \
	backend/chimera-saves.cpp wbx-entry.cpp
CORE_OBJS := $(addprefix chimera/,$(CORE_SRCS:.cpp=.o))

# patches/0001 is switched on for the core's build alone
base/main.o: CXXFLAGS += -DCHIMERA_BACKEND

chimera/%.o: $(CHIMERA_DIR)/%.cpp
	$(QUIET)$(MKDIR) $(@D)/$(DEPDIR)
	$(QUIET_CXX)$(CXX) -MMD -MF $(@D)/$(DEPDIR)/$(@F:.o=.d) -MQ $@ -MP $(CXXFLAGS) $(CPPFLAGS) $(CHIMERA_INCS) -c $(<) -o $@

-include $(wildcard chimera/*/$(DEPDIR)/*.d chimera/$(DEPDIR)/*.d)

# link.stamp is newer than the core's link whenever an object it links is:
# what the link rules (core.mk) depend on
link.stamp: $(CHIMERA_OBJS) $(CORE_OBJS)
	@touch $@

chimera-objs: link.stamp
	@printf '%s\n' $(CHIMERA_OBJS) > objs.txt.new
	@printf '%s\n' $(LIBS) > libs.txt.new
	@printf '%s\n' $(CORE_OBJS) > core-objs.txt.new
	@printf '%s\n' $(DIST_FILES_ENGINEDATA) $(DIST_FILES_ENGINEDATA_BIG) $(DIST_FILES_THEMES) > datafiles.txt.new
	@for f in objs libs core-objs datafiles; do cmp -s $$f.txt.new $$f.txt || mv $$f.txt.new $$f.txt; rm -f $$f.txt.new; done

.PHONY: chimera-objs
