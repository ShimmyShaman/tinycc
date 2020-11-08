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
LIBTCCINTERPAPI int tcci_add_files(TCCInterpState *ds, const char **files, unsigned nb_files);

LIBTCCINTERPAPI int tcci_relocate_into_memory(TCCInterpState *ds);

LIBTCCINTERPAPI void *tcci_get_symbol(TCCInterpState *ds, const char *symbol_name);
LIBTCCINTERPAPI void *tcci_add_symbol(TCCInterpState *ds, const char *symbol_name, void *addr);

#endif // LIBTCCINTERP_H