trivial_module := y
required_by    := fuzz test
mock_module    := y

$(module_name)_mocksymbs := cache_struct_is_padded cache_file_exists_in_sysfs
$(module_name)_mockobjs  := $(builddir)/fand/cache.o

$(call ldmock,$($(module_name)_mocksymbs),$($(module_name)_mockobjs))

$(module_name)_mocksymbs := fanctrl_get_speed fanctrl_get_temp
$(module_name)_mockobjs  := $(builddir)/fand/fanctrl.o

$(call ldmock,$($(module_name)_mocksymbs),$($(module_name)_mockobjs))

$(module_name)_mocksymbs := hwmon_write_pwm
$(module_name)_mockobjs  := $(builddir)/fand/hwmon.o

$(call ldmock,$($(module_name)_mocksymbs),$($(module_name)_mockobjs))

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
