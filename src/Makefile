ifneq ($(findstring fand,$(configuration)),)
    $(call include-module,fand)
endif
ifneq ($(findstring fanctl,$(configuration)),)
    $(call include-module,fanctl)
endif

ifneq ($(findstring test,$(configuration)),)
    $(call include-module,fanctl)
    $(call include-module,fand)
    $(call include-module,test)
endif

$(call include-module,common)

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
