﻿
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>

#include <kernwin.hpp>
#include <ieee.h>
#include <hexrays.hpp>
#include <functional>
#include <array>
#include <list>
#include <string>
#include "cs_core.hpp"
#define HEXEXTV2
#include "micro_on_70.hpp"

#include "microgen_additions/microgen_core.hpp"
#include "combine_rules/combine_core.hpp"
#include "hexutils/hexutils.hpp"
#include "preoptimization-postprocess/prepostprocess.hpp"

#include "ctree/ctree.hpp"
CS_COLD_CODE
static bool dump_microcode_(mbl_array_t* mba) {
	dump_mba(mba);
	return false;
}


static bool g_installed = false;

static hexext_arch_e g_currarch;

hexext_arch_e hexext::currarch() {
	return g_currarch;

}
class scanspoil_handler_t : public action_handler_t {

	virtual int idaapi activate(action_activation_ctx_t* ctx) {

		/*auto mba = hexext::gen_microcode_ex(get_screen_ea(), hexext_micromat_e::preopt);
		msg("I have stinky one.\n");

		release_dup_mba((dup_mbl_array_t*)mba);*/

		func_t* ffunc = get_next_func(0);
		std::vector<mbl_array_t*> allfuncs{};

		while (ffunc) {
			allfuncs.push_back(hexext::gen_microcode_ex(ffunc->start_ea, hexext_micromat_e::preopt));
			ffunc = get_next_func(ffunc->start_ea);
		}

		return 0;
	}
	virtual action_state_t idaapi update(action_update_ctx_t* ctx) {
		return AST_ENABLE;
	}
};
static scanspoil_handler_t scanspoil{};
static action_desc_t dadescr{ sizeof(action_desc_t), "hexext.scanspoil", "Scan spoiled lists", &scanspoil, nullptr, "Ctrl-7", "", 0,0 };
constexpr uintptr_t the_offs = 0x73518AE0ull - 0x732B0000ull;

bool dump_the_patterns(mbl_array_t* unused) {
	qvector<mbl_array_t*>* pats = 
		reinterpret_cast<qvector<mbl_array_t*>*>(((uintptr_t)get_hexmod()) + the_offs);

	for (auto&& pat : *pats) {
		dump_mba(pat);
	}
	
	return false;
}


CS_COLD_CODE
int idaapi init(void)
{

	

	if (!strcmp(inf.procname, "ARM")) {
		g_currarch = hexext_arch_e::ARM;

	}
	else if (!strcmp(inf.procname, "metapc")) {
		g_currarch = hexext_arch_e::x86;
	}

	else {
		return PLUGIN_SKIP;
	}

	hexext_internal::init_hexext();

	if (!hexdsp)
		return PLUGIN_SKIP;

	auto hexver = get_hexrays_version();

	cs_assert(hexver);

	if (!strcmp("7.0.0.170914", hexver)) {
#if 1
		hexext::install_glbopt_cb(dump_microcode_);
		register_action(dadescr);
#endif
#if 0
		hexext::install_glbopt_cb(dump_the_patterns);
		cs_assert(init_hexmod(g_currarch == hexext_arch_e::ARM));
#endif
		msg("Hexext is loaded! Use Ctrl-2 to toggle optimizations on and off!\n");
		
		

		
		return PLUGIN_KEEP;
	}
	else {
		msg("Hexext only supports decompilers with version 7.0.0.170914 right now, this version is %s. Unloading.\n", hexver);
		return PLUGIN_SKIP;
	}
	

}


CS_COLD_CODE
bool idaapi run(size_t)
{
	


	g_installed = !g_installed;

	if (g_installed) {
		msg("Installing Hexext optimizations.\n");
	}
	else {
		msg("Removing Hexext optimizations.\n");
	}
	toggle_common_combination_rules(g_installed);
	toggle_archspec_combination_rules(g_installed);
	toggle_microgen_additions(g_installed);
	toggle_ctree_rules(g_installed);
	hexext::install_preopt_cb(prepostprocess_run, g_installed);

	return true;

}
CS_COLD_CODE
void idaapi term(void)
{

	hexext_internal::deinit_hexext();
	
}

plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  0,           // plugin flags
  init,                 // initialize
  term,                 // terminate. this pointer may be NULL.
  run,                  // invoke plugin
  NULL,                 // long comment about the plugin
  NULL,                 // multiline help about the plugin
  "Hexext",       // the preferred short name of the plugin
  "Ctrl+2"                  // the preferred hotkey to run the plugin
};
