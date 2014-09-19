CXXFLAGS = -std=c++11 -g

EXT_LIBS = sdl2 libglog
CPPFLAGS = -Isrc/ $(shell pkg-config --cflags $(EXT_LIBS))
LDLIBS = $(shell pkg-config --libs $(EXT_LIBS))

SRCS = src/hello-sdl.cc
OBJS = $(subst src,build,$(subst .cc,.o,$(SRCS)))

hello-sdl: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDLIBS)

build/%.o: src/%.cc
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	$(RM) $(OBJS)
