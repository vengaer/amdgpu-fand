$(call mk-module-build-dir)

$(module_name)_target := $(builddir)/fand_configure
$(module_name)_obj    := $(patsubst $(srcdir)/%,$(builddir)/%,$(patsubst %.$(cext),%.$(oext),$(wildcard $(module_path)/*.$(cext))))

$($(module_name)_target): $($(module_name)_obj)
	$(call echo-ld,$@)
	$(QUIET)$(CC) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(config_mk): $($(module_name)_target)
	$(QUIET)$^ $@

$(patsubst $(srcdir)/%,$(builddir)/%,$(module_path))/%.$(oext): $(module_path)/%.$(cext)
	$(call echo-cc,$@)
	$(QUIET)$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS)

prepare_deps += $(config_mk)
build_deps   += $(eval include $(config_mk)) $(call set-libc-flags)

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
