CXX_STANDARD=1z
CXXFLAGS += -Wall -Wextra -Wpedantic -Wshadow \
            -std=c++$(CXX_STANDARD) -pthread
LDFLAGS += -pthread

CXXFLAGS += \
	-I"$(PWD)/src"                        \
	-isystem"$(PWD)/deps/include"         \
	-I"$(PWD)/vendor/phoenix_ptr/include" \
	-I"$(PWD)/vendor/thread_pool/include" \
	-isystem"$(PWD)/vendor/glm/"

CXXFLAGS += \
	-DGLM_FORCE_CXX14=1

ifdef DEBUG
  CFLAGS += -O0 -g
  CXXFLAGS += -O0 -g
else
  CFLAGS += -Ofast -flto
  CXXFLAGS += -Ofast -flto
  LDFLAGS += -flto
endif

pkgs = glfw3 epoxy

CXXFLAGS += $(shell echo $(pkgs) | tr ' ' '\n' | (echo; xargs -l1 pkg-config --cflags) | tr '\n' ' ' | sed 's@ -I@ -isystem @g')
libs += \
	$(shell echo $(pkgs) | tr ' ' '\n' | xargs -l1 pkg-config --libs) \
	-lwebp

exe = gassist
objects = $(shell find src -iname '*.cc' | sed 's@\.cc$$@.o@')

.PHONY: all
all: $(exe) assets

$(exe): $(objects)
	$(CXX) $(LDFLAGS) $(libs) $(objects) -o $(exe)

gassist.o wrap.o: wrap_glfw.hh
gassist.o: deps/include/oglplus/

.PHONY: clean clean-deps clean-all

clean:
	rm -fv $(objects) $(exe)

clean-deps:
	rm -fvr deps/

clean-all: clean clean-deps clean-assets

#### ASSET PIPELINE ####

assets_sdir = assets_src/
assets_tdir = assets/

__assets_files = $(shell find $(assets_sdir) -type f \
	| grep -vPi '(^|/)\.[^/.]'  \
	| awk -v sd='$(assets_sdir)' -v td='$(assets_tdir)' \
			'{ gsub(sd, td); print }')

assets_imgs = $(shell echo "$(__assets_files)" | tr ' ' '\n' \
	| grep -Pi '\.(dds|png|webp|gif|jpeg|tiff?|tga)$$' \
	| sed 's@\.[^.]*$$@.webp@g')

assets_copy = $(shell echo "$(__assets_files)" | tr ' ' '\n' \
	| grep -Pi '\.(txt)$$')

assets_targets = $(assets_imgs) $(assets_copy)

.PHONY: assets clean-assets

assets: $(assets_targets)

# TODO: Support more suffices
# TODO Make dire generation more pretty
$(assets_tdir)%.webp: $(assets_sdir)%.dds
	mkdir -p "$(shell dirname "$@")"
	convert $< $@

$(assets_tdir)%: $(assets_sdir)%
	mkdir -p "$(shell dirname "$@")"
	cp $< $@

clean-assets:
	rm -rfv "$(assets_tdir)"/*

#### DEPS ####

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
