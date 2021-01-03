CC          ?= gcc

FAND        ?= amdgpu-fand
FANCTL      ?= amdgpu-fanctl
FAND_TEST   ?= amdgpu-testd
FAND_FUZZ   ?= amdgpu-fuzzd

cflags      := -std=c11 -Wall -Wextra -Wpedantic -Waggregate-return -Wbad-function-cast                \
                -Wcast-qual -Wfloat-equal -Wmissing-include-dirs -Wnested-externs -Wpointer-arith      \
                -Wredundant-decls -Wshadow -Wunknown-pragmas -Wswitch -Wundef -Wunused -Wwrite-strings \
                -MD -MP -c -g
cppflags    := -D_GNU_SOURCE

ldflags     :=
ldlibs      := -lm

TOUCH       := touch
QUIET       := @
MKDIR       := mkdir -p
RM          := rm -rf

cext        := c
oext        := o
depext      := d

root        := $(abspath $(CURDIR))

srcdir      := $(root)/src
builddir    := $(root)/build

audot       := audot/audot
docdir      := docs
graphsrc    := src/common/serialize.c
digraph     := $(docdir)/serialize.png

fand_objs   :=
fanctl_objs :=
test_objs    = $(filter-out %/main.$(oext),$(fand_objs) $(fanctl_objs))
fuzz_objs    = $(filter-out %/main.$(oext),$(fand_objs))

drm_support := $(if $(wildcard /usr/*/libdrm/amdgpu_drm.h),y,n)
cppflags    += $(if $(findstring _y_,_$(drm_support)_),-DFAND_DRM_SUPPORT)

# $(call mk-module-build-dir)
define mk-module-build-dir
$(shell $(MKDIR) $(patsubst $(srcdir)/%,$(builddir)/%,$(module_path)))
endef

# $(call stack-top, stack_name)
define stack-top
$(subst $(stack_top_symb),,$(firstword $($(1))))
endef

# $(call stack-push, stack_name, value)
define stack-push
$(eval $(1) := $(stack_top_symb)$(strip $(2)) $(subst $(stack_top_symb),,$($(1))))
endef

# $(call stack-pop, stack_name)
define stack-pop
$(eval __top := $(call stack-top,$(1)))
$(eval $(1) := $(stack_top_symb)$(subst $(stack_top_symb)$(__top) ,,$($(1))))
endef

# $(cal include-module-prologue, module_name)
define include-module-prologue
$(eval module_name := $(strip $(1)))
$(call stack-push,module_stack,$(module_name))
$(call stack-push,trivial_stack,$(trivial_module))
$(eval module_path := $(module_path)/$(module_name))
$(call mk-module-build-dir)
$(eval trivial_module := n)
$(eval required_by := )
endef

# $(call include-module-epilogue)
define include-module-epilogue
$(if $(findstring y,$(trivial_module)),
    $(call declare-trivial-c-module))
$(eval trivial_module := $(call stack-top,trivial_stack))
$(eval module_path := $(patsubst %/$(module_name),%,$(module_path)))
$(call stack-pop,module_stack)
$(call stack-pop,trivial_stack)
$(eval module_name := $(call stack-top,module_stack))
endef

# $(call include-module, module_name)
define include-module
$(call include-module-prologue,$(1))
$(eval include $(module_path)/$(module_mk))
$(call include-module-epilogue)
endef

# Includes the module $(module_name) if it is
# listed in the $(configuration) list
# $(call conditional-include-module, module_name)
define conditional-include-module
$(if $(findstring $(1),$(configuration)),
    $(call include-module,$(1)))
endef

# $(call add-build-dep, rule)
define add-build-dep
$(eval build_deps += $(1))
endef

# $(call add-target-dep, rule)
define add-target-dep
$(eval target_deps += $(1))
endef

# $(call declare-trivial-c-module)
define declare-trivial-c-module
$(eval __src    := $(wildcard $(module_path)/*.$(cext)))
$(eval __obj    := $(patsubst $(srcdir)/%,$(builddir)/%,$(__src:.$(cext)=.$(oext))))
$(foreach __co,$(cond_objs),
    $(if $(filter-out y,$($(firstword $(subst $(cond_separator), ,$(__co))))),
        $(eval __obj := $(filter-out %/$(lastword $(subst $(cond_separator), ,$(__co))).$(oext),$(__obj)))))
$(eval cond_objs := )
$(foreach t,$(required_by),$(eval $(t)_objs += $(__obj)))
$(eval cppflags += -I$(module_path))
endef

# Results in the object file indicated by object_basename
# being filtered out if the flag is not set to 'y'
# $(call conditional-obj,object_basename,flag)
define conditional-obj
$(eval obj_deps := $(obj_deps) $(2):$(1))
endef

# $(call echo-build-step, program, file)
define echo-build-step
$(info [$(1)] $(notdir $(2)))
endef

# $(call echo-cc, file)
define echo-cc
$(call echo-build-step,CC,$(1))
endef

# $(call echo-ld, target)
define echo-ld
$(call echo-build-step,LD,$(1))
endef

# $(call build-configuration)
define build-configuration
$(strip
$(eval __cfg := )
$(if $(MAKECMDGOALS),
    $(if $(or $(findstring $(FAND_TEST),$(MAKECMDGOALS)), $(findstring test,$(MAKECMDGOALS))),
        $(eval __cfg := fand fanctl test),
      $(if $(or $(findstring $(FAND_FUZZ),$(MAKECMDGOALS)), $(findstring fuzz,$(MAKECMDGOALS))),
          $(eval __cfg := fand fuzz),
        $(if $(or $(findstring $(FAND),$(MAKECMDGOALS)), $(findstring fand,$(MAKECMDGOALS))),
            $(eval __cfg += fand))
        $(if $(or $(findstring $(FANCTL),$(MAKECMDGOALS)), $(findstring fanctl,$(MAKECMDGOALS))),
            $(eval __cfg += fanctl)))),
  $(eval __cfg += fand fanctl))
$(__cfg)
)
endef

# $(call override-implicit-vars)
define override-implicit-vars
$(eval override CFLAGS   += $(cflags))
$(eval override CPPFLAGS += $(cppflags))
$(eval override LDFLAGS  += $(ldflags))
$(eval override LDLIBS   += $(ldlibs))
endef

mk-build-root  := $(shell $(MKDIR) $(builddir))
module_mk      := Makefile
module_path    := $(root)

stack_top_symb := :
cond_separator := :

prepare_deps   :=
build_deps     :=
target_deps    :=

configuration  := $(call build-configuration)

prepare        := $(builddir)/.prepare.stamp

.PHONY: all
all: $(FAND) $(FANCTL)

ifneq ($(configuration),)
    $(call include-module,src)
endif

$(call override-implicit-vars)

$(prepare): $(prepare_deps)
	$(QUIET)$(TOUCH) $@

$(FAND): $(target_deps) $(fand_objs)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(FANCTL): $(target_deps) $(fanctl_objs)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(FAND_TEST): $(target_deps) $(test_objs)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(FAND_FUZZ): CC       := clang
$(FAND_FUZZ): CFLAGS   += -fsanitize=fuzzer
$(FAND_FUZZ): CPPFLAGS := -DFAND_FUZZ_CONFIG $(CPPFLAGS)
$(FAND_FUZZ): LDFLAGS  += -fsanitize=fuzzer
$(FAND_FUZZ): $(target_deps) $(fuzz_objs)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(builddir)/%.$(oext): $(srcdir)/%.$(cext) | $(build_deps)
	$(call echo-cc,$@)
	$(QUIET)$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS)

$(audot):
	$(QUIET)git submodule update --init

$(digraph): $(graphsrc) | $(docdir) $(audot)
	$(info [audot] $@)
	$(QUIET)$(audot) -o $@ $^

$(docdir):
	$(QUIET)mkdir -p $@

.PHONY: prepare
prepare: $(prepare)

.PHONY: fand
fand:  $(FAND)

.PHONY: fanctl
fanctl: $(FANCTL)

.PHONY: test
test: $(FAND_TEST)

.PHONY: fuzz
fuzz: $(FAND_FUZZ)

.PHONY: doc
doc: $(digraph)

.PHONY: clean
clean:
	$(QUIET)$(RM) $(builddir) $(FAND) $(FANCTL) $(FAND_TEST) $(FAND_FUZZ) $(docdir)
