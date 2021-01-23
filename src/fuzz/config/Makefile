trivial_module           := y
required_by              := fuzz

FUZZLEN                  := 2048
covsymbs                 := config_parse regmatch_length config_regcomp \
                            config_replace_char config_strip_comments   \
                            config_line_empty config_set_interval       \
                            config_set_hysteresis config_set_matrix     \
                            config_set_throttle config_append_matrix    \
                            strscpy strstoul_range strsncpy

$(call ldmock,$($(module_name)_mocksymbs), $($(module_name)_mockobjs))

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
