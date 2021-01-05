$(call include-module,configure)
$(call include-module,common)

$(call conditional-include-module,fand)
$(call conditional-include-module,fanctl)
$(call conditional-include-module,test)
$(call conditional-include-module,fuzz)

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
