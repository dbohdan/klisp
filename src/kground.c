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
#include "kgblobs.h"
#include "kgsystem.h"

/* for initing cont names */
#include "ktable.h"
#include "kstring.h"
#include "keval.h"
#include "krepl.h"
#include "kscript.h"

/* for init_cont_names */
#define add_cont_name(K_, t_, c_, n_)					\
    { TValue str = kstring_new_b_imm(K_, n_);				\
    TValue *node = klispH_set(K_, t_, p2tv(c_));			\
    *node = str;							\
    }

/*
** This is called once to save the names of the types of continuations
** used in the ground environment & repl
** TODO the repl should init its own names!
*/
void kinit_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);

    /* REPL, root-continuation & error-continuation */
    add_cont_name(K, t, do_repl_exit, "exit");
    add_cont_name(K, t, do_repl_read, "repl-read");
    add_cont_name(K, t, do_repl_eval, "repl-eval");
    add_cont_name(K, t, do_repl_loop, "repl-loop");
    add_cont_name(K, t, do_repl_error, "repl-report-error");

    /* SCRIPT, root-continuation & error-continuation */
    add_cont_name(K, t, do_script_exit, "script-exit");
    add_cont_name(K, t, do_script_error, "script-report-error");

    /* GROUND ENV */
    add_cont_name(K, t, do_eval_ls, "eval-list");
    add_cont_name(K, t, do_combine, "eval-combine");
    add_cont_name(K, t, do_Sandp_Sorp, "eval-booleans");
    add_cont_name(K, t, do_seq, "eval-sequence");
    add_cont_name(K, t, do_map, "map-acyclic-part");
    add_cont_name(K, t, do_map_encycle, "map-encycle!");
    add_cont_name(K, t, do_map_ret, "map-ret");
    add_cont_name(K, t, do_map_cycle, "map-cyclic-part");
    add_cont_name(K, t, do_extended_cont, "extended-cont");
    add_cont_name(K, t, do_pass_value, "pass-value");
    add_cont_name(K, t, do_select_clause, "select-clause");
    add_cont_name(K, t, do_cond, "eval-cond-list");
    add_cont_name(K, t, do_for_each, "for-each");
    add_cont_name(K, t, do_let, "eval-let");
    add_cont_name(K, t, do_bindsp, "eval-$binds?-env");
    add_cont_name(K, t, do_let_redirect, "eval-let-redirect");
    add_cont_name(K, t, do_remote_eval, "eval-remote-eval-env");
    add_cont_name(K, t, do_b_to_env, "bindings-to-env");
    add_cont_name(K, t, do_match, "match-ptree");
    add_cont_name(K, t, do_set_eval_obj, "set-eval-obj");
    add_cont_name(K, t, do_import, "import-bindings");
    add_cont_name(K, t, do_return_value, "return-value");
    add_cont_name(K, t, do_unbind, "unbind-dynamic-var");
    add_cont_name(K, t, do_filter, "filter-acyclic-part");
    add_cont_name(K, t, do_filter_encycle, "filter-encycle!");
    add_cont_name(K, t, do_ret_cdr, "return-cdr");
    add_cont_name(K, t, do_filter_cycle, "filter-cyclic-part");
    add_cont_name(K, t, do_reduce_prec, "reduce-precycle");
    add_cont_name(K, t, do_reduce_combine, "reduce-combine");
    add_cont_name(K, t, do_reduce_postc, "reduce-postcycle");
    add_cont_name(K, t, do_reduce, "reduce-acyclic-part");
    add_cont_name(K, t, do_reduce_cycle, "reduce-cyclic-part");
    add_cont_name(K, t, do_close_file_ret, "close-file-and-ret");
    add_cont_name(K, t, do_handle_result, "handle-result");
    add_cont_name(K, t, do_interception, "do-interception");
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
    kinit_blobs_ground_env(K);
    kinit_system_ground_env(K);

    /*
    ** Initialize the names of the continuation used in
    ** the supported modules to aid in debugging/error msgs
    */
    /* MAYBE some/most/all of these could be done in each module */
    kinit_cont_names(K);
}
