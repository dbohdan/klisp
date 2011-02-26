/*
** kstate.c
** klisp vm state
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is mostly from Lua.
*/

#include <stddef.h>

#include "klisp.h"
#include "kstate.h"
#include "kmem.h"

/*
** State creation and destruction
*/
klisp_State *klisp_newstate (klisp_Alloc f, void *ud) {
  klisp_State *K;
  void *k = (*f)(ud, NULL, 0, state_size());
  if (k == NULL) return NULL;
  void *s = (*f)(ud, NULL, 0, KS_ISSIZE * sizeof(TValue));
  if (s == NULL) { 
      (*f)(ud, k, state_size(), 0); 
      return NULL;
  }
  void *b = (*f)(ud, NULL, 0, KS_ITBSIZE);
  if (b == NULL) {
      (*f)(ud, k, state_size(), 0); 
      (*f)(ud, s, KS_ISSIZE * sizeof(TValue), 0); 
      return NULL;
  }

  K = (klisp_State *) k;

  K->symbol_table = KNIL;
  /* TODO: create a continuation */
  K->curr_cont = NULL;
  K->ret_value = KINERT;

  K->frealloc = f;
  K->ud = ud;

  /* current input and output */
  K->curr_in = stdin;
  K->curr_out = stdout;
  K->filename_in = "*STDIN*";
  K->filename_out = "*STDOUT*";

  /* TODO: more gc info */
  K->totalbytes = KS_ISSIZE + state_size();

  /* TEMP: err */
  /* do nothing for now */

  K->ssize = KS_ISSIZE;
  K->stop = 0; /* stack is empty */
  K->sbuf = (TValue *)s;

  /* initialize tokenizer */
  ks_tbsize(K) = KS_ITBSIZE;
  ks_tbidx(K) = 0; /* buffer is empty */
  ks_tbuf(K) = (char *)b;

  /* XXX: For now just hardcode it to 8 spaces tab-stop */
  K->ktok_source_info.tab_width = 8;
  K->ktok_source_info.filename = "*STDIN*";
  ktok_init(K);
  ktok_reset_source_info(K);

  /* initialize reader */
  K->shared_dict = KNIL;

  /* initialize writer */
  return K;
}

void klisp_close (klisp_State *K)
{
    /* TODO: free memory for all objects */
    klispM_freemem(K, ks_sbuf(K), ks_ssize(K));
    /* NOTE: this needs to be done "by hand" */
    (*(K->frealloc))(K->ud, K, state_size(), 0);
}
