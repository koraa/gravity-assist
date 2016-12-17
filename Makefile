CXXFLAGS += -Wall -Wextra -Wpedantic -Wshadow \
            -O3 -std=c++1z -flto
LDFLAGS += -flto

CXXFLAGS += \
	-I"$(PWD)/vendor/phoenix_ptr/include"

pkgs = glfw3 epoxy

CXXFLAGS += $(shell echo $(pkgs) | tr ' ' '\n' | (echo; xargs -l1 pkg-config --cflags) | tr '\n' ' ' | sed 's@ -I@ -isystem @g')
libs += $(shell echo $(pkgs) | tr ' ' '\n' | xargs -l1 pkg-config --libs)

exe = gassist
objects = $(shell ls *.cc | sed 's@\.cc$$@.o@')

$(exe): $(objects)
	$(CXX) $(LDFLAGS) $(libs) $(objects) -o $(exe)

gassist.o wrap.o: wrap.hh

.PHONY: clean

clean:
	rm -fv $(objects) $(exe)
