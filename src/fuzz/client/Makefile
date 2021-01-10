trivial_module  := y
required_by     := fuzz

FUZZLEN         := 256
covsymbs        := client_init client_kill client_send_and_recv \
                   request_convert request_process_get strscpy

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
