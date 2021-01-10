$(call include-module,configure)
$(call include-module,common)
$(call include-module,mock)

$(call conditional-include-module,fanctl)
$(call conditional-include-module,fand)
$(call conditional-include-module,fuzz)
$(call conditional-include-module,test)

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
