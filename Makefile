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

# $(call module-stack-top)
define module-stack-top
$(subst :,,$(firstwod $(module_stack)))
endef

# $(call module-stack-push, module_name)
define module-stack-push
$(eval module_stack := $(stack_top_symb)$(strip $(1)) $(subst $(stack_top_symb),,$(module_stack)))
endef

# $(call module-stack-pop)
define module-stack-pop
$(eval module_stack := $(stack_top_symb)$(subst $(stack_top_symv)$(module_name) ,,$(module_stack)))
endef

# $(cal include-module-prologue, module_name)
define include-module-prologue
$(eval module_name := $(strip $(1)))
$(call module-stack-push,$(module_name))
$(eval module_path := $(module_path)/$(module_name))
$(call mk-module-build-dir)
$(eval trivial_module := n)
$(eval required_by := )
endef

# $(call include-module-epilogue)
define include-module-epilogue
$(if $(findstring y,$(trivial_module)),
    $(call declare-trivial-c-module))
$(eval trivial_module := n)
$(eval module_path := $(patsubst %/$(module_name),%,$(module_path)))
$(call module-stack-pop)
$(eval module_name := $(call module-stack-top))
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
