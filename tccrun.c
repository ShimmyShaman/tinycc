/*
 *  TCC - Tiny C Compiler - Support for -run switch
 *
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tcc.h"

/* only native compiler supports -run */
#ifdef TCC_IS_NATIVE

#ifdef CONFIG_TCC_BACKTRACE
typedef struct rt_context {
  /* --> tccelf.c:tcc_add_btstub wants those below in that order: */
  Stab_Sym *stab_sym, *stab_sym_end;
  char *stab_str;
  ElfW(Sym) * esym_start, *esym_end;
  char *elf_str;
  addr_t prog_base;
  void *bounds_start;
  struct rt_context *next;
  /* <-- */
  int num_callers;
  addr_t ip, fp, sp;
  void *top_func;
  jmp_buf jmp_buf;
  char do_jmp;
} rt_context;

static rt_context g_rtctxt;
static void set_exception_handler(void);
static int _rt_error(void *fp, void *ip, const char *fmt, va_list ap);
static void rt_exit(int code);
#endif /* CONFIG_TCC_BACKTRACE */

/* defined when included from lib/bt-exe.c */
#ifndef CONFIG_TCC_BACKTRACE_ONLY

#ifndef _WIN32
#include <sys/mman.h>
#endif

static void set_pages_executable(TCCState *s1, void *ptr, unsigned long length);
static int tcc_relocate_ex(TCCState *s1, void *ptr, addr_t ptr_diff);

#ifdef _WIN64
static void *win64_add_function_table(TCCState *s1);
static void win64_del_function_table(void *);
#endif

/* ------------------------------------------------------------- */
/* Do all relocations (needed before using tcc_get_symbol())
   Returns -1 on error. */
LIBTCCAPI int tcc_relocate(TCCState *s1, void *ptr)
{
  int size;
  addr_t ptr_diff = 0;

  if (TCC_RELOCATE_AUTO != ptr)
    return tcc_relocate_ex(s1, ptr, 0);

  size = tcc_relocate_ex(s1, NULL, 0);
  if (size < 0)
    return -1;
  printf("relocate size:%i nbruntimemem:%i\n", size, s1->nb_runtime_mem);
#ifdef HAVE_SELINUX
  {
    /* Using mmap instead of malloc */
    void *prx;
    char tmpfname[] = "/tmp/.tccrunXXXXXX";
    int fd = mkstemp(tmpfname);
    unlink(tmpfname);
    ftruncate(fd, size);

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    prx = mmap(NULL, size, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED || prx == MAP_FAILED)
      tcc_error("tccrun: could not map memory");
    dynarray_add(&s1->runtime_mem, &s1->nb_runtime_mem, (void *)(addr_t)size);
    dynarray_add(&s1->runtime_mem, &s1->nb_runtime_mem, prx);
    ptr_diff = (char *)prx - (char *)ptr;
    close(fd);
  }
#else
  ptr = tcc_malloc(size);
#endif
  tcc_relocate_ex(s1, ptr, ptr_diff); /* no more errors expected */
  dynarray_add(&s1->runtime_mem, &s1->nb_runtime_mem, ptr);
  return 0;
}

ST_FUNC void tcc_run_free(TCCState *s1)
{
  int i;

  for (i = 0; i < s1->nb_runtime_mem; ++i) {
#ifdef HAVE_SELINUX
    unsigned size = (unsigned)(addr_t)s1->runtime_mem[i++];
    munmap(s1->runtime_mem[i++], size);
    munmap(s1->runtime_mem[i], size);
#else
#ifdef _WIN64
    win64_del_function_table(*(void **)s1->runtime_mem[i]);
#endif
    tcc_free(s1->runtime_mem[i]);
#endif
  }
  tcc_free(s1->runtime_mem);
}

// static void run_cdtors(TCCState *s1, const char *start, const char *end,
//                        int argc, char **argv, char **envp)
// {
//     void **a = (void **)get_sym_addr(s1, start, 0, 0);
//     void **b = (void **)get_sym_addr(s1, end, 0, 0);
//     while (a != b)
//         ((void(*)(int, char **, char **))*a++)(argc, argv, envp);
// }

// /* launch the compiled program with the given arguments */
// LIBTCCAPI int tcc_run(TCCState *s1, int argc, char **argv)
// {
//     int (*prog_main)(int, char **, char **), ret;
// #ifdef CONFIG_TCC_BACKTRACE
//     rt_context *rc = &g_rtctxt;
// #endif
// # if defined(__APPLE__)
//     char **envp = NULL;
// #else
//     char **envp = environ;
// #endif

//     s1->runtime_main = s1->nostdlib ? "_start" : "main";
//     if ((s1->dflag & 16) && (addr_t)-1 == get_sym_addr(s1, s1->runtime_main, 0, 1))
//         return 0;
// #ifdef CONFIG_TCC_BACKTRACE
//     if (s1->do_debug)
//         tcc_add_symbol(s1, "exit", rt_exit);
// #endif
//     if (tcc_relocate(s1, TCC_RELOCATE_AUTO) < 0)
//         return -1;
//     prog_main = (void*)get_sym_addr(s1, s1->runtime_main, 1, 1);

// #ifdef CONFIG_TCC_BACKTRACE
//     memset(rc, 0, sizeof *rc);
//     if (s1->do_debug) {
//         void *p;
//         rc->stab_sym = (Stab_Sym *)stab_section->data;
//         rc->stab_sym_end = (Stab_Sym *)(stab_section->data + stab_section->data_offset);
//         rc->stab_str = (char *)stab_section->link->data;
//         rc->esym_start = (ElfW(Sym) *)(symtab_section->data);
//         rc->esym_end = (ElfW(Sym) *)(symtab_section->data + symtab_section->data_offset);
//         rc->elf_str = (char *)symtab_section->link->data;
// #if PTR_SIZE == 8
//         rc->prog_base = text_section->sh_addr & 0xffffffff00000000ULL;
// #endif
//         rc->top_func = tcc_get_symbol(s1, "main");
//         rc->num_callers = s1->rt_num_callers;
//         rc->do_jmp = 1;
//         if ((p = tcc_get_symbol(s1, "__rt_error")))
//             *(void**)p = _rt_error;
// #ifdef CONFIG_TCC_BCHECK
//         if (s1->do_bounds_check) {
//             if ((p = tcc_get_symbol(s1, "__bound_init")))
//                 ((void(*)(void*, int))p)(bounds_section->data, 1);
//         }
// #endif
//         set_exception_handler();
//     }
// #endif

//     errno = 0; /* clean errno value */
//     fflush(stdout);
//     fflush(stderr);
//     /* These aren't C symbols, so don't need leading underscore handling.  */
//     run_cdtors(s1, "__init_array_start", "__init_array_end", argc, argv, envp);
// #ifdef CONFIG_TCC_BACKTRACE
//     if (!rc->do_jmp || !(ret = setjmp(rc->jmp_buf)))
// #endif
//     {
//         ret = prog_main(argc, argv, envp);
//     }
//     run_cdtors(s1, "__fini_array_start", "__fini_array_end", 0, NULL, NULL);
//     if ((s1->dflag & 16) && ret)
//         fprintf(s1->ppfp, "[returns %d]\n", ret), fflush(s1->ppfp);
//     return ret;
// }

#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
/* To avoid that x86 processors would reload cached instructions
   each time when data is written in the near, we need to make
   sure that code and data do not share the same 64 byte unit */
#define RUN_SECTION_ALIGNMENT 63
#else
#define RUN_SECTION_ALIGNMENT 0
#endif

/* relocate code. Return -1 on error, required size if ptr is NULL,
   otherwise copy code into buffer passed by the caller */
static int tcc_relocate_ex(TCCState *s1, void *ptr, addr_t ptr_diff)
{
  Section *s;
  unsigned offset, length, align, max_align, i, k, f;
  addr_t mem, addr;

  if (NULL == ptr) {
    s1->nb_errors = 0;
#ifdef TCC_TARGET_PE
    pe_output_file(s1, NULL);
#else
    tcc_add_runtime(s1);     // TODO -- this gets called multiple times - probably not good, but doesn't break anything
    resolve_common_syms(s1); // TODO -- caused error .. didn't need it .. but should have it, same as line above

    build_got_entries(s1);
#endif
    if (s1->nb_errors)
      return -1;
  }

  offset = max_align = 0, mem = (addr_t)ptr;
#ifdef _WIN64
  offset += sizeof(void *); /* space for function_table pointer */
#endif
  for (k = 0; k < 2; ++k) {
    f = 0, addr = k ? mem : mem + ptr_diff;
    for (i = 1; i < s1->nb_sections; i++) {
      s = s1->sections[i];
      if (0 == (s->sh_flags & SHF_ALLOC))
        continue;
      if (k != !(s->sh_flags & SHF_EXECINSTR))
        continue;
      align = s->sh_addralign - 1;
      if (++f == 1 && align < RUN_SECTION_ALIGNMENT)
        align = RUN_SECTION_ALIGNMENT;
      if (max_align < align)
        max_align = align;
      offset += -(addr + offset) & align;
      // printf("rex0, s1->sections[%i]('%s')->data_offset:%lu %i\n", i, s1->sections[i]->name,
      //        s1->sections[i]->data_offset, offset);
      s->sh_addr = mem ? addr + offset : 0;
      offset += s->data_offset;
#if 0
            if (mem)
                printf("%-16s %p  len %04x  align %2d\n",
                    s->name, (void*)s->sh_addr, (unsigned)s->data_offset, align + 1);
#endif
    }
  }

  // printf("##'L.0' [before relocate_syms] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
  /* relocate symbols */
  relocate_syms(s1, s1->symtab, !(s1->nostdlib));
  if (s1->nb_errors)
    return -1;

  // printf("offset=%i max_align=%i\n", offset, max_align);
  if (0 == mem)
    return offset + max_align;

#ifdef TCC_TARGET_PE
  s1->pe_imagebase = mem;
#endif

  // printf("##'L.0' [before relocate_sections] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
  // puts("xxx");
  /* relocate each section */
  for (i = 1; i < s1->nb_sections; i++) {
    s = s1->sections[i];
    if (s->reloc) {
      // printf("relocate-section:%i '%s'\n", i, s->name);
      relocate_section(s1, s);
      // puts("zzz");
    }
  }
  // printf("##'L.0' [before relocate_plt] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
#if !defined(TCC_TARGET_PE) || defined(TCC_TARGET_MACHO)
  relocate_plt(s1);
#endif

  for (i = 1; i < s1->nb_sections; i++) {
    s = s1->sections[i];
    if (0 == (s->sh_flags & SHF_ALLOC))
      continue;
    length = s->data_offset;
    ptr = (void *)s->sh_addr;
    if (s->sh_flags & SHF_EXECINSTR)
      ptr = (char *)((addr_t)ptr - ptr_diff);
    if (NULL == s->data || s->sh_type == SHT_NOBITS)
      memset(ptr, 0, length);
    else
      memcpy(ptr, s->data, length);
    /* mark executable sections as executable in memory */
    // printf("preexec-section:%i '%s' (void *)s->sh_addr:%p\n", i, s->name, (void *)s->sh_addr);
    if (s->sh_flags & SHF_EXECINSTR) {
      // printf("exec-section:%i '%s'\n", i, s->name);
      set_pages_executable(s1, (char *)((addr_t)ptr + ptr_diff), length);
    }
    // for (int b = 0; b < length;) {
    //   printf("      ");
    //   for (int bi = 0; bi < 8 && b < length; ++bi, ++b) {
    //     printf(" %02x", *((u_char *)((addr_t)ptr + ptr_diff) + b));
    //   }
    //   puts("\n");
    // }
  }

  // {
  //   ElfW(Sym) * sym;
  //   for_each_elem(symtab_section, 1, sym, ElfW(Sym))
  //   {
  //     if (!strcmp((char *)symtab_section->link->data + sym->st_name, "alpha")) {
  //       printf("\nalpha3(%lu bytes):", sym->st_size);
  //       for (int b = 0; b < (int)sym->st_size; ++b) {
  //         printf("%x ", *(text_section->data + b));
  //       }
  //       puts("\n");
  //     }
  //   }
  // }

#ifdef _WIN64
  *(void **)mem = win64_add_function_table(s1);
#endif

  return 0;
}

#define INTERP_RUNTIME_MEMORY_BASE_ALLOCATION 1024

typedef struct TCCIRuntimeMemory {
  unsigned allocated, size;
  u_char *data;

  struct {
    unsigned capacity, count;
    unsigned *offsets;
  } available;
} TCCIRuntimeMemory;

typedef struct TCCIFile {
} TCCIFile;

typedef struct TCCIFuncRel {
  // offset in the function
  uint32_t func_offset;

  // offset in plt
  uint32_t plt_offset;

  // the symbol strtab-name index and type (r_info)
  uint32_t symbol_strindex;

} TCCIFuncRel;

// typedef struct TCCIFunction {
//   char *name;
//   u_char is_global_access;

//   // u_char *runtime_ptr;
//   unsigned runtime_size;

//   unsigned nb_relocs;
//   ElfW_Rel *relocs;

//   TCCIFunction **users;
//   unsigned nb_users;
// } TCCIFunction;

// static int tcci_allocate_runtime_memory(TCCInterpState *itp, unsigned required_size, u_char **ptr)
// {
//   // Obtain the executable block of memory
//   TCCIRuntimeMemory *rm;
//   int e = 0, ai = -1;
//   do {
//     // Obtain the next runtime memory block
//     if (e >= itp->nb_runtime_mem_blocks) {
//       rm = tcc_mallocz(sizeof(TCCIRuntimeMemory));
//       rm->allocated = INTERP_RUNTIME_MEMORY_BASE_ALLOCATION * (1 << itp->nb_runtime_mem_blocks);
//       if (rm->allocated < required_size) {
//         rm->allocated = required_size + INTERP_RUNTIME_MEMORY_BASE_ALLOCATION;
//       }
//       rm->data = tcc_malloc(rm->allocated);
//       set_pages_executable(itp->s1, (void *)rm->data, rm->allocated);

//       rm->available.count = 0;
//       rm->available.capacity = 32;
//       rm->available.offsets = (unsigned *)tcc_mallocz(sizeof(unsigned) * rm->available.capacity);

//       rm->available.offsets[rm->available.count++] = 0;
//       rm->available.offsets[rm->available.count++] = rm->allocated;

//       dynarray_add(&itp->runtime_mem_blocks, &itp->nb_runtime_mem_blocks, rm);
//     }
//     else {
//       rm = itp->runtime_mem_blocks[itp->nb_runtime_mem_blocks - 1];
//     }
//     ++e;

//     // Find a fragment inside the runtime memory block large enough to allocate
//     for (ai = 0; ai < rm->available.count; ai += 2) {
//       if (rm->available.offsets[ai + 1] - rm->available.offsets[ai] >= required_size)
//         break;
//     }
//   } while (ai < 0 || ai >= rm->available.count);

//   // Set the ptr
//   *ptr = (u_char *)rm->data + rm->available.offsets[ai];

//   // Adjust the available fragment offsets collection
//   if (rm->available.offsets[ai + 1] - rm->available.offsets[ai] > required_size) {
//     rm->available.offsets[ai] += required_size;
//     return 0;
//   }

//   // Order doesn't matter just copy the last two values into place
//   for (e = 0; e < 2; ++e)
//     rm->available.offsets[ai + e] = rm->available.offsets[rm->available.count - 1 + e];
//   rm->available.count -= 2;

//   return 0;
// }

// static int tcci_allocate_runtime_function(TCCInterpState *itp, TCCIFile *file, ElfW(Sym) * sym, TCCIFunction **out)
// {
//   Section *strtab = itp->s1->symtab->link;

//   TCCIFunction *f = (TCCIFunction *)tcc_mallocz(sizeof(TCCIFunction));
//   f->name = tcc_strdup((char *)strtab->data + sym->st_name);
//   f->is_global_access = ELF64_ST_BIND(sym->st_info);
//   f->runtime_size = sym->st_size;

//   // tcci_allocate_runtime_memory(itp, f->runtime_size, &f->runtime_ptr);

//   *out = f;
//   return 0;
// }

void tcci_set_interp_symbol(TCCInterpState *itp, const char *filename, const char *symbol_name, u_char binding,
                            u_char type, void *addr)
{
  long unsigned hash = hash_djb2(symbol_name);
  // printf("tcci_set_interp_symbol: '%s' bnd:%u type:%u '%s'\n", symbol_name, binding, type, filename);
  if (binding == STB_LOCAL) {
    hash *= hash_djb2(filename);
  }

  TCCISymbol *sym = hash_table_get_by_hash(hash, &itp->symbols);

  if (sym) {
    dba(printf(">>>>> replacing old symbol for '%s' from %p > %p\n", symbol_name, (void *)sym->addr, addr));
    // printf(">>>>> replacing old symbol for '%s' from %p > %p\n", symbol_name, (void *)sym->addr, addr);

    // void *prev_addr = hash_table_get_by_hash(hash, &itp->redir.hash_to_addr);
    hash_table_set_by_hash(hash, addr, &itp->redir.hash_to_addr);
    hash_table_set_by_hash((unsigned long)sym->addr, addr, &itp->redir.addr_to_addr);
  }
  else {
    sym = tcc_mallocz(sizeof(TCCISymbol));
    hash_table_set_by_hash(hash, sym, &itp->symbols);
    sym->name = tcc_strdup(symbol_name);
    sym->filename = tcc_strdup(filename ? filename : "<unknown-file>");

    dba(printf(">>>>> added new symbol for '%s':%lu @ %p\n", symbol_name, hash, addr));
    // printf(">>>>> added new symbol for '%s':%lu @ %p\n", symbol_name, hash, addr);

    hash_table_set_by_hash(hash, (void *)addr, &itp->redir.hash_to_addr);
  }

  // Set Properties
  sym->binding = binding;
  sym->type = type;
  sym->addr = addr;

  if (sym->nb_got_users) {
    printf("has %u users>\n", sym->nb_got_users);
    for (int b = 0; b < sym->nb_got_users; ++b) {
      printf("--%p %p  before:%p\n", sym->got_users[b], sym->got_users[b], *(void **)sym->got_users[b]);
      *(void **)sym->got_users[b] = (void *)addr;
      printf("--%p %p  after:%p\n", sym->got_users[b], sym->got_users[b], *(void **)sym->got_users[b]);
    }
  }
}

LIBTCCINTERPAPI int tcci_relocate_into_memory(TCCInterpState *itp)
{
  TCCState *s1 = itp->s1;
  Section *s;
  unsigned offset, length, align, max_align, i, k, f;
  addr_t mem, addr;

  s1->nb_errors = 0;
#ifdef TCC_TARGET_PE
  pe_output_file(s1, NULL);
#else
  tcc_add_runtime(s1);     // TODO -- this gets called multiple times - probably not good, but doesn't break anything
  resolve_common_syms(s1); // TODO -- caused error .. didn't need it .. but should have it, same as line above

  build_got_entries(s1);
#endif
  if (s1->nb_errors)
    return 1;

  offset = max_align = 0, mem = (addr_t)NULL;
#ifdef _WIN64
  offset += sizeof(void *); /* space for function_table pointer */
#endif
  for (k = 0; k < 2; ++k) {
    f = 0, addr = k ? mem : mem + 0 /*ptr_diff*/;
    for (i = 1; i < s1->nb_sections; i++) {
      s = s1->sections[i];
      if (0 == (s->sh_flags & SHF_ALLOC))
        continue;
      if (k != !(s->sh_flags & SHF_EXECINSTR))
        continue;
      align = s->sh_addralign - 1;
      if (++f == 1 && align < RUN_SECTION_ALIGNMENT)
        align = RUN_SECTION_ALIGNMENT;
      if (max_align < align)
        max_align = align;
      offset += -(addr + offset) & align;
      // printf("rex0, s1->sections[%i]('%s')->data_offset:%lu %i\n", i, s1->sections[i]->name,
      //        s1->sections[i]->data_offset, offset);
      s->sh_addr = mem ? addr + offset : 0;
      offset += s->data_offset;
#if 0
            if (mem)
                printf("%-16s %p  len %04x  align %2d\n",
                    s->name, (void*)s->sh_addr, (unsigned)s->data_offset, align + 1);
#endif
    }
  }

  // printf("##'L.0' [before relocate_syms] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
  /* relocate symbols */
  tcci_relocate_syms(itp, s1->symtab, !(s1->nostdlib), 0);
  if (s1->nb_errors)
    return 2;

  // printf("offset=%i max_align=%i\n", offset, max_align);
  void *ptr = tcc_malloc(offset + max_align);
  if (itp->in_single_use_state) {
    itp->single_use.runtime_mem = ptr;
  }
  else {
    dynarray_add(&itp->runtime_mem_blocks, &itp->nb_runtime_mem_blocks, ptr);
    itp->runtime_mem_size += (uint64_t)offset + max_align;
  }
  // printf("ptr @%p allocated:%i\n", ptr, offset + max_align);

  offset = max_align = 0, mem = (addr_t)ptr;
#ifdef _WIN64
  offset += sizeof(void *); /* space for function_table pointer */
#endif
  for (k = 0; k < 2; ++k) {
    f = 0, addr = k ? mem : mem + 0 /*ptr_diff*/;
    for (i = 1; i < s1->nb_sections; i++) {
      s = s1->sections[i];
      if (0 == (s->sh_flags & SHF_ALLOC))
        continue;
      if (k != !(s->sh_flags & SHF_EXECINSTR))
        continue;
      align = s->sh_addralign - 1;
      if (++f == 1 && align < RUN_SECTION_ALIGNMENT)
        align = RUN_SECTION_ALIGNMENT;
      if (max_align < align)
        max_align = align;
      offset += -(addr + offset) & align;
      // printf("rex0, s1->sections[%i]('%s')->data_offset:%lu %i\n", i, s1->sections[i]->name,
      //        s1->sections[i]->data_offset, offset);
      s->sh_addr = mem ? addr + offset : 0;
      offset += s->data_offset;
#if 0
            if (mem)
                printf("%-16s %p  len %04x  align %2d\n",
                    s->name, (void*)s->sh_addr, (unsigned)s->data_offset, align + 1);
#endif
    }
  }

  // printf("##'L.0' [before relocate_syms] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
  /* relocate symbols */
  tcci_relocate_syms(itp, s1->symtab, !(s1->nostdlib), 1);
  if (s1->nb_errors)
    return 3;

#ifdef TCC_TARGET_PE
  s1->pe_imagebase = mem;
#endif

  // printf("##'L.0' [before relocate_sections] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
  // puts("xxx");
  /* relocate each section */
  for (i = 1; i < s1->nb_sections; i++) {
    s = s1->sections[i];
    if (s->reloc) {
      // printf("relocate-section:%i '%s'\n", i, s->name);
      relocate_section(s1, s);
      // puts("zzz");
    }
  }
  // printf("##'L.0' [before relocate_plt] st_value=%p\n",
  //        (void *)((ElfW(Sym) *)symtab_section->data)[ELFW(R_SYM)(((ElfW_Rel *)text_section->reloc->data)->r_info)]
  //            .st_value);
#if !defined(TCC_TARGET_PE) || defined(TCC_TARGET_MACHO)
  relocate_plt(s1);
#endif

  for (i = 1; i < s1->nb_sections; i++) {
    s = s1->sections[i];
    if (0 == (s->sh_flags & SHF_ALLOC))
      continue;
    length = s->data_offset;
    ptr = (void *)s->sh_addr;
    if (s->sh_flags & SHF_EXECINSTR)
      ptr = (char *)((addr_t)ptr - 0 /*ptr_diff*/);
    if (NULL == s->data || s->sh_type == SHT_NOBITS)
      memset(ptr, 0, length);
    else
      memcpy(ptr, s->data, length);
    /* mark executable sections as executable in memory */
    // printf("preexec-section:%i '%s' (void *)s->sh_addr:%p\n", i, s->name, (void *)s->sh_addr);
    if (s->sh_flags & SHF_EXECINSTR) {
      // printf("exec-section:%i '%s'\n", i, s->name);
      set_pages_executable(s1, (char *)((addr_t)ptr + 0 /*ptr_diff*/), length);
    }
    // for (int b = 0; b < length;) {
    //   printf("      ");
    //   for (int bi = 0; bi < 8 && b < length; ++bi, ++b) {
    //     printf(" %02x", *((u_char *)((addr_t)ptr + 0 /*ptr_diff*/) + b));
    //   }
    //   puts("\n");
    // }
  }

  // Copy Info to itp from s1
  ElfW(Sym) * sym;
  Section *symtab = symtab_section;
  u_char binding, type;
  for_each_elem(symtab, 1, sym, ElfW(Sym))
  {
    // if (!strcmp((char *)symtab->link->data + sym->st_name, "register_midge_error_tag"))
    //   printf("!> register_midge_error_tag was found at index %li\n", sym - (ElfW(Sym) *)symtab->data);
    // printf("sym->st_name:%s binding:%u st_shndx:%i st_other:%u\n", (char *)symtab->link->data + sym->st_name,
    //        ELF64_ST_BIND(sym->st_info), sym->st_shndx, sym->st_other);
    type = ELF64_ST_TYPE(sym->st_info);
    if (type != STT_FUNC || sym->st_shndx == 0)
      continue;

    if (s1->sections[sym->st_shndx] != text_section)
      continue;

    binding = ELF64_ST_BIND(sym->st_info);
    if (itp->in_single_use_state) {
      // printf("sus-sym->st_name:%s binding:%u st_shndx:%i st_other:%u\n", (char *)symtab->link->data + sym->st_name,
      //        ELF64_ST_BIND(sym->st_info), sym->st_shndx, sym->st_other);
      // if (itp->single_use.func_ptr ) {
      //   // Only one should be set
      //   return 7654;
      // }
      // printf("itp->single_use.func_ptr = (void *)sym->st_value(%p);\n", (void *)sym->st_value);
      itp->single_use.func_ptr = (void *)sym->st_value;
    }
    else if (ELF64_ST_VISIBILITY(sym->st_other) == STV_DEFAULT) {
      // printf("sym->st_name:%s binding:%u st_shndx:%i st_other:%u\n", (char *)symtab->link->data + sym->st_name,
      //        ELF64_ST_BIND(sym->st_info), sym->st_shndx, sym->st_other);

      // Get the filename
      const char *sym_fn =
          hash_table_get_by_hash((unsigned long)(sym - (ElfW(Sym) *)symtab->data), &itp->redir.sym_index_to_filename);
      // printf("index?:%li file='%s' %zu\n", sym - (ElfW(Sym) *)symtab->data, sym_fn, sym->st_size);
      if (!sym_fn) {
        printf("sym->st_name:%s binding:%u st_shndx:%i st_other:%u\n", (char *)symtab->link->data + sym->st_name,
               ELF64_ST_BIND(sym->st_info), sym->st_shndx, sym->st_other);
        printf("index?:%li file='%s' %zu\n", sym - (ElfW(Sym) *)symtab->data, sym_fn, sym->st_size);
        expect("sym_fn not NULL");
      }

      tcci_set_interp_symbol(itp, sym_fn, (char *)symtab->link->data + sym->st_name, binding, type,
                             (void *)sym->st_value);

      // if (binding != STB_GLOBAL)
      //   continue;
      // printf("<sym[%li]'%s'> \n", (int)((unsigned char *)sym - symtab->data) / sizeof(ElfW(Sym)),
      //        (char *)symtab->link->data + sym->st_name);
      // printf("binding:%u type:%u st_shndx:%u st_value:%p\n", binding, type, sym->st_shndx,
      //        (void *)sym->st_value /*s1->sections[sym->st_shndx]->name,  */);

      // tcci_set_global_symbol(itp, (char *)symtab->link->data + sym->st_name, binding, type, (void *)sym->st_value);
    }
  }

  return 0;
}
//   TCCState *s1 = itp->s1;
//   Section *s;
//   ElfW(Sym) * sym;
//   unsigned offset, block_size, length, align, max_align, i, e, k, f;
//   u_char *ptr;

//   const int TEXT_SECTION_INDEX = 1;

//   Section *symtab = s1->symtab;
//   Section *strtab = symtab->link;
//   int first_sym = symtab->sh_offset / sizeof(ElfSym);

//   // Create the file
//   // TODO -- with the first symbol of the symbol table for file name
//   TCCIFile *file_ref = NULL;

//   // Build global offset table
//   build_got_entries(s1);
//   itp->got.offset = 0U;
//   itp->got.allocated = 1024; // TODO
//   itp->got.ptr = tcc_malloc(itp->got.allocated);

//   // Relocate symbols
//   for_each_elem(symtab, 1, sym, ElfW(Sym))
//   {
//     if (sym->st_shndx == SHN_UNDEF) {
//       printf("-sym:'%s' SHN_UNDEF st_info:%u st_shndx:%i st_value:%lu\n", (char *)symtab->link->data +
//       sym->st_name,
//              sym->st_info, sym->st_shndx, sym->st_value);
//       char *sym_name = (char *)s1->symtab->link->data + sym->st_name;
//       /* Use ld.so to resolve symbol for us (for tcc -run) */
//       if (1) {
// #if defined TCC_IS_NATIVE && !defined TCC_TARGET_PE
// #ifdef TCC_TARGET_MACHO
//         /* The symbols in the symtables have a prepended '_'
//            but dlsym() needs the undecorated name.  */
//         void *addr = dlsym(RTLD_DEFAULT, name + 1);
// #else
//         void *addr = dlsym(RTLD_DEFAULT, sym_name);
// #endif
//         if (addr) {
//           sym->st_value = (addr_t)addr;
//           // #ifdef DEBUG_RELOC
//           printf("relocate_sym: '%s' -> 0x%lx\n", sym_name, sym->st_value);
//           // #endif
//           goto found;
//         }
// #endif
//         /* if dynamic symbol exist, it will be used in relocate_section */
//       }
//       else if (s1->dynsym && find_elf_sym(s1->dynsym, sym_name))
//         goto found;
//       /* XXX: _fp_hw seems to be part of the ABI, so we ignore
//          it */
//       if (!strcmp(sym_name, "_fp_hw"))
//         goto found;
//       /* only weak symbols are accepted to be undefined. Their
//          value is zero */
//       unsigned sym_bind = ELFW(ST_BIND)(sym->st_info);
//       if (sym_bind == STB_WEAK)
//         sym->st_value = 0;
//       else
//         tcc_error_noabort("undefined symbol '%s'", sym_name);
//     }
//     else if (sym->st_shndx < SHN_LORESERVE) {
//       // /* add section base */
//       printf("-sym:'%s' SHN_LORESERVE st_info:%u st_shndx:%i st_value:%lu\n", (char *)symtab->link->data +
//       sym->st_name,
//              sym->st_info, sym->st_shndx, sym->st_value);
//       printf("s1->sections[%i]('%s')->sh_addr:%lu\n", sym->st_shndx, s1->sections[sym->st_shndx]->name,
//              s1->sections[sym->st_shndx]->sh_addr);

//       // if (!strcmp((char *)symtab->link->data + sym->st_name, "alpha")) {
//       //   printf("\nalpha(%lu bytes):", sym->st_size);
//       //   for (int b = 0; b < (int)sym->st_size; ++b) {
//       //     printf("%x ", *(text_section->data + b));
//       //   }
//       //   puts("\n");
//       // }

//       sym->st_value += s1->sections[sym->st_shndx]->sh_addr;
//       // ++s1->nb_symtab_reloc_syms;
//     }
//   found:;
//   }

//   // Relocate symbols
//   for (i = 1; i < s1->nb_sections; i++) {
//     s = s1->sections[i];
//     if (s->reloc) {
//       printf(">reloc:%s\n", s->reloc->name);
//       Section *sr = s->reloc;
//       ElfW_Rel *rel;
//       int type, sym_index;
//       addr_t tgt, addr;

//       qrel = (ElfW_Rel *)sr->data;

//       for_each_elem(sr, 0, rel, ElfW_Rel)
//       {
//         ptr = s->data + rel->r_offset;
//         sym_index = ELFW(R_SYM)(rel->r_info);
//         sym = &((ElfW(Sym) *)symtab->data)[sym_index];
//         type = ELFW(R_TYPE)(rel->r_info);
//         tgt = sym->st_value;
// #if SHT_RELX == SHT_RELA
//         tgt += rel->r_addend;
// #endif
//         addr = s->sh_addr + rel->r_offset;
//         printf(">rela:%i-'%s' type:%i ptr:%p addr:%p sym->st_value:%lu(%p) tgt:%p\n", sym_index,
//                (char *)strtab->data + sym->st_name, type, ptr, (void *)addr, sym->st_value, (void *)sym->st_value,
//                (void *)tgt);

//         //         // printf("before:%p\n", *(void * *)ptr);
//         // relocate(s1, rel, type, ptr, addr, tgt);
//         //         // printf("after:%p\n", *(void * *)ptr);
//         //       }

//         //       /* if the relocation is allocated, we change its symbol table */
//         //       if (sr->sh_flags & SHF_ALLOC) {
//         //         sr->link = s1->dynsym;
//         //         if (s1->output_type == TCC_OUTPUT_DLL) {
//         //           size_t r = (uint8_t *)qrel - sr->data;
//         //           if (sizeof((Stab_Sym *)0)->n_value < PTR_SIZE && 0 == strcmp(s->name, ".stab"))
//         //             r = 0; /* cannot apply 64bit relocation to 32bit value */
//         //           sr->data_offset = sr->sh_size = r;
//         //         }
//       }
//     }
//   }

// #if !defined(TCC_TARGET_PE) || defined(TCC_TARGET_MACHO)
//   relocate_plt(s1);
//   itp->plt.offset = 0U;
//   itp->plt.allocated = 1024; // TODO
//   itp->plt.ptr = tcc_malloc(itp->plt.allocated);
// #endif
//   addr_t ptr_diff = 0; /* Selinux thing */

//   // Allocate runtime
//   int nb_syms = symtab->data_offset / sizeof(ElfSym) - first_sym;

//   int nb_func_copies = 0;
//   struct {
//     TCCIFunction *ifunc;
//     Elf64_Addr src_offset;
//   } pending_func_copies[nb_syms];

//   printf("End_Compile: nb_syms:%i, first_sym:%i\n", nb_syms, first_sym);
//   for (int i = 0; i < nb_syms; ++i) {
//     ElfSym *sym = (ElfSym *)symtab->data + first_sym + i;
//     printf("-sym:'%s' st_type:%u st_bind:%u st_shndx:%i st_value:%lu st_size:%lu st_other:%u\n",
//            (char *)strtab->data + sym->st_name, ELF64_ST_TYPE(sym->st_info), ELF64_ST_BIND(sym->st_info),
//            sym->st_shndx, sym->st_value, sym->st_size, sym->st_other);

//     if (ELF64_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_shndx == TEXT_SECTION_INDEX) {
//       // Allocate function entry to the interpreter state
//       tcci_allocate_runtime_function(itp, file_ref, sym, &pending_func_copies[nb_func_copies].ifunc);
//       pending_func_copies[nb_func_copies++].src_offset = sym->st_value;
//     }
//   }
//   for (i = 0; i < nb_func_copies; ++i) {
//     printf("%s: ", pending_func_copies[i].ifunc->name);
//     for (int b = 0; b < pending_func_copies[i].ifunc->runtime_size; ++b) {
//       printf("%x ", *(text_section->data + b));
//     }
//     puts("");
//   }

//   // Copy to runtime
//   TCCIFuncRel func_rels[1024];
//   int nb_func_rels;

//   for (i = 0; i < nb_func_copies; ++i) {
//     printf("\nCopying function '%s' from %p\n", pending_func_copies[i].ifunc->name,
//            text_section->data + pending_func_copies[i].src_offset);
//     memcpy(pending_func_copies[i].ifunc->runtime_ptr, text_section->data + pending_func_copies[i].src_offset,
//            pending_func_copies[i].ifunc->runtime_size);

//     // Append Relocations
//     nb_func_rels = 0;
//     {
//       // printf(">reloc:%s\n", text_section->reloc->name);
//       Section *sr = text_section->reloc;
//       ElfW_Rel *rel;
//       int type, sym_index;
//       addr_t tgt, addr;

//       qrel = (ElfW_Rel *)sr->data;

//       for_each_elem(sr, 0, rel, ElfW_Rel)
//       {
//         long long diff = (long long)rel->r_offset - pending_func_copies[i].src_offset;
//         if (diff < 0 || diff >= pending_func_copies[i].ifunc->runtime_size)
//           continue;

//         sym_index = ELFW(R_SYM)(rel->r_info);
//         sym = &((ElfW(Sym) *)symtab->data)[sym_index];
//         type = ELFW(R_TYPE)(rel->r_info);

//         TCCIFuncRel *fr = &func_rels[nb_func_rels++];
//         fr->func_offset = (uint32_t)diff;
//         fr->plt_offset = (uint32_t)((long long)sym->st_value - (long long)s1->plt->data);
//         fr->symbol_strindex = (uint32_t)sym->st_name;

//         printf("diff:%u(%p) sym->st_value:%p, s1->plt->data:%p\n", fr->plt_offset, (void *)fr->plt_offset, (void
//         *)sym->st_value, s1->plt->data); printf("funcrel func_offset:%u plt_offset:%u sym_name_index:%u ('%s')\n",
//         fr->func_offset, fr->plt_offset,
//                fr->symbol_strindex, (char *)strtab->data + fr->symbol_strindex);

//         // offset in the function
//         // offset in plt
//         // the symbol strtab-name index and type (r_info)

//         // if(rel->r_offset <  pending_func_copies[i].src_offset)

//         //         ptr = s->data + rel->r_offset;
//         //         sym_index = ELFW(R_SYM)(rel->r_info);
//         //         sym = &((ElfW(Sym) *)symtab->data)[sym_index];
//         //         type = ELFW(R_TYPE)(rel->r_info);
//         //         tgt = sym->st_value;
//         // #if SHT_RELX == SHT_RELA
//         //         tgt += rel->r_addend;
//         // #endif
//         //         addr = s->sh_addr + rel->r_offset;

//         //         printf(">rela:%i-'%s' type:%i ptr:%p addr:%p sym->st_value:%lu(%p) tgt:%p\n", sym_index,
//         //                (char *)strtab->data + sym->st_name, type, ptr, (void *)addr, sym->st_value, (void
//         //                *)sym->st_value, (void *)tgt);
//       }
//     }

//     // void *addr = dlsym(RTLD_DEFAULT, "puts");
//     // if (addr) {
//     //   // sym->st_value = (addr_t)addr;
//     //   // #ifdef DEBUG_RELOC
//     //   printf("puts: -> %p\n", addr);
//     //   // add32le(pending_func_copies[i].ifunc->runtime_ptr + 14, addr);
//     // }

//     // printf("%s: ", pending_func_copies[i].ifunc->name);
//     // for (int b = 0; b < pending_func_copies[i].ifunc->runtime_size; ++b) {
//     //   printf("%x ", *(text_section->data + b));
//     // }
//     // puts("");

//     // void (*ff)(void) = (void *)pending_func_copies[i].ifunc->runtime_ptr;
//     // puts("calling...");
//     // // add32le()
//     // ff();
//     // puts("end");
// }

/* ------------------------------------------------------------- */
/* allow to run code in memory */
static void set_pages_executable(TCCState *s1, void *ptr, unsigned long length)
{
  // {
  //   int a;
  //   unsigned int res = MPROT_0;
  //   FILE *f = fopen("/proc/self/maps", "r");
  //   struct buffer *b = _new_buffer(1024);
  //   while ((a = fgetc(f)) >= 0) {
  //     if (_buf_putchar(b, a) || a == '\n') {
  //       char *end0 = (void *)0;
  //       unsigned long addr0 = strtoul(b->mem, &end0, 0x10);
  //       char *end1 = (void *)0;
  //       unsigned long addr1 = strtoul(end0 + 1, &end1, 0x10);
  //       if ((void *)addr0 < addr && addr < (void *)addr1) {
  //         res |= (end1 + 1)[0] == 'r' ? MPROT_R : 0;
  //         res |= (end1 + 1)[1] == 'w' ? MPROT_W : 0;
  //         res |= (end1 + 1)[2] == 'x' ? MPROT_X : 0;
  //         res |= (end1 + 1)[3] == 'p' ? MPROT_P : (end1 + 1)[3] == 's' ? MPROT_S : 0;
  //         break;
  //       }
  //       _buf_reset(b);
  //     }
  //   }
  //   free(b);
  //   fclose(f);
  // }

#ifdef _WIN32
  unsigned long old_protect;
  VirtualProtect(ptr, length, PAGE_EXECUTE_READWRITE, &old_protect);
#else
  void __clear_cache(void *beginning, void *end);
#ifndef HAVE_SELINUX
  addr_t start, end;
#ifndef PAGESIZE
#define PAGESIZE 4096
#endif
  start = (addr_t)ptr & ~(PAGESIZE - 1);
  end = (addr_t)ptr + length;
  end = (end + PAGESIZE - 1) & ~(PAGESIZE - 1);
  if (mprotect((void *)start, end - start, PROT_READ | PROT_WRITE | PROT_EXEC))
    tcc_error("mprotect failed: did you mean to configure --with-selinux?");
#endif
#if defined TCC_TARGET_ARM || defined TCC_TARGET_ARM64
  __clear_cache(ptr, (char *)ptr + length);
#endif
#endif
}

//   static void remove_pages_executable(void *ptr, unsigned long length) {
//     // TODO -- I assume the default to return to is read-write. I have no idea

// #ifdef _WIN32
//     unsigned long old_protect;
//     VirtualProtect(ptr, length, PAGE_READWRITE, &old_protect);
// #else
//     void __clear_cache(void *beginning, void *end);
// #ifndef HAVE_SELINUX
//     addr_t start, end;
// #ifndef PAGESIZE
// #define PAGESIZE 4096
// #endif
//     start = (addr_t)ptr & ~(PAGESIZE - 1);
//     end = (addr_t)ptr + length;
//     end = (end + PAGESIZE - 1) & ~(PAGESIZE - 1);
//     if (mprotect((void *)start, end - start, PROT_READ | PROT_WRITE))
//       tcc_error("mprotect failed: did you mean to configure --with-selinux?");
// #endif
// #if defined TCC_TARGET_ARM || defined TCC_TARGET_ARM64
//     __clear_cache(ptr, (char *)ptr + length);
// #endif
// #endif
//   }

#ifdef _WIN64
// static void *win64_add_function_table(TCCState *s1)
// {
//     void *p = NULL;
//     if (s1->uw_pdata) {
//         p = (void*)s1->uw_pdata->sh_addr;
//         RtlAddFunctionTable(
//             (RUNTIME_FUNCTION*)p,
//             s1->uw_pdata->data_offset / sizeof (RUNTIME_FUNCTION),
//             s1->pe_imagebase
//             );
//         s1->uw_pdata = NULL;
//     }
//     return p;
// }

// static void win64_del_function_table(void *p)
// {
//     if (p) {
//         RtlDeleteFunctionTable((RUNTIME_FUNCTION*)p);
//     }
// }
#endif
#endif // ndef CONFIG_TCC_BACKTRACE_ONLY
/* ------------------------------------------------------------- */
#ifdef CONFIG_TCC_BACKTRACE

// static int rt_vprintf(const char *fmt, va_list ap)
// {
//     int ret = vfprintf(stderr, fmt, ap);
//     fflush(stderr);
//     return ret;
// }

// static int rt_printf(const char *fmt, ...)
// {
//     va_list ap;
//     int r;
//     va_start(ap, fmt);
//     r = rt_vprintf(fmt, ap);
//     va_end(ap);
//     return r;
// }

#define INCLUDE_STACK_SIZE 32

// /* print the position in the source file of PC value 'pc' by reading
//    the stabs debug information */
// static addr_t rt_printline (rt_context *rc, addr_t wanted_pc,
//     const char *msg, const char *skip)
// {
//     char func_name[128];
//     addr_t func_addr, last_pc, pc;
//     const char *incl_files[INCLUDE_STACK_SIZE];
//     int incl_index, last_incl_index, len, last_line_num, i;
//     const char *str, *p;
//     ElfW(Sym) *esym;
//     Stab_Sym *sym;

// next:
//     func_name[0] = '\0';
//     func_addr = 0;
//     incl_index = 0;
//     last_pc = (addr_t)-1;
//     last_line_num = 1;
//     last_incl_index = 0;

//     for (sym = rc->stab_sym + 1; sym < rc->stab_sym_end; ++sym) {
//         str = rc->stab_str + sym->n_strx;
//         pc = sym->n_value;

//         switch(sym->n_type) {
//         case N_SLINE:
//             if (func_addr)
//                 goto rel_pc;
//         case N_SO:
//         case N_SOL:
//             goto abs_pc;
//         case N_FUN:
//             if (sym->n_strx == 0) /* end of function */
//                 goto rel_pc;
//         abs_pc:
// #if PTR_SIZE == 8
//             /* Stab_Sym.n_value is only 32bits */
//             pc += rc->prog_base;
// #endif
//             goto check_pc;
//         rel_pc:
//             pc += func_addr;
//         check_pc:
//             if (pc >= wanted_pc && wanted_pc >= last_pc)
//                 goto found;
//             break;
//         }

//         switch(sym->n_type) {
//             /* function start or end */
//         case N_FUN:
//             if (sym->n_strx == 0)
//                 goto reset_func;
//             p = strchr(str, ':');
//             if (0 == p || (len = p - str + 1, len > sizeof func_name))
//                 len = sizeof func_name;
//             pstrcpy(func_name, len, str);
//             func_addr = pc;
//             break;
//             /* line number info */
//         case N_SLINE:
//             last_pc = pc;
//             last_line_num = sym->n_desc;
//             last_incl_index = incl_index;
//             break;
//             /* include files */
//         case N_BINCL:
//             if (incl_index < INCLUDE_STACK_SIZE)
//                 incl_files[incl_index++] = str;
//             break;
//         case N_EINCL:
//             if (incl_index > 1)
//                 incl_index--;
//             break;
//             /* start/end of translation unit */
//         case N_SO:
//             incl_index = 0;
//             if (sym->n_strx) {
//                 /* do not add path */
//                 len = strlen(str);
//                 if (len > 0 && str[len - 1] != '/')
//                     incl_files[incl_index++] = str;
//             }
//         reset_func:
//             func_name[0] = '\0';
//             func_addr = 0;
//             last_pc = (addr_t)-1;
//             break;
//             /* alternative file name (from #line or #include directives) */
//         case N_SOL:
//             if (incl_index)
//                 incl_files[incl_index-1] = str;
//             break;
//         }
//     }

//     func_name[0] = '\0';
//     func_addr = 0;
//     last_incl_index = 0;

//     /* we try symtab symbols (no line number info) */
//     for (esym = rc->esym_start + 1; esym < rc->esym_end; ++esym) {
//         int type = ELFW(ST_TYPE)(esym->st_info);
//         if (type == STT_FUNC || type == STT_GNU_IFUNC) {
//             if (wanted_pc >= esym->st_value &&
//                 wanted_pc < esym->st_value + esym->st_size) {
//                 pstrcpy(func_name, sizeof(func_name),
//                     rc->elf_str + esym->st_name);
//                 func_addr = esym->st_value;
//                 goto found;
//             }
//         }
//     }

//     if ((rc = rc->next))
//         goto next;

// found:
//     i = last_incl_index;
//     if (i > 0) {
//         str = incl_files[--i];
//         if (skip[0] && strstr(str, skip))
//             return (addr_t)-1;
//         rt_printf("%s:%d: ", str, last_line_num);
//     } else
//         rt_printf("%08llx : ", (long long)wanted_pc);
//     rt_printf("%s %s", msg, func_name[0] ? func_name : "???");
// #if 0
//     if (--i >= 0) {
//         rt_printf(" (included from ");
//         for (;;) {
//             rt_printf("%s", incl_files[i]);
//             if (--i < 0)
//                 break;
//             rt_printf(", ");
//         }
//         rt_printf(")");
//     }
// #endif
//     return func_addr;
// }

// static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level);

// static int _rt_error(void *fp, void *ip, const char *fmt, va_list ap)
// {
//     rt_context *rc = &g_rtctxt;
//     addr_t pc = 0;
//     char skip[100];
//     int i, level, ret, n;
//     const char *a, *b, *msg;

//     if (fp) {
//         /* we're called from tcc_backtrace. */
//         rc->fp = (addr_t)fp;
//         rc->ip = (addr_t)ip;
//         msg = "";
//     } else {
//         /* we're called from signal/exception handler */
//         msg = "RUNTIME ERROR: ";
//     }

//     skip[0] = 0;
//     /* If fmt is like "^file.c^..." then skip calls from 'file.c' */
//     if (fmt[0] == '^' && (b = strchr(a = fmt + 1, fmt[0]))) {
//         memcpy(skip, a, b - a), skip[b - a] = 0;
//         fmt = b + 1;
//     }

//     n = rc->num_callers ? rc->num_callers : 6;
//     for (i = level = 0; level < n; i++) {
//         ret = rt_get_caller_pc(&pc, rc, i);
//         a = "%s";
//         if (ret != -1) {
//             pc = rt_printline(rc, pc, level ? "by" : "at", skip);
//             if (pc == (addr_t)-1)
//                 continue;
//             a = ": %s";
//         }
//         if (level == 0) {
//             rt_printf(a, msg);
//             rt_vprintf(fmt, ap);
//         } else if (ret == -1)
//             break;
//         rt_printf("\n");
//         if (ret == -1 || (pc == (addr_t)rc->top_func && pc))
//             break;
//         ++level;
//     }

//     rc->ip = rc->fp = 0;
//     return 0;
// }

// /* emit a run time error at position 'pc' */
// static int rt_error(const char *fmt, ...)
// {
//     va_list ap;
//     int ret;
//     va_start(ap, fmt);
//     ret = _rt_error(0, 0, fmt, ap);
//     va_end(ap);
//     return ret;
// }

// static void rt_exit(int code)
// {
//     rt_context *rc = &g_rtctxt;
//     if (rc->do_jmp)
//         longjmp(rc->jmp_buf, code ? code : 256);
//     exit(code);
// }

// /* ------------------------------------------------------------- */

#ifndef _WIN32
#include <signal.h>
#ifndef __OpenBSD__
#include <sys/ucontext.h>
#endif
#else
#define ucontext_t CONTEXT
#endif

// /* translate from ucontext_t* to internal rt_context * */
// static void rt_getcontext(ucontext_t *uc, rt_context *rc)
// {
// #if defined _WIN64
//     rc->ip = uc->Rip;
//     rc->fp = uc->Rbp;
//     rc->sp = uc->Rsp;
// #elif defined _WIN32
//     rc->ip = uc->Eip;
//     rc->fp = uc->Ebp;
//     rc->sp = uc->Esp;
// #elif defined __i386__
// # if defined(__APPLE__)
//     rc->ip = uc->uc_mcontext->__ss.__eip;
//     rc->fp = uc->uc_mcontext->__ss.__ebp;
// # elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
//     rc->ip = uc->uc_mcontext.mc_eip;
//     rc->fp = uc->uc_mcontext.mc_ebp;
// # elif defined(__dietlibc__)
//     rc->ip = uc->uc_mcontext.eip;
//     rc->fp = uc->uc_mcontext.ebp;
// # elif defined(__NetBSD__)
//     rc->ip = uc->uc_mcontext.__gregs[_REG_EIP];
//     rc->fp = uc->uc_mcontext.__gregs[_REG_EBP];
// # elif defined(__OpenBSD__)
//     rc->ip = uc->sc_eip;
//     rc->fp = uc->sc_ebp;
// # elif !defined REG_EIP && defined EIP /* fix for glibc 2.1 */
//     rc->ip = uc->uc_mcontext.gregs[EIP];
//     rc->fp = uc->uc_mcontext.gregs[EBP];
// # else
//     rc->ip = uc->uc_mcontext.gregs[REG_EIP];
//     rc->fp = uc->uc_mcontext.gregs[REG_EBP];
// # endif
// #elif defined(__x86_64__)
// # if defined(__APPLE__)
//     rc->ip = uc->uc_mcontext->__ss.__rip;
//     rc->fp = uc->uc_mcontext->__ss.__rbp;
// # elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
//     rc->ip = uc->uc_mcontext.mc_rip;
//     rc->fp = uc->uc_mcontext.mc_rbp;
// # elif defined(__NetBSD__)
//     rc->ip = uc->uc_mcontext.__gregs[_REG_RIP];
//     rc->fp = uc->uc_mcontext.__gregs[_REG_RBP];
// # else
//     rc->ip = uc->uc_mcontext.gregs[REG_RIP];
//     rc->fp = uc->uc_mcontext.gregs[REG_RBP];
// # endif
// #elif defined(__arm__)
//     rc->ip = uc->uc_mcontext.arm_pc;
//     rc->fp = uc->uc_mcontext.arm_fp;
// #elif defined(__aarch64__)
//     rc->ip = uc->uc_mcontext.pc;
//     rc->fp = uc->uc_mcontext.regs[29];
// #elif defined(__riscv)
//     rc->ip = uc->uc_mcontext.__gregs[REG_PC];
//     rc->fp = uc->uc_mcontext.__gregs[REG_S0];
// #endif
// }

/* ------------------------------------------------------------- */
#ifndef _WIN32
// /* signal handler for fatal errors */
// static void sig_error(int signum, siginfo_t *siginf, void *puc)
// {
//     rt_context *rc = &g_rtctxt;
//     rt_getcontext(puc, rc);

//     switch(signum) {
//     case SIGFPE:
//         switch(siginf->si_code) {
//         case FPE_INTDIV:
//         case FPE_FLTDIV:
//             rt_error("division by zero");
//             break;
//         default:
//             rt_error("floating point exception");
//             break;
//         }
//         break;
//     case SIGBUS:
//     case SIGSEGV:
//         rt_error("invalid memory access");
//         break;
//     case SIGILL:
//         rt_error("illegal instruction");
//         break;
//     case SIGABRT:
//         rt_error("abort() called");
//         break;
//     default:
//         rt_error("caught signal %d", signum);
//         break;
//     }
//     rt_exit(255);
// }

#ifndef SA_SIGINFO
#define SA_SIGINFO 0x00000004u
#endif

// /* Generate a stack backtrace when a CPU exception occurs. */
// static void set_exception_handler(void)
// {
//     struct sigaction sigact;
//     /* install TCC signal handlers to print debug info on fatal
//        runtime errors */
//     sigact.sa_flags = SA_SIGINFO | SA_RESETHAND;
// #if 0//def SIGSTKSZ // this causes signals not to work at all on some (older) linuxes
//     sigact.sa_flags |= SA_ONSTACK;
// #endif
//     sigact.sa_sigaction = sig_error;
//     sigemptyset(&sigact.sa_mask);
//     sigaction(SIGFPE, &sigact, NULL);
//     sigaction(SIGILL, &sigact, NULL);
//     sigaction(SIGSEGV, &sigact, NULL);
//     sigaction(SIGBUS, &sigact, NULL);
//     sigaction(SIGABRT, &sigact, NULL);
// #if 0//def SIGSTKSZ
//     /* This allows stack overflow to be reported instead of a SEGV */
//     {
//         stack_t ss;
//         static unsigned char stack[SIGSTKSZ] __attribute__((aligned(16)));

//         ss.ss_sp = stack;
//         ss.ss_size = SIGSTKSZ;
//         ss.ss_flags = 0;
//         sigaltstack(&ss, NULL);
//     }
// #endif
// }

#else /* WIN32 */

// /* signal handler for fatal errors */
// static long __stdcall cpu_exception_handler(EXCEPTION_POINTERS *ex_info)
// {
//     rt_context *rc = &g_rtctxt;
//     unsigned code;
//     rt_getcontext(ex_info->ContextRecord, rc);

//     switch (code = ex_info->ExceptionRecord->ExceptionCode) {
//     case EXCEPTION_ACCESS_VIOLATION:
// 	rt_error("invalid memory access");
//         break;
//     case EXCEPTION_STACK_OVERFLOW:
//         rt_error("stack overflow");
//         break;
//     case EXCEPTION_INT_DIVIDE_BY_ZERO:
//         rt_error("division by zero");
//         break;
//     case EXCEPTION_BREAKPOINT:
//     case EXCEPTION_SINGLE_STEP:
//         rc->ip = *(addr_t*)rc->sp;
//         rt_error("breakpoint/single-step exception:");
//         return EXCEPTION_CONTINUE_SEARCH;
//     default:
//         rt_error("caught exception %08x", code);
//         break;
//     }
//     if (rc->do_jmp)
//         rt_exit(255);
//     return EXCEPTION_EXECUTE_HANDLER;
// }

// /* Generate a stack backtrace when a CPU exception occurs. */
// static void set_exception_handler(void)
// {
//     SetUnhandledExceptionFilter(cpu_exception_handler);
// }

#endif

// /* ------------------------------------------------------------- */
// /* return the PC at frame level 'level'. Return negative if not found */
#if defined(__i386__) || defined(__x86_64__)
// static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
// {
//     addr_t ip, fp;
//     if (level == 0) {
//         ip = rc->ip;
//     } else {
//         ip = 0;
//         fp = rc->fp;
//         while (--level) {
//             /* XXX: check address validity with program info */
//             if (fp <= 0x1000)
//                 break;
//             fp = ((addr_t *)fp)[0];
//         }
//         if (fp > 0x1000)
//             ip = ((addr_t *)fp)[1];
//     }
//     if (ip <= 0x1000)
//         return -1;
//     *paddr = ip;
//     return 0;
// }

#elif defined(__arm__)
// static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
// {
//     /* XXX: only supports linux */
// #if !defined(__linux__)
//     return -1;
// #else
//     if (level == 0) {
//         *paddr = rc->ip;
//     } else {
//         addr_t fp = rc->fp;
//         while (--level)
//             fp = ((addr_t *)fp)[0];
//         *paddr = ((addr_t *)fp)[2];
//     }
//     return 0;
// #endif
// }

#elif defined(__aarch64__)
// static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
// {
//     if (level == 0) {
//         *paddr = rc->ip;
//     } else {
//         addr_t *fp = (addr_t*)rc->fp;
//         while (--level)
//             fp = (addr_t *)fp[0];
//         *paddr = fp[1];
//     }
//     return 0;
// }

#elif defined(__riscv)
// static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
// {
//     if (level == 0) {
//         *paddr = rc->ip;
//     } else {
//         addr_t *fp = (addr_t*)rc->fp;
//         while (--level && fp >= (addr_t*)0x1000)
//             fp = (addr_t *)fp[-2];
//         if (fp < (addr_t*)0x1000)
//           return -1;
//         *paddr = fp[-1];
//     }
//     return 0;
// }

#else
#warning add arch specific rt_get_caller_pc()
// static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
// {
//     return -1;
// }

#endif
#endif /* CONFIG_TCC_BACKTRACE */
/* ------------------------------------------------------------- */
#ifdef CONFIG_TCC_STATIC

// /* dummy function for profiling */
// ST_FUNC void *dlopen(const char *filename, int flag)
// {
//     return NULL;
// }

// ST_FUNC void dlclose(void *p)
// {
// }

// ST_FUNC const char *dlerror(void)
// {
//     return "error";
// }

// typedef struct TCCSyms {
//     char *str;
//     void *ptr;
// } TCCSyms;

// /* add the symbol you want here if no dynamic linking is done */
// static TCCSyms tcc_syms[] = {
// #if !defined(CONFIG_TCCBOOT)
// #define TCCSYM(a) { #a, &a, },
//     TCCSYM(printf)
//     TCCSYM(fprintf)
//     TCCSYM(fopen)
//     TCCSYM(fclose)
// #undef TCCSYM
// #endif
//     { NULL, NULL },
// };

// ST_FUNC void *dlsym(void *handle, const char *symbol)
// {
//     TCCSyms *p;
//     p = tcc_syms;
//     while (p->str != NULL) {
//         if (!strcmp(p->str, symbol))
//             return p->ptr;
//         p++;
//     }
//     return NULL;
// }

#endif /* CONFIG_TCC_STATIC */
#endif /* TCC_IS_NATIVE */
/* ------------------------------------------------------------- */
