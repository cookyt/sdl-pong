# External libraries. Configured using `pkg-config`
EXT_LIBS := sdl2 libglog eigen3

CXX = clang++
CXXFLAGS = -std=c++11 -Wall -pedantic -g
CPPFLAGS := $(shell pkg-config --cflags $(EXT_LIBS)) \
            -Isrc/ \
            -DEIGEN_DONT_ALIGN  # trade performance for simpler code
LDLIBS := $(shell pkg-config --libs $(EXT_LIBS)) \
          -lm

# Where object and dependency files are placed (out-of-tree build)
BUILD_DIR = build

SRCS = src/hello-sdl.cc src/pieces.cc
OBJS := $(SRCS:src/%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:%.o=%.d)  # Auto-generated dependencies

# Binaries
# --------
hello-sdl: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDLIBS)

# Compilation Rules
# -----------------
# If they've been created, include auto-generated dependency graphs. These are
# generated on first call to make, and are updated on subsequent calls. Their
# purpose is to recompile source files when dependent header files change.
-include $(DEPS)

$(BUILD_DIR)/%.cc.o: src/%.cc
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -o $@ -c $<

.PHONY: cflags
cflags:
	@echo $(CPPFLAGS) $(CXXFLAGS)

# Clean* rules
# ------------
.PHONY: clean
clean:
	$(RM) $(OBJS)

# Remove auto-generated dependency files
.PHONY: clean-deps
clean-deps:
	-$(RM) $(DEPS)

.PHONY: clean-all
clean-all: clean clean-deps
