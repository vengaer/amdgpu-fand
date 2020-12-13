$(module_name)_target   := $(builddir)/fand_configure
$(module_name)_src      := $(wildcard $(module_path)/*.$(cext))
$(module_name)_obj      := $(patsubst $(srcdir)/%,$(builddir)/%, $($(module_name)_src:.$(cext)=.$(oext)))
$(module_name)_builddir := $(patsubst $(srcdir)/%,$(builddir)/%,$(module_path))

endian_mk               := $(builddir)/endian.mk

prepare_deps += $(endian_mk) $($(module_name)_builddir)
build_deps   += $($(module_name)_target)
target_deps  += $(eval include $(endian_mk))
cppflags     += -DFAND_$(if $(filter-out y,$(little_endian)),BIG,LITTLE)_ENDIAN

$($(module_name)_target): $($(module_name)_obj)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$($(module_name)_obj): $($(module_name)_src)
	$(call echo-cc,$@)
	$(QUIET)$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS)

$(endian_mk): $($(module_name)_target)
	$(QUIET)$^ $@

$($(module_name)_builddir):
	$(QUIET)$(MKDIR) $@

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
