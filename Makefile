# Makefile with automatic targets and dependencies*.
#
# Try make info.
#
# Targets:
#   lib:  built from files in $(SRC_DIR) that have both a .$(SRC_EXT) and .$(HDR_EXT)
#   bin:  built from files in $(SRC_DIR) that only have a .$(SRC_EXT)
#   test:  built from *.$(SRC_EXT) in $(TEST_DIR) and executed in $(TEST_EXEC_DIR)
#          see $(TEST_DIR)/Test.h for a very simple unit-test framework
#   scratch:  built from *.$(SRC_EXT) in $(SCRATCH_DIR)
#   include:  copy-only for *.(HDR_EXT) in $(SRC_DIR)
#
# Subdirectories are supported, only caveat is don't have a duplicate name for
# things in the bin target, or it will cause overwrites in $(BIN_DIR), you can still
# get everything manually from $(BUILD_DIR) though.
#
# You can add dependencies in ./deps to get automatic -I and -L flags.
#
# *: the only indirect(i.e. not generated straight from an #include) rule is that
# binaries(bin, test, scratch) that include F.$(HDR_EXT) are also assumed to depend
# on F.$(SRC_EXT) if it exists.


# Config...
# NAME - used for lib$(NAME).a/.so
NAME = hehelp
LDFLAGS = -L/usr/local/lib/
LDLIBS = ./deps/HElib/src/fhe.a -lntl -lgmp -lm -lboost_system -lpthread
CXX = clang++
CXXFLAGS = -g -O2 -Wshadow -Wall -Wfatal-errors -std=c++11 -fPIC -I/usr/local/include
# File extensions / ignored files.
SRC_EXT = .cc
HDR_EXT = .h
IGNORE =

all: test-auto lib bin scratch include

# Paths. Using ./ for relative paths not optional since text must be compatible
# with find output.
SRC_DIR = ./src
TEST_DIR = ./test
SCRATCH_DIR = ./scratch
BUILD_DIR = ./build
DEP_DIR = ./deps
LIB_DIR = ./lib
BIN_DIR = ./bin
HDR_DIR = ./include
LIB_SHARED = $(LIB_DIR)/lib$(NAME).so
LIB_STATIC = $(LIB_DIR)/lib$(NAME).a
# Some recipes expect these to be nested in $(BUILD_DIR), so don't change them.
TEST_EXEC_DIR = $(BUILD_DIR)/.tests
SCRATCH_BUILD_DIR = $(BUILD_DIR)/.scratch

# Flags.
INCDIR = $(addprefix -I, $(shell find $(DEP_DIR) -type d))
INCDIR += $(addprefix -I, $(shell find $(SRC_DIR) -type d))
INCDIR += -I$(TEST_DIR)
CXXFLAGS += $(INCDIR)
LDFLAGS += $(addprefix -L, $(shell find $(DEP_DIR) -type d))

# Sources / prerequisites.
SRC := $(shell find $(SRC_DIR) -type f -name "*$(SRC_EXT)")
HDR := $(shell find $(SRC_DIR) -type f -name "*$(HDR_EXT)")
SRC := $(filter-out $(IGNORE), $(SRC))
HDR := $(filter-out $(IGNORE), $(HDR))
BIN_SRC := $(filter-out $(HDR:$(HDR_EXT)=$(SRC_EXT)), $(SRC))
SRC := $(filter-out $(BIN_SRC), $(SRC))
TEST_SRC := $(shell find $(TEST_DIR) -type f -name "*$(SRC_EXT)")
SCRATCH_SRC := $(shell find $(SCRATCH_DIR) -type f -name "*$(SRC_EXT)")

# Binaries / targets. _COPY for targets that just end up copying files.
BIN := $(subst $(SRC_DIR), $(BUILD_DIR), $(BIN_SRC:$(SRC_EXT)=))
OBJ := $(subst $(SRC_DIR),$(BUILD_DIR),$(SRC:$(SRC_EXT)=.o))
TEST := $(subst $(TEST_DIR), $(TEST_EXEC_DIR), $(TEST_SRC:$(SRC_EXT)=))
SCRATCH := $(subst $(SCRATCH_DIR), $(SCRATCH_BUILD_DIR), $(SCRATCH_SRC:$(SRC_EXT)=))
BIN_COPY := $(foreach f, $(BIN), $(BIN_DIR)/$(notdir $(f)))
SCRATCH_COPY = $(subst $(SCRATCH_BUILD_DIR), $(SCRATCH_DIR), $(SCRATCH))
HDR_COPY = $(subst $(SRC_DIR), $(HDR_DIR), $(HDR))

# Build dir skeleton.
BUILD_SKEL := $(shell find $(SRC_DIR) -type d ! -name .)
BUILD_SKEL := $(subst $(SRC_DIR), $(BUILD_DIR), $(BUILD_SKEL))
BUILD_SKEL += $(TEST_EXEC_DIR)
BUILD_SKEL += $(SCRATCH_BUILD_DIR)
BUILD_SKEL += $(subst $(TEST_DIR), $(TEST_EXEC_DIR), \
				$(shell find $(TEST_DIR) -type d ! -name .))
BUILD_SKEL += $(subst $(SCRATCH_DIR), $(SCRATCH_BUILD_DIR), \
				$(shell find $(SCRATCH_DIR) -type d ! -name .))


# Auto deps.
DEP = $(OBJ:=.d) $(TEST:=.d) $(BIN:=.d) $(SCRATCH:=.d)
-include $(DEP)

# Commands.
remove_root = $(shell echo $1 | sed s/\[.]\*\[/]\*\[^/]\*\\\///)
get_src =  $2/$(call remove_root, $(basename $1)$(SRC_EXT))
get_bin = $(filter %$(notdir $1), $(BIN))
get_bin_dep = $(filter $(subst $(HDR_EXT),$(SRC_EXT), $(filter $(patsubst ./%,%,$(HDR)), $1)), $(patsubst ./%,%,$(SRC)))

make_dep = @$(CXX) -MM $(CXXFLAGS) $1 > $2.d; \
			sed -i 's/.*:/$(subst /,\/,$@):/' $@.d

compile = @echo "Compiling:   $(strip $1) -> $(strip $2): " $(shell $(call compile_cmd, $1, $2, $3))
compile_cmd = CMD="$(CXX) $1 $3 -o $2"; \
$$CMD; \
RET=$$?; \
if [ $$RET -eq 0 ]; then \
	echo " Ok"; \
	exit 0; \
else \
	echo " Error\($$CMD\)"; \
	exit $$RET; \
fi \

run_test = printf "Executing:    $1:  "; \
$1; \
OUT=$$?; \
if [ $$OUT -eq 0 ]; then \
	echo "Ok"; \
else \
	rm $1; \
fi \

# Help message.
HELP = \
\rBuild summary:\n \
 $(SRC_DIR)/: $(foreach f, $(SRC), $(call remove_root, $(f)))  ->  $(LIB_STATIC) $(LIB_SHARED)\n \
 \t $(foreach f, $(BIN_SRC), $(call remove_root, $(f)))  ->  $(BIN_COPY)\n \
 \t $(foreach f, $(HDR), $(call remove_root, $(f)))  ->  $(HDR_DIR)\n \
 $(SCRATCH_DIR)/: $(foreach f, $(SCRATCH_SRC), $(call remove_root, $(f)))  ->  $(SCRATCH_COPY)\n \
 $(TEST_DIR)/: $(foreach f, $(TEST_SRC), $(call remove_root, $(f)))  ->  $(TEST) (executed automatically)\n

# Required for dynamic prerequisites.
.SECONDEXPANSION:

# Actual recipes.
$(BIN): %: $$(call get_src, $$@, $(SRC_DIR))
$(BIN): %: $$(call get_bin_dep, $$^) | $(OBJ) $(BUILD_DIR)
	$(eval FROM_SRC=$(call get_src, $@, $(SRC_DIR)))
	$(call make_dep, $(FROM_SRC), $@)
	$(call compile, $(FROM_SRC), $@, $(OBJ) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS))

$(SCRATCH): %: $$(call get_src, $$(call remove_root, $$@), $(SCRATCH_DIR))
$(SCRATCH): %: $$(call get_bin_dep, $$^) | $(OBJ) $(BUILD_DIR)
	$(eval FROM_SRC=$$(call get_src, $$(call remove_root, $$@), $(SCRATCH_DIR)))
	$(call make_dep, $(FROM_SRC), $@)
	$(call compile, $(FROM_SRC), $@, $(OBJ) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS))

$(TEST): %: $$(call get_src, $$(call remove_root, $$@), $(TEST_DIR))
$(TEST): %: $$(call get_bin_dep, $$^)  | $(OBJ) $(BUILD_DIR)
	$(eval FROM_SRC=$$(call get_src, $$(call remove_root, $$@), $(TEST_DIR)))
	$(call make_dep, $(FROM_SRC), $@)
	$(call compile, $(FROM_SRC), $@, $(OBJ) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS))
	@$(call run_test, $@)

$(OBJ): %.o: $$(call get_src, $$@, $(SRC_DIR)) | $(BUILD_DIR)
	$(call make_dep, $<, $@)
	$(call compile, $<, $@, -c $(CXXFLAGS))

test-auto: $(TEST)

test: $(TEST)
	@for t in $(TEST); do \
		$(call run_test, $$t); \
	done

ifeq "$(strip $(OBJ))" ""
lib:
else
lib: lib_dir $(LIB_STATIC) $(LIB_SHARED)
endif

lib_dir:
	@if [ ! -d $(LIB_DIR) ]; then \
		mkdir -pv $(LIB_DIR); \
	fi \

$(LIB_STATIC): $(OBJ)
	ar rcs $(LIB_DIR)/lib$(NAME).a $(OBJ)

$(LIB_SHARED): $(OBJ)
	$(CXX) -shared $(OBJ) -o $(LIB_DIR)/lib$(NAME).so

ifeq "$(strip $(BIN_COPY))" ""
bin:
else
bin: bin_dir $(BIN_COPY)
endif

bin_dir:
	@if [ ! -d $(BIN_DIR) ]; then \
		mkdir -pv $(BIN_DIR); \
	fi

$(BIN_COPY): %: $$(call get_bin, $$@)
	cp $< $@

scratch: $(SCRATCH_COPY)

$(SCRATCH_COPY): %: $$(subst $(SCRATCH_DIR), $(SCRATCH_BUILD_DIR), ./$$@)
	cp $< $@

include: $(HDR_COPY)

$(HDR_COPY): %: $$(subst $(HDR_DIR), $(SRC_DIR), ./$$@)
	@if [ ! -d $(dir $@) ]; then \
		mkdir -pv $(dir $@); \
	fi
	cp $< $@

$(BUILD_DIR): FORCE
	@for d in $(BUILD_SKEL); do \
		if [ ! -d $$d ]; then \
			mkdir -pv $$d; \
		fi; \
	done

info:
	@printf "$(HELP)"

FORCE:

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(HDR_DIR)
	@for f in $(SCRATCH); do \
		rm -fv $(SCRATCH_DIR)/$(basename $$f); \
	done

.PHONY: all clean test test-auto include lib bin scratch lib_dir bin_dir