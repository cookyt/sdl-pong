# External Programs
# -----------------
CXX = clang++
PROTOC ?= protoc
MKDIR_P ?= mkdir -p

# Special Directories
# -------------------
# Root of the source tree
SRC_DIR = src

# Auto-generated source files (auto-generated dir)
GEN_DIR = gen

# Compiled executables (auto-generated dir)
BIN_DIR = bin

# Compiler intermediate output (auto-generated dir)
BUILD_DIR = build


# Build Configuration
# -------------------
# External libraries configured using `pkg-config`
PKG_CONFIG_LIBS := libglog \
                   gflags \
                   eigen3 \
                   sdl2 \
                   SDL2_ttf

CXXFLAGS += -std=c++11 -Wall -Wno-unused-private-field -pedantic -g
CPPFLAGS := $(shell pkg-config --cflags $(PKG_CONFIG_LIBS)) \
            -I$(GEN_DIR) -I$(SRC_DIR) \
            -DEIGEN_DONT_ALIGN  # trade performance for simpler code
LIBS := $(shell pkg-config --libs $(PKG_CONFIG_LIBS)) \
        -lm

CC_SRCS = $(SRC_DIR)/controller.cc \
          $(SRC_DIR)/rendering.cc \
          $(SRC_DIR)/game.cc
PROTO_SRCS =

CC_BINS := $(BIN_DIR)/hello-sdl

CC_GEN_PROTO = $(PROTO_SRCS:$(SRC_DIR)/%.proto=$(GEN_DIR)/%.pb.cc)
CC_OBJS := $(CC_SRCS:$(SRC_DIR)/%.cc=$(BUILD_DIR)/%.cc.o) \
           $(PROTO_SRCS:$(SRC_DIR)/%.proto=$(BUILD_DIR)/%.pb.cc.o)
CC_DEPS := $(CC_OBJS:%.o=%.d) \
           $(CC_BINS:$(BIN_DIR)/%=$(BUILD_DIR)/%.cc.d)

# Binaries
# --------
.PHONY: all
all: $(CC_BINS)

$(BIN_DIR)/%: $(BUILD_DIR)/%.cc.o $(CC_OBJS)
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)


# Compilation Rules
# -----------------
# If they've been created, include auto-generated dependency graphs. These are
# generated on first call to make, and are updated on subsequent calls. Their
# purpose is to recompile source files when dependent header files change.
-include $(CC_DEPS)

$(BUILD_DIR)/%.cc.o: $(SRC_DIR)/%.cc
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -o $@ -c $<

# TODO: This is exactly the same as the above rule, but uses GEN_DIR instead of
# SRC_DIR. I'd like to keep generated code seperate from hand-written code, but
# it's nasty having two identical rules.
$(BUILD_DIR)/%.cc.o: $(GEN_DIR)/%.cc
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -o $@ -c $<

$(GEN_DIR)/%.pb.cc: $(SRC_DIR)/%.proto
	$(MKDIR_P) $(dir $@)
	$(PROTOC) --cpp_out=$(GEN_DIR) --proto_path=$(SRC_DIR) $^


# Convenience Rules
# -----------------
# Easy way to regen protobuf sources
.PHONY: protos
protos: $(CC_GEN_PROTO)

# Simple way to pass flags to you complete me
.PHONY: cflags
cflags:
	@echo $(CPPFLAGS) $(CXXFLAGS)


# Clean* rules
# ------------
.PHONY: clean
clean:
	$(RM) $(CC_OBJS)
	$(RM) $(CC_BINS:$(BIN_DIR)/%=$(BUILD_DIR)/%.cc.o)

.PHONY: clean-bin
clean-bin:
	$(RM) $(CC_BINS)

# Remove auto-generated dependency files
.PHONY: clean-deps
clean-deps:
	-$(RM) $(CC_DEPS)

# Remove protobuf c++ classes
.PHONY: clean-gen
clean-gen:
	-$(RM) $(CC_GEN_PROTO)
	-$(RM) $(CC_GEN_PROTO:.cc=.h)  # generated header files

.PHONY: clean-all
clean-all: clean clean-bin clean-deps clean-gen

.SECONDARY:
