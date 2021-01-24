#ifndef LIBTCCINTERP_H
#define LIBTCCINTERP_H

#define LIBTCCINTERPAPI

struct TCCInterpState;
typedef struct TCCInterpState TCCInterpState;

/* create a new TCC interpretation context */
LIBTCCINTERPAPI TCCInterpState *tcci_new(void);

/* free a TCC interpretation context */
LIBTCCINTERPAPI void tcci_delete(TCCInterpState *ds);

LIBTCCINTERPAPI void tcci_set_Werror(TCCInterpState *ds, unsigned char value);

LIBTCCINTERPAPI int tcci_add_include_path(TCCInterpState *ds, const char *pathname);

LIBTCCINTERPAPI int tcci_add_library(TCCInterpState *ds, const char *libname);

/* compile & link a c-code file-like declaration */
LIBTCCINTERPAPI int tcci_add_string(TCCInterpState *ds, const char *filename, const char *str);

// /* set/change pre-process define identifier : Will only affect code that is defined & will
//      not override included header defines for files */
// LIBTCCINTERPAPI int tcci_set_pp_define(TCCInterpState *ds, const char *identifier, const char *value);

/*
 * interpret and execute single use statement-level code
 * @code must describe the code block of a function which can take as param: void *@vargs, and returns (void *) to
 * @result.
 * @result may be NULL.
 */
LIBTCCINTERPAPI int tcci_execute_single_use_code(TCCInterpState *ds, const char *filename,
                                                 const char *comma_seperated_includes, const char *code, void *vargs,
                                                 void **result);
/* compile a group of C-files then link */
LIBTCCINTERPAPI int tcci_add_files(TCCInterpState *ds, const char **files, unsigned nb_files);

LIBTCCINTERPAPI int tcci_relocate_into_memory(TCCInterpState *ds);

LIBTCCINTERPAPI void *tcci_get_symbol(TCCInterpState *ds, const char *symbol_name);

/* Sets a symbols address (append or update)
   NOTE: NOT YET PROPERLY IMPLEMENTED. It will also redirect internal interpreter calls if those calls
     were from seperate compilation units (ie. different calls to add_files,execute_single_use_code,add_string).
     Yes. Its quite icky. TODO */
LIBTCCINTERPAPI void tcci_set_global_symbol(TCCInterpState *ds, const char *symbol_name, void *addr);

/* TODO -- figure out naming and if I want to rid of tcci or just redirect with this
    -- I hope it doesn't have to wait till I figure out how to integrate tcc accumulative interpretation into tccs code
   base
 */
// LIBTCCAPI void tcc_define_symbol(TCCState *s1, const char *sym, const char *value)
LIBTCCINTERPAPI void tcci_define_symbol(TCCInterpState *ds, const char *sym, const char *value);
LIBTCCINTERPAPI void tcci_undefine_symbol(TCCInterpState *ds, const char *sym);
#endif // LIBTCCINTERP_H