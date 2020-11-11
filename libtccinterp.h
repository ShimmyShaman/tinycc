#ifndef LIBTCCINTERP_H
#define LIBTCCINTERP_H

#define LIBTCCINTERPAPI

struct TCCInterpState;
typedef struct TCCInterpState TCCInterpState;

/* create a new TCC interpretation context */
LIBTCCINTERPAPI TCCInterpState *tcci_new(void);

/* free a TCC interpretation context */
LIBTCCINTERPAPI void tcci_delete(TCCInterpState *ds);

LIBTCCINTERPAPI int tcci_add_include_path(TCCInterpState *ds, const char *pathname);

/* compile & link a c-code file-like declaration */
LIBTCCINTERPAPI int tcci_add_string(TCCInterpState *ds, const char *filename, const char *str);

// /* set/change pre-process define identifier : Will only affect code that is defined & will
//      not override included header defines for files */
// LIBTCCINTERPAPI int tcci_set_pp_define(TCCInterpState *ds, const char *identifier, const char *value);

/* interpret and execute single use statement-level code */
LIBTCCINTERPAPI int tcci_execute_single_use_code(TCCInterpState *ds, const char *filename,
                                                 const char *comma_seperated_includes, const char *code);
/* compile a group of C-files then link */
LIBTCCINTERPAPI int tcci_add_files(TCCInterpState *ds, const char **files, unsigned nb_files);

LIBTCCINTERPAPI int tcci_relocate_into_memory(TCCInterpState *ds);

LIBTCCINTERPAPI void *tcci_get_symbol(TCCInterpState *ds, const char *symbol_name);
LIBTCCINTERPAPI void *tcci_add_symbol(TCCInterpState *ds, const char *symbol_name, void *addr);

#endif // LIBTCCINTERP_H