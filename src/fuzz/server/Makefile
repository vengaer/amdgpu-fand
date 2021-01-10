trivial_module  := y
required_by     := fuzz

FUZZLEN         := 256
covsymbs        := server_init server_poll server_validate_request server_recv_and_respond server_kill \
                   server_pack_result pack_error pack_exit_rsp pack_matrix pack_speed pack_temp packf  \
                   valist_strip_pointer valist_strip_integral dfa_fmtlen dfa_valsize dfa_simulate      \
                   dfa_flags_to_fmttype dfa_accept dfa_bitflag_set dfa_edge_match

ifeq ($(module_name),)
target := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),recurse)

.PHONY: $(target)
$(target):
	@$(MAKE) -C .. $(MAKECMDGOALS) --no-print-directory
endif
