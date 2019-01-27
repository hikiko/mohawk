src = $(wildcard src/*.cc src/math/*.cc src/shaders/*.cc)
csrc = $(wildcard src/*.c)
obj = $(src:.cc=.o) $(csrc:.c=.o)
dep = $(obj:.o=.d)
bin = hair

dbg = -g
opt = -O3 -ffast-math
inc = -Isrc -Isrc/shaders -Isrc/math
CXX = g++
CC = gcc
CXXFLAGS = -pedantic -Wall $(dbg) $(opt) $(inc)
CFLAGS = -pedantic -Wall $(dbg) $(opt) $(inc)
LDFLAGS = -lGL -lGLU -lglut -lGLEW -limago -lassimp -lgmath

$(bin): $(obj)
	$(CXX) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.cc
	@$(CPP) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin) $(dep)
