ifndef FUZZIFACE
    comma  := ,
    ifaces := $(filter-out $(module_mk),$(notdir $(wildcard $(module_path)/*)))
    ifaces := $(patsubst %,%$(comma),$(filter-out $(lastword $(ifaces)),$(ifaces))) or $(lastword $(ifaces))
    $(error FUZZIFACE is not defined, must be either $(ifaces))
endif

$(call include-module,$(FUZZIFACE))

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
