/*
** kground.c
** Bindings in the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "kstate.h"
#include "kobject.h"
#include "kground.h"

#include "kghelpers.h"
#include "kgbooleans.h"
#include "kgeqp.h"
#include "kgequalp.h"
#include "kgsymbols.h"
#include "kgcontrol.h"
#include "kgpairs_lists.h"
#include "kgpair_mut.h"
#include "kgenvironments.h"
#include "kgenv_mut.h"
#include "kgcombiners.h"
#include "kgcontinuations.h"
#include "kgencapsulations.h"
#include "kgpromises.h"
#include "kgkd_vars.h"
#include "kgks_vars.h"
#include "kgnumbers.h"
#include "kgstrings.h"
#include "kgchars.h"
#include "kgports.h"
#include "kgbytevectors.h"
#include "kgvectors.h"
#include "kgsystem.h"
#include "kgerrors.h"
#include "kgkeywords.h"
#include "kgmodules.h"

#if KUSE_LIBFFI
#  include "kgffi.h"
#endif

/* for initing cont names */
#include "ktable.h"
#include "kstring.h"
#include "keval.h"
#include "krepl.h"

/*
** This is called once to save the names of the types of continuations
** used in the ground environment & repl
** TODO the repl should init its own names!
*/
void kinit_cont_names(klisp_State *K)
{
    /* TEMP root and error continuations are set here (they are in kstate) */
    Table *t = tv2table(K->cont_name_table);
    add_cont_name(K, t, do_root_exit, "exit");
    add_cont_name(K, t, do_error_exit, "error");
    /* TEMP this is also in kstate */
    add_cont_name(K, t, do_interception, "do-interception");

    /* TEMP repl ones should be done in the interpreter, and not in
       the init state */
    kinit_repl_cont_names(K); 

    kinit_eval_cont_names(K);
    kinit_kghelpers_cont_names(K);

    kinit_booleans_cont_names(K);
    kinit_combiners_cont_names(K);
    kinit_environments_cont_names(K);
    kinit_env_mut_cont_names(K);
    kinit_pairs_lists_cont_names(K);
    kinit_continuations_cont_names(K);
    kinit_control_cont_names(K);
    kinit_promises_cont_names(K);
    kinit_ports_cont_names(K);
#if KUSE_LIBFFI
    kinit_ffi_cont_names(K);
#endif
    kinit_modules_cont_names(K);
}

/*
** This is called once to bind all symbols in the ground environment
*/
void kinit_ground_env(klisp_State *K)
{
    /*
    ** Initialize the combiners/vars for all supported modules
    */
    kinit_booleans_ground_env(K);
    kinit_eqp_ground_env(K);
    kinit_equalp_ground_env(K);
    kinit_symbols_ground_env(K);
    kinit_control_ground_env(K);
    kinit_pairs_lists_ground_env(K);
    kinit_pair_mut_ground_env(K);
    kinit_environments_ground_env(K);
    kinit_env_mut_ground_env(K);
    kinit_combiners_ground_env(K);
    kinit_continuations_ground_env(K);
    kinit_encapsulations_ground_env(K);
    kinit_promises_ground_env(K);
    kinit_kgkd_vars_ground_env(K);
    kinit_kgks_vars_ground_env(K);
    kinit_numbers_ground_env(K);
    kinit_strings_ground_env(K);
    kinit_chars_ground_env(K);
    kinit_ports_ground_env(K);
    kinit_bytevectors_ground_env(K);
    kinit_vectors_ground_env(K);
    kinit_system_ground_env(K);
    kinit_error_ground_env(K);
    kinit_keywords_ground_env(K);
    kinit_modules_ground_env(K);
#if KUSE_LIBFFI
    kinit_ffi_ground_env(K);
#endif
}
