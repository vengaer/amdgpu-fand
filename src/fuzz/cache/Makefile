trivial_module           := y
required_by              := fuzz

FUZZLEN                  := 2048
covsymbs                 := cache_load cache_validate cache_unpack cache_struct_is_padded \
                            unpackf dfa_simulate  valist_strip_pointer dfa_fmtlen         \
                            dfa_valsize dfa_accept dfa_flags_to_fmttype dfa_bitflag_set   \
                            dfa_edge_match cache_file_exists_in_sysfs

$(module_name)_mocksymbs := cache_struct_is_padded
$(module_name)_mockobjs  := $(builddir)/fand/cache.o

$(call ldmock,$($(module_name)_mocksymbs), $($(module_name)_mockobjs))

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
