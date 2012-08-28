/*
** kwrite.c
** Writer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "kwrite.h"
#include "kobject.h"
#include "kinteger.h"
#include "krational.h"
#include "kreal.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kkeyword.h"
#include "kstate.h"
#include "kerror.h"
#include "ktable.h"
#include "kport.h"
#include "kenvironment.h"
#include "kbytevector.h"
#include "kvector.h"
#include "ktoken.h" /* for identifier checking */

/*
** Stack for the write FSM
** 
*/
#define push_data(ks_, data_) (ks_spush(ks_, data_))
#define pop_data(ks_) (ks_sdpop(ks_))
#define get_data(ks_) (ks_sget(ks_))
#define data_is_empty(ks_) (ks_sisempty(ks_))

void kwrite_error(klisp_State *K, char *msg)
{
    /* all cleaning is done in throw 
       (stacks, shared_dict, rooted objs) */
    klispE_throw_simple(K, msg);
}

void kw_printf(klisp_State *K, const char *format, ...)
{
    va_list argp;
    TValue port = K->curr_port;

    if (ttisfport(port)) {
        FILE *file = kfport_file(port);
        va_start(argp, format);
        /* LOCK: only a single lock should be acquired */
        klisp_unlock(K);
        int ret = vfprintf(file, format, argp);
        klisp_lock(K);
        va_end(argp);

        if (ret < 0) {
            clearerr(file); /* clear error for next time */
            kwrite_error(K, "error writing");
            return;
        }
    } else if (ttismport(port)) {
        /* bytevector ports shouldn't write chars */
        klisp_assert(kport_is_textual(port));
        /* string port */
        uint32_t size;
        int written;
        uint32_t off = kmport_off(port);

        size = kstring_size(kmport_buf(port)) -
            kmport_off(port) + 1;

        /* size is always at least 1 (for the '\0') */
        va_start(argp, format);
        written = vsnprintf(kstring_buf(kmport_buf(port)) + off, 
                            size, format, argp);
        va_end(argp);

        if (written >= size) { /* space wasn't enough */
            kmport_resize_buffer(K, port, off + written);
            /* size may be greater than off + written, so get again */
            size = kstring_size(kmport_buf(port)) - off + 1;
            va_start(argp, format);
            written = vsnprintf(kstring_buf(kmport_buf(port)) + off, 
                                size, format, argp);
            va_end(argp);
            if (written < 0 || written >= size) {
                /* shouldn't happen */
                kwrite_error(K, "error writing");
                return;
            }
        }
        kmport_off(port) = off + written;
    } else {
        kwrite_error(K, "unknown port type");
        return;
    }
}

void kw_flush(klisp_State *K) { kwrite_flush_port(K, K->curr_port); }
    

/* TODO: check for return codes and throw error if necessary */
#define KDEFAULT_NUMBER_RADIX 10
void kw_print_bigint(klisp_State *K, TValue bigint)
{
    int32_t radix = KDEFAULT_NUMBER_RADIX;
    int32_t size = kbigint_print_size(bigint, radix); 
    krooted_tvs_push(K, bigint);
    /* here we are using 1 byte extra, because size already includes
       1 for the terminator, but better be safe than sorry */
    TValue buf_str = kstring_new_s(K, size);
    krooted_tvs_push(K, buf_str);

    char *buf = kstring_buf(buf_str);
    kbigint_print_string(K, bigint, radix, buf, size);
    kw_printf(K, "%s", buf);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
}

void kw_print_bigrat(klisp_State *K, TValue bigrat)
{
    int32_t radix = KDEFAULT_NUMBER_RADIX;
    int32_t size = kbigrat_print_size(bigrat, radix); 
    krooted_tvs_push(K, bigrat);
    /* here we are using 1 byte extra, because size already includes
       1 for the terminator, but better be safe than sorry */
    TValue buf_str = kstring_new_s(K, size);
    krooted_tvs_push(K, buf_str);

    char *buf = kstring_buf(buf_str);
    kbigrat_print_string(K, bigrat, radix, buf, size);
    kw_printf(K, "%s", buf);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
}

void kw_print_double(klisp_State *K, TValue tv_double)
{
    int32_t size = kdouble_print_size(tv_double); 
    krooted_tvs_push(K, tv_double);
    /* here we are using 1 byte extra, because size already includes
       1 for the terminator, but better be safe than sorry */
    TValue buf_str = kstring_new_s(K, size);
    krooted_tvs_push(K, buf_str);

    char *buf = kstring_buf(buf_str);
    kdouble_print_string(K, tv_double, buf, size);
    kw_printf(K, "%s", buf);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
}

/*
** Helper for printing strings.
** If !displayp it prints the surrounding double quotes
** and escapes backslashes, double quotes,
** and non printable chars (including NULL). 
** if displayp it doesn't include surrounding quotes and just
** converts non-printable characters to spaces
*/
void kw_print_string(klisp_State *K, TValue str)
{
    int size = kstring_size(str);
    char *buf = kstring_buf(str);
    char *ptr = buf;
    int i = 0;

    if (!K->write_displayp)
        kw_printf(K, "\"");

    while (i < size) {
        /* find the longest printf-able substring to avoid calling printf
           for every char */
        for (ptr = buf; 
             i < size && *ptr != '\0' &&
                 (*ptr >= 32 && *ptr < 127) &&
                 (K->write_displayp || (*ptr != '\\' && *ptr != '"')); 
             i++, ptr++)
            ;

        /* NOTE: this work even if ptr == buf (which can only happen the 
           first or last time) */
        char ch = *ptr;
        *ptr = '\0';
        kw_printf(K, "%s", buf);
        *ptr = ch;

        for(; i < size && (*ptr == '\0' || (*ptr < 32 || *ptr >= 127) ||
                           (!K->write_displayp && 
                            (*ptr == '\\' || *ptr == '"')));
            ++i, ptr++) {
            /* This are all ASCII printable characters (including space,
               and exceptuating '\' and '"' if !displayp) */
            char *fmt;
            /* must be uint32_t to support all unicode chars
               in the future */
            uint32_t arg;
            ch = *ptr;
            if (K->write_displayp) {
                fmt = "%c";
                /* in display only show tabs and newlines, 
                   all other non printables are shown as spaces */
                arg = (uint32_t) ((ch == '\r' || ch == '\n' || ch == '\t')? 
                                  ch : ' ');
            } else {
                switch(*ptr) {
                    /* regular \ escapes */
                case '\"': fmt = "\\%c"; arg = (uint32_t) '"'; break;
                case '\\': fmt = "\\%c"; arg = (uint32_t) '\\'; break;
                case '\0': fmt = "\\%c"; arg = (uint32_t) '0'; break;
                case '\a': fmt = "\\%c"; arg = (uint32_t) 'a'; break;
                case '\b': fmt = "\\%c"; arg = (uint32_t) 'b'; break;
                case '\t': fmt = "\\%c"; arg = (uint32_t) 't'; break;
                case '\n': fmt = "\\%c"; arg = (uint32_t) 'n'; break;
                case '\r': fmt = "\\%c"; arg = (uint32_t) 'r'; break;
                case '\v': fmt = "\\%c"; arg = (uint32_t) 'v'; break;
                case '\f': fmt = "\\%c"; arg = (uint32_t) 'f'; break;
                    /* for the rest of the non printable chars, 
                       use hex escape */
                default: fmt = "\\x%x;"; arg = (uint32_t) ch; break;
                }
            }
            kw_printf(K, fmt, arg);
        }
        buf = ptr;
    }
			
    if (!K->write_displayp)
        kw_printf(K, "\"");
}

/*
** Helper for printing symbols & keywords.
** If symbol is not a regular identifier it
** uses the "|...|" syntax, escaping '|', '\' and 
** non printing characters.
*/
void kw_print_symbol_buf(klisp_State *K, char *buf, uint32_t size)
{
    /* first determine if it's a simple identifier */
    bool identifierp;
    if (size == 0)
        identifierp = false;
    else if (size == 1 && *buf == '.')
        identifierp = false;
    else if (size == 1 && (*buf == '+' || *buf == '-'))
        identifierp = true;
    else if (*buf == tolower(*buf) && ktok_is_initial(*buf)) {
        char *ptr = buf;
        uint32_t i = 0;
        identifierp = true;
        while (identifierp && i < size) {
            char ch = *ptr++;
            ++i;
            if (tolower(ch) != ch || !ktok_is_subsequent(ch))
                identifierp = false;
        }
    } else
        identifierp = false;

    if (identifierp) {
        /* no problem, just a simple string */
        kw_printf(K, "%s", buf);
        return;
    } 

    /*
    ** In case we get here, we'll have to use the "|...|" syntax
    */
    char *ptr = buf;
    int i = 0;

    kw_printf(K, "|");

    while (i < size) {
        /* find the longest printf-able substring to avoid calling printf
           for every char */
        for (ptr = buf; 
             i < size && *ptr != '\0' &&
                 (*ptr >= 32 && *ptr < 127) &&
                 (*ptr != '\\' && *ptr != '|'); 
             i++, ptr++)
            ;

        /* NOTE: this work even if ptr == buf (which can only happen the 
           first or last time) */
        char ch = *ptr;
        *ptr = '\0';
        kw_printf(K, "%s", buf);
        *ptr = ch;

        for(; i < size && (*ptr == '\0' || (*ptr < 32 || *ptr >= 127) ||
                           (*ptr == '\\' || *ptr == '|'));
            ++i, ptr++) {
            /* This are all ASCII printable characters (including space,
               and exceptuating '\' and '|') */
            char *fmt;
            /* must be uint32_t to support all unicode chars
               in the future */
            uint32_t arg;
            ch = *ptr;
            switch(*ptr) {
                /* regular \ escapes */
            case '|': fmt = "\\%c"; arg = (uint32_t) '|'; break;
            case '\\': fmt = "\\%c"; arg = (uint32_t) '\\'; break;
                /* for the rest of the non printable chars, 
                   use hex escape */
            default: fmt = "\\x%x;"; arg = (uint32_t) ch; break;
            }
            kw_printf(K, fmt, arg);
        }
        buf = ptr;
    }
			
    kw_printf(K, "|");
}

void kw_print_symbol(klisp_State *K, TValue sym)
{
    kw_print_symbol_buf(K, ksymbol_buf(sym), ksymbol_size(sym));
}

void kw_print_keyword(klisp_State *K, TValue keyw)
{
    kw_printf(K, "#:");
    kw_print_symbol_buf(K, kkeyword_buf(keyw), kkeyword_size(keyw));
}

/*
** Mark initialization and clearing
*/
/* GC: root is rooted */
void kw_clear_marks(klisp_State *K, TValue root)
{
    assert(ks_sisempty(K));
    push_data(K, root);

    while(!data_is_empty(K)) {
        TValue obj = get_data(K);
        pop_data(K);
	
        if (ttispair(obj)) {
            if (kis_marked(obj)) {
                kunmark(obj);
                push_data(K, kcdr(obj));
                push_data(K, kcar(obj));
            }
        } else if (ttisstring(obj) && (kis_marked(obj))) {
            kunmark(obj);
        }
    }
    assert(ks_sisempty(K));
}

/*
** NOTE: 
**   - The objects that appear more than once are marked with a -1.
**   that way, the first time they are found in write, a shared def
**   token will be generated and the mark updated with the number;
**   from then on, the writer will generate a shared ref each time
**   it appears again.
**   - The objects that appear only once are marked with a #t to 
**   find repetitions and to allow unmarking after write
*/
/* GC: root is rooted */
void kw_set_initial_marks(klisp_State *K, TValue root)
{
    assert(ks_sisempty(K));
    push_data(K, root);
    
    while(!data_is_empty(K)) {
        TValue obj = get_data(K);
        pop_data(K);

        if (ttispair(obj)) {
            if (kis_unmarked(obj)) {
                kmark(obj); /* this mark just means visited */
                push_data(K, kcdr(obj));
                push_data(K, kcar(obj));
            } else {
                /* this mark means it will need a ref number */
                kset_mark(obj, i2tv(-1));
            }
        } else if (ttisstring(obj)) {
            if (kis_unmarked(obj)) {
                kmark(obj); /* this mark just means visited */
            } else {
                /* this mark means it will need a ref number */
                kset_mark(obj, i2tv(-1));
            }
        }
        /* all other types of object don't matter */
    }
    assert(ks_sisempty(K));
}

#if KTRACK_NAMES
void kw_print_name(klisp_State *K, TValue obj)
{
    kw_printf(K, ": ");
    kw_print_symbol(K, kget_name(K, obj));
}
#endif /* KTRACK_NAMES */

#if KTRACK_SI
/* Assumes obj has a si */
void kw_print_si(klisp_State *K, TValue obj)
{
    /* should be an improper list of 2 pairs,
       with a string and 2 fixints */
    TValue si = kget_source_info(K, obj);
    kw_printf(K, " @ ");
    /* this is a hack, would be better to change the interface of 
       kw_print_string */
    bool saved_displayp = K->write_displayp; 
    K->write_displayp = true; /* avoid "s and escapes */

    TValue str = kcar(si);
    int32_t row = ivalue(kcadr(si));
    int32_t col = ivalue(kcddr(si));
    kw_print_string(K, str);
    kw_printf(K, " (line: %d, col: %d)", row, col);

    K->write_displayp = saved_displayp;
}
#endif /* KTRACK_SI */

/* obj should be a continuation */
/* REFACTOR: move get cont name to a function somewhere else */
void kw_print_cont_type(klisp_State *K, TValue obj)
{
    bool saved_displayp = K->write_displayp; 
    K->write_displayp = true; /* avoid "s and escapes */

    Continuation *cont = tv2cont(obj);

    /* XXX lock? */
    const TValue *node = klispH_get(tv2table(G(K)->cont_name_table),
                                    p2tv(cont->fn));

    char *type;
    if (node == &kfree) {
        type = "?";
    } else {
        klisp_assert(ttisstring(*node));
        type = kstring_buf(*node);
    }

    kw_printf(K, " (%s)", type);
    K->write_displayp = saved_displayp;
}

/*
** Writes all values except strings and pairs
*/
void kwrite_scalar(klisp_State *K, TValue obj)
{
    switch(ttype(obj)) {
    case K_TSTRING:
        /* shouldn't happen */
        klisp_assert(0);
        /* avoid warning */
        return;
    case K_TFIXINT:
        kw_printf(K, "%" PRId32, ivalue(obj));
        break;
    case K_TBIGINT:
        kw_print_bigint(K, obj);
        break;
    case K_TBIGRAT:
        kw_print_bigrat(K, obj);
        break;
    case K_TEINF:
        kw_printf(K, "#e%cinfinity", tv_equal(obj, KEPINF)? '+' : '-');
        break;
    case K_TIINF:
        kw_printf(K, "#i%cinfinity", tv_equal(obj, KIPINF)? '+' : '-');
        break;
    case K_TDOUBLE: {
        kw_print_double(K, obj);
        break;
    }
    case K_TRWNPV:
        /* ASK John/TEMP: until John tells me what should this be... */
        kw_printf(K, "#real");
        break;
    case K_TUNDEFINED:
        kw_printf(K, "#undefined");
        break;
    case K_TNIL:
        kw_printf(K, "()");
        break;
    case K_TCHAR: {
        if (K->write_displayp) {
            kw_printf(K, "%c", chvalue(obj));
        } else {
            char ch_buf[16]; /* should be able to contain hex escapes */
            char ch = chvalue(obj);
            char *ch_ptr;

            switch (ch) {
            case '\0':
                ch_ptr = "null";
                break;
            case '\a':
                ch_ptr = "alarm";
                break;
            case '\b':
                ch_ptr = "backspace";
                break;
            case '\t':
                ch_ptr = "tab";
                break;
            case '\n':
                ch_ptr = "newline";
                break;
            case '\r':
                ch_ptr = "return";
                break;
            case '\x1b':
                ch_ptr = "escape";
                break;
            case ' ':
                ch_ptr = "space";
                break;
            case '\x7f':
                ch_ptr = "delete";
                break;
            case '\v':
                ch_ptr = "vtab";
                break;
            default: {
                int i = 0;
                if (ch >= 32 && ch < 127) {
                    /* printable ASCII range */
                    /* (del(127) and space(32) were already considered, 
                       but it's clearer this way) */
                    ch_buf[i++] = ch;
                } else {
                    /* use an hex escape for non printing, unnamed chars */
                    ch_buf[i++] = 'x';
                    int res = snprintf(ch_buf+i, sizeof(ch_buf) - i, 
                                       "%x", ch);
                    if (res < 0) {
                        /* shouldn't happen, but for the sake of
                           completeness... */
                        TValue port = K->curr_port;
                        if (ttisfport(port)) {
                            FILE *file = kfport_file(port);
                            clearerr(file); /* clear error for next time */
                        }
                        kwrite_error(K, "error writing");
                        return;
                    } 
                    i += res; /* res doesn't include the '\0' */
                }
                ch_buf[i++] = '\0';
                ch_ptr = ch_buf;
            }
            }
            kw_printf(K, "#\\%s", ch_ptr);
        }
        break;
    }
    case K_TBOOLEAN:
        kw_printf(K, "#%c", bvalue(obj)? 't' : 'f');
        break;
    case K_TSYMBOL:
        kw_print_symbol(K, obj);
        break;
    case K_TKEYWORD:
        kw_print_keyword(K, obj);
        break;
    case K_TINERT:
        kw_printf(K, "#inert");
        break;
    case K_TIGNORE:
        kw_printf(K, "#ignore");
        break;
/* unreadable objects */
    case K_TUSER:
        kw_printf(K, "#[user pointer: %p]", pvalue(obj));
        break;
    case K_TEOF:
        kw_printf(K, "#[eof]");
        break;
    case K_TENVIRONMENT:
        kw_printf(K, "#[environment");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TCONTINUATION:
        kw_printf(K, "#[continuation");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif

        kw_print_cont_type(K, obj);

#if KTRACK_SI
        if (khas_si(obj))
            kw_print_si(K, obj);
#endif
        kw_printf(K, "]");
        break;
    case K_TOPERATIVE:
        kw_printf(K, "#[operative");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
#if KTRACK_SI
        if (khas_si(obj))
            kw_print_si(K, obj);
#endif
        kw_printf(K, "]");
        break;
    case K_TAPPLICATIVE:
        kw_printf(K, "#[applicative");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
#if KTRACK_SI
        if (khas_si(obj))
            kw_print_si(K, obj);
#endif
        kw_printf(K, "]");
        break;
    case K_TENCAPSULATION:
        /* TODO try to get the name */
        kw_printf(K, "#[encapsulation]");
        break;
    case K_TPROMISE:
        /* TODO try to get the name */
        kw_printf(K, "#[promise]");
        break;
    case K_TFPORT:
        /* TODO try to get the filename */
        kw_printf(K, "#[%s %s file port", 
                  kport_is_binary(obj)? "binary" : "textual",
                  kport_is_input(obj)? "input" : "output");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TMPORT:
        kw_printf(K, "#[%s %s port", 
                  kport_is_binary(obj)? "bytevector" : "string",
                  kport_is_input(obj)? "input" : "output");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TERROR: {
        kw_printf(K, "#[error: ");

        /* TEMP for now show only msg */
        bool saved_displayp = K->write_displayp; 
        K->write_displayp = false; /* use "'s and escapes */
        kw_print_string(K, tv2error(obj)->msg);
        K->write_displayp = saved_displayp;

        kw_printf(K, "]");
        break;
    }
    case K_TBYTEVECTOR:
        kw_printf(K, "#[bytevector");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TVECTOR:
        kw_printf(K, "#[vector");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TTABLE:
        kw_printf(K, "#[hash-table");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TLIBRARY:
        kw_printf(K, "#[library");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    case K_TTHREAD:
        kw_printf(K, "#[thread");
#if KTRACK_NAMES
        if (khas_name(obj)) {
            kw_print_name(K, obj);
        }
#endif
        kw_printf(K, "]");
        break;
    default:
        /* shouldn't happen */
        kwrite_error(K, "unknown object type");
        /* avoid warning */
        return;
    }
}


/* GC: obj is rooted */
void kwrite_fsm(klisp_State *K, TValue obj)
{
    /* NOTE: a fixint is more than enough for output */
    int32_t kw_shared_count = 0;

    assert(ks_sisempty(K));
    push_data(K, obj);

    bool middle_list = false;
    while (!data_is_empty(K)) {
        TValue obj = get_data(K);
        pop_data(K);

        if (middle_list) {
            if (ttisnil(obj)) { /* end of list */
                kw_printf(K, ")");
                /* middle_list = true; */
            } else if (ttispair(obj) && ttisboolean(kget_mark(obj))) {
                push_data(K, kcdr(obj));
                push_data(K, kcar(obj));
                kw_printf(K, " ");
                middle_list = false;
            } else { /* improper list is the same as shared ref */
                kw_printf(K, " . ");
                push_data(K, KNIL);
                push_data(K, obj);
                middle_list = false;
            }
        } else { /* if (middle_list) */
            switch(ttype(obj)) {
            case K_TPAIR: {
                TValue mark = kget_mark(obj);
                if (ttisboolean(mark)) { /* simple pair (only once) */
                    kw_printf(K, "(");
                    push_data(K, kcdr(obj));
                    push_data(K, kcar(obj));
                    middle_list = false;
                } else if (ivalue(mark) < 0) { /* pair with no assigned # */
                    /* TEMP: for now only fixints in shared refs */
                    assert(kw_shared_count >= 0);

                    kset_mark(obj, i2tv(kw_shared_count));
                    kw_printf(K, "#%" PRId32 "=(", kw_shared_count);
                    kw_shared_count++;
                    push_data(K, kcdr(obj));
                    push_data(K, kcar(obj));
                    middle_list = false;
                } else { /* pair with an assigned number */
                    kw_printf(K, "#%" PRId32 "#", ivalue(mark));
                    middle_list = true;
                }
                break;
            }
            case K_TSTRING: {
                if (kstring_emptyp(obj)) {
                    if (!K->write_displayp)
                        kw_printf(K, "\"\"");
                } else {
                    TValue mark = kget_mark(obj);
                    if (K->write_displayp || ttisboolean(mark)) { 
                        /* simple string (only once) or in display
                           (show all strings) */
                        kw_print_string(K, obj);
                    } else if (ivalue(mark) < 0) { /* string with no assigned # */
                        /* TEMP: for now only fixints in shared refs */
                        assert(kw_shared_count >= 0);
                        kset_mark(obj, i2tv(kw_shared_count));
                        kw_printf(K, "#%" PRId32 "=", kw_shared_count);
                        kw_shared_count++;
                        kw_print_string(K, obj);
                    } else { /* string with an assigned number */
                        kw_printf(K, "#%" PRId32 "#", ivalue(mark));
                    }
                }
                middle_list = true;
                break;
            }
            default:
                kwrite_scalar(K, obj);
                middle_list = true;
            }
        }
    }

    assert(ks_sisempty(K));
}

/*
** Writer Main function
*/
void kwrite(klisp_State *K, TValue obj)
{
    /* GC: root obj */
    krooted_tvs_push(K, obj);

    kw_set_initial_marks(K, obj);
    kwrite_fsm(K, obj);
    kw_flush(K);
    kw_clear_marks(K, obj);

    krooted_tvs_pop(K);
}

/*
** This is the same as above but will not display
** shared tags (and will hang if there are cycles)
*/
void kwrite_simple(klisp_State *K, TValue obj)
{
    /* GC: root obj */
    krooted_tvs_push(K, obj);
    kwrite_fsm(K, obj);
    kw_flush(K);
    krooted_tvs_pop(K);
}

/*
** Writer Interface
*/
void kwrite_display_to_port(klisp_State *K, TValue port, TValue obj, 
                            bool displayp)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_output(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));

    K->curr_port = port;
    K->write_displayp = displayp;
    kwrite(K, obj);
}

void kwrite_simple_to_port(klisp_State *K, TValue port, TValue obj)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_output(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));

    K->curr_port = port;
    K->write_displayp = false;
    kwrite_simple(K, obj);
}

void kwrite_newline_to_port(klisp_State *K, TValue port)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_output(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));
    K->curr_port = port; /* this isn't needed but all other 
                            i/o functions set it */
    kwrite_char_to_port(K, port, ch2tv('\n'));
}

void kwrite_char_to_port(klisp_State *K, TValue port, TValue ch)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_output(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));
    K->curr_port = port; /* this isn't needed but all other 
                            i/o functions set it */

    if (ttisfport(port)) {
        FILE *file = kfport_file(port);
        klisp_unlock(K);
        int res = fputc(chvalue(ch), file);
        klisp_lock(K);

        if (res == EOF) {
            clearerr(file); /* clear error for next time */
            kwrite_error(K, "error writing char");
        }
    } else if (ttismport(port)) {
        if (kport_is_binary(port)) {
            /* bytebuffer port */
            if (kmport_off(port) >= kbytevector_size(kmport_buf(port))) {
                kmport_resize_buffer(K, port, kmport_off(port) + 1);
            }
            kbytevector_buf(kmport_buf(port))[kmport_off(port)] = chvalue(ch);
            ++kmport_off(port);
        } else {
            /* string port */
            if (kmport_off(port) >= kstring_size(kmport_buf(port))) {
                kmport_resize_buffer(K, port, kmport_off(port) + 1);
            }
            kstring_buf(kmport_buf(port))[kmport_off(port)] = chvalue(ch);
            ++kmport_off(port);
        }
    } else {
        kwrite_error(K, "unknown port type");
        return;
    }
}

void kwrite_u8_to_port(klisp_State *K, TValue port, TValue u8)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_output(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_binary(port));
    K->curr_port = port; /* this isn't needed but all other 
                            i/o functions set it */
    if (ttisfport(port)) {
        FILE *file = kfport_file(port);
        klisp_unlock(K);
        int res = fputc(ivalue(u8), file);
        klisp_lock(K);

        if (res == EOF) {
            clearerr(file); /* clear error for next time */
            kwrite_error(K, "error writing u8");
        }
    } else if (ttismport(port)) {
        if (kport_is_binary(port)) {
            /* bytebuffer port */
            if (kmport_off(port) >= kbytevector_size(kmport_buf(port))) {
                kmport_resize_buffer(K, port, kmport_off(port) + 1);
            }
            kbytevector_buf(kmport_buf(port))[kmport_off(port)] = 
                (uint8_t) ivalue(u8);
            ++kmport_off(port);
        } else {
            /* string port */
            if (kmport_off(port) >= kstring_size(kmport_buf(port))) {
                kmport_resize_buffer(K, port, kmport_off(port) + 1);
            }
            kstring_buf(kmport_buf(port))[kmport_off(port)] = 
                (char) ivalue(u8);
            ++kmport_off(port);
        }
    } else {
        kwrite_error(K, "unknown port type");
        return;
    }
}

void kwrite_flush_port(klisp_State *K, TValue port) 
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_output(port));
    klisp_assert(kport_is_open(port));
    K->curr_port = port; /* this isn't needed but all other 
                            i/o functions set it */
    if (ttisfport(port)) { /* only necessary for file ports */
        FILE *file = kfport_file(port);
        klisp_assert(file);
        klisp_unlock(K);
        int res = fflush(file);
        klisp_lock(K);
        if (res == EOF) {
            clearerr(file); /* clear error for next time */
            kwrite_error(K, "error writing");
        }
    }
}
