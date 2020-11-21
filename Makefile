CC          ?= gcc

cflags      := -std=c11 -Wall -Wextra -Wpedantic -Waggregate-return -Wbad-function-cast \
                -Wcast-qual -Wfloat-equal -Wmissing-include-dirs -Wnested-externs -Wpointer-arith \
                -Wredundant-decls -Wshadow -Wunknown-pragmas -Wswitch -Wundef -Wunused -Wwrite-strings \
                -MD -MP -c
cppflags    :=

ldflags     :=
ldlibs      :=

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

incdirs     :=
fand_objs   :=
fanctl_objs :=

fand        := amdgpu-fand-3.0
fanctl      := amdgpu-fanctl-3.0

# $(call mk-module-build-dir)
define mk-module-build-dir
$(shell $(MKDIR) $(patsubst $(srcdir)/%,$(builddir)/%,$(module_path)))
endef

# $(call stack-top, stack_name)
define stack-top
$(subst :,,$(firstword $($(1))))
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
$(eval trivial_module := $(stack-top,trivial_stack))
$(eval module_path := $(patsubst %/$(module_name),%,$(module_path)))
$(call stack-pop,module_stack)
$(eval module_name := $(call stack-top,module_stack))
endef

# $(call include-module, module_name)
define include-module
$(call include-module-prologue,$(1))
$(eval include $(module_path)/$(module_mk))
$(call include-module-epilogue)
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
$(foreach t,$(required_by),$(eval $(t)_objs += $(__obj)))
$(eval cppflags += -I$(module_path))
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
$(if $(MAKECMDGOALS),
    $(if $(findstring $(fand),$(MAKECMDGOALS)),
        $(eval __cfg += fand))
    $(if $(findstring $(fanctl),$(MAKECMDGOALS)),
        $(eval __cfg += fanctl)),
  $(eval __cfg += fand fanctl))
$(__cfg)
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

module_stack   := $(stack_top_symb)
trivial_stack  := $(stack_top_symb)
trivial_module := n

prepare_deps   :=
build_deps     :=
target_deps    :=

configuration  := $(call build-configuration)

.PHONY: all
all: $(fand)

ifneq ($(configuration),)
    $(call include-module,src)
endif

$(call override-implicit-vars)

$(fand): $(target_deps) $(fand_objs)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(fanctl): $(target_deps) $(fanctl_objs)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(builddir)/%.$(oext): $(srcdir)/%.$(cext)
	$(call echo-cc,$@)
	$(QUIET)$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	$(QUIET)$(RM) $(builddir) $(fand) $(fanctl)
