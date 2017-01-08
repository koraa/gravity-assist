CXX_STANDARD=1z
CXXFLAGS += -Wall -Wextra -Wpedantic -Wshadow \
            -std=c++$(CXX_STANDARD) -pthread \
						-Ofast -flto
LDFLAGS += -flto -pthread

CXXFLAGS += \
	-I"$(PWD)/src"                        \
	-isystem"$(PWD)/deps/include"         \
	-I"$(PWD)/vendor/phoenix_ptr/include" \
	-I"$(PWD)/vendor/thread_pool/include" \
	-isystem"$(PWD)/vendor/glm/"

CXXFLAGS += \
	-DOGLPLUS_LOW_PROFILE=1

pkgs = glfw3 epoxy

CXXFLAGS += $(shell echo $(pkgs) | tr ' ' '\n' | (echo; xargs -l1 pkg-config --cflags) | tr '\n' ' ' | sed 's@ -I@ -isystem @g')
libs += $(shell echo $(pkgs) | tr ' ' '\n' | xargs -l1 pkg-config --libs)

exe = gassist
objects = $(shell find src -iname '*.cc' | sed 's@\.cc$$@.o@')

$(exe): $(objects)
	$(CXX) $(LDFLAGS) $(libs) $(objects) -o $(exe)

gassist.o wrap.o: wrap_glfw.hh
gassist.o: deps/include/oglplus/

.PHONY: clean clean-deps

clean:
	rm -fv $(objects) $(exe)

clean-deps:
	rm -fvr deps/

deps/include/oglplus/:
	mkdir -p deps/build/oglplus
	cd deps/build/oglplus; cmake "$(PWD)/vendor/oglplus" \
		-DOGLPLUS_NO_EXAMPLES=On \
		-DOGLPLUS_NO_SCREENSHOTS=On \
		-DOGLPLUS_NO_DOCS=On \
		-DOGLPLUS_WITH_TESTS=Off \
		-DOGLPLUS_LOW_PROFILE=On \
		-DCMAKE_CXX_STANDARD="$(CXX_STANDARD)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DCMAKE_CXX_FLAGS="$(CXXFLAGS) $(CPPFLAGS) $(SUBPROJECT_FLAGS)" \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_C_FLAGS="$(CFLAGS) $(CPPFLAGS) $(SUBPROJECT_FLAGS)" \
		-DCMAKE_INSTALL_PREFIX="$(PWD)/deps"
	cd deps/build/oglplus; $(MAKE) install
