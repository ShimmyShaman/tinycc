
#include "tinycc/tcc.h"

#define MCtest(function)                                                              \
  {                                                                                   \
    int mc_res = function;                                                            \
    if (mc_res) {                                                                     \
      printf("\n\n[TestFailure]\n--" #function "line:%i:ERR:%i\n", __LINE__, mc_res); \
      exit(mc_res);                                                                   \
    }                                                                                 \
  }

void _test_int_func_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "int _getnb(void) {\n"
               "  return 7;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int a = _getnb();\n"
               "  return a - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(7));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "int _getnb(void) {\n"
               "  return 414;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(414));
}

void _test_int_func_replace_mem_alloc(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "#include <stdlib.h>\n"
               "\n"
               "int _getnb(void) {\n"
               "  return 7;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int a = _getnb();\n"
               "  int *m = (int *)malloc(sizeof(int) * 64256654);\n"
               "  m[424422] = a;\n"
               "  return m[424422] - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(7));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "int _getnb(void) {\n"
               "  return 414;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(414));
}

void _test_struct_func_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "struct pepe {\n"
               "  int alpha, beta;"
               "};\n"
               "\n"
               "struct pepe _getpepe(void) {\n"
               "  return (struct pepe){ 3, 8 };\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  struct pepe pp = _getpepe();\n"
               "  return pp.alpha * pp.beta - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(24));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "struct pepe {\n"
               "  int alpha, beta;"
               "};\n"
               "\n"
               "struct pepe _getpepe(void) {\n"
               "  return (struct pepe){ 14, 7 };\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(98));
}

void _test_func_with_args_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "int _getadd(int a, int b) {\n"
               "  return a + a * b;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int res = _getadd(3, 8);\n"
               "  return res - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(27));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "int _getadd(int a, int b) {\n"
               "  return b + a * b;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(32));
}

void _test_variadic_func_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "#include <stdarg.h>\n"
               "\n"
               "int _getcumulative(int n, ...) {\n"
               "  va_list valist;\n"
               "  va_start(valist, n);\n"
               "\n"
               "  int ret = 0;\n"
               "  for(int i = 0; i < n; ++i) {\n"
               "    ret += va_arg(valist, int);\n"
               "  }\n"
               "  va_end(valist);\n"
               "\n"
               "  return ret;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int res = _getcumulative(2, 8, 13);\n"
               "  return res - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "_test_variadic_func_replace.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(21));

  // -- change the function to print to 8
  sprintf(buf, "#include <stdarg.h>\n"
               "\n"
               "int _getcumulative(int n, ...) {\n"
               "  va_list valist;\n"
               "  va_start(valist, n);\n"
               "\n"
               "  int ret = 0;\n"
               "  for(int i = 0; i < n; ++i) {\n"
               "    ret += 2 + va_arg(valist, int);\n"
               "  }\n"
               "  va_end(valist);\n"
               "\n"
               "  return ret;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(25));
}

void _test_func_ptr_replace(TCCInterpState *itp)
{
  // void *(*mc_routine)(void *) = (void *(*)(void *))state_args[0];
  //   thread_res = mc_routine(wrapped_state);

  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "int _convert(void) {\n"
               "  return 3;\n"
               "}\n"
               "\n"
               "void *_getfptr(void) {\n"
               "  return (void *)&_convert;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int (*convnb)(void) = (int (*)(void))_getfptr();\n"
               "  int res = convnb();\n"
               "  return res - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(3));

  // -- change the function
  sprintf(buf, "\n"
               "int _convert(void) {\n"
               "  return 7;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(7));

  // -- change the function again
  sprintf(buf, "\n"
               "int _convert(void) {\n"
               "  return 113;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(113));
}

void _test_func_ptr_with_args_replace(TCCInterpState *itp)
{
  // void *(*mc_routine)(void *) = (void *(*)(void *))state_args[0];
  //   thread_res = mc_routine(wrapped_state);

  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "int _transform(int a, int b) {\n"
               "  return 3 + a + b;\n"
               "}\n"
               "\n"
               "void *_getfptr(void) {\n"
               "  return (void *)&_transform;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int (*convnb)(int, int) = (int (*)(int, int))_getfptr();\n"
               "  int res = convnb(4, 3);\n"
               "  return res - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(10));

  // -- change the function
  sprintf(buf, "\n"
               "int _transform(int a, int b) {\n"
               "  return 7 * a + b;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(31));

  // -- change the function again
  sprintf(buf, "\n"
               "int _transform(int a, int b) {\n"
               "  return 113 - a * b;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(101));
}

void _test_struct_func_ptr_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "struct pepe {\n"
               "  int alpha, beta;"
               "};\n"
               "\n"
               "struct pepe _getpepe(void) {\n"
               "  return (struct pepe){ 2, 8 };\n"
               "}\n"
               "\n"
               "void *_getfptr(void) {\n"
               "  return (void *)&_getpepe;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  struct pepe (*getpepe)(void) = (struct pepe (*)(void))_getfptr();\n"
               "  struct pepe pp = getpepe();\n"
               "  return pp.alpha * pp.beta - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(16));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "struct pepe {\n"
               "  int alpha, beta;"
               "};\n"
               "\n"
               "struct pepe _getpepe(void) {\n"
               "  return (struct pepe){ -4, 9 };\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(-36));
}

void _test_variadic_func_ptr_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "#include <stdarg.h>\n"
               "\n"
               "int _getcumulative(int n, ...) {\n"
               "  va_list valist;\n"
               "  va_start(valist, n);\n"
               "\n"
               "  int ret = 0;\n"
               "  for(int i = 0; i < n; ++i) {\n"
               "    ret += va_arg(valist, int);\n"
               "  }\n"
               "  va_end(valist);\n"
               "\n"
               "  return ret;\n"
               "}\n"
               "\n"
               "void *_getfptr(void) {\n"
               "  return (void *)&_getcumulative;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  int (*accumulate)(int, ...) = (int (*)(int, ...))_getfptr();\n"
               "  int res = accumulate(4, 7, 89, 44, -144);\n"
               "  return res - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "_test_variadic_func_replace.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(-4));

  // -- change the function to print to 8
  sprintf(buf, "#include <stdarg.h>\n"
               "\n"
               "int _getcumulative(int n, ...) {\n"
               "  va_list valist;\n"
               "  va_start(valist, n);\n"
               "\n"
               "  int ret = 0;\n"
               "  for(int i = 0; i < n; ++i) {\n"
               "    ret += 2 + va_arg(valist, int);\n"
               "  }\n"
               "  va_end(valist);\n"
               "\n"
               "  return ret;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(4));
}

void _test_static_func_replace(TCCInterpState *itp)
{
  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "#include <stdlib.h>\n"
               "\n"
               "static int _getnb(void) {\n"
               "  return 7;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  return _getnb() - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));
  usleep(10000);

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(7));

  // -- change the function to print to 8
  sprintf(buf, "#include <stdio.h>\n"
               "#include <stdlib.h>\n"
               "\n"
               "static int _getnb(void) {\n"
               "  return 44;\n"
               "}\n"
               "\n"
               "int doit2(int expect) {\n"
               "  return _getnb() - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies2.c", buf));
  usleep(100000);

  // -- invoke the new address
  int (*doit2)(int) = (int (*)(int))tcci_get_symbol(itp, "doit2");
  MCtest(doit2(44));

  // -- invoke the old address again to ensure it hasn't changed
  MCtest(doit(7));
  usleep(100000);

  // change the old static _getnb
  sprintf(buf, "static int _getnb(void) {\n"
               "  return 18;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));
  usleep(100000);

  // -- invoke the changed func
  MCtest(doit(18));

  // -- invoke the newer func again to ensure it hasn't changed
  MCtest(doit2(44));
}

void _test_static_func_replace_from_files(TCCInterpState *itp)
{
  const char *file0 = "dep/tinycc/tests/itpf0.c";
  const char *file1 = "dep/tinycc/tests/itpf1.c";
  const char *files[2] = {file0, file1};
  tcci_add_files(itp, files, 2);

  // -- Test the functions
  int (*checkit0)(int) = (int (*)(int))tcci_get_symbol(itp, "checkit0");
  MCtest(checkit0(88));
  int (*checkit1)(int) = (int (*)(int))tcci_get_symbol(itp, "checkit1");
  MCtest(checkit1(242));

  // Change one static method
  char buf[2048];
  sprintf(buf, "static int obtippit(void) {\n"
               "  return 98;\n"
               "}\n");
  MCtest(tcci_add_string(itp, file1, buf));
  // usleep(10000);

  MCtest(checkit0(88));
  MCtest(checkit1(98));

  // And the other
  sprintf(buf, "static int obtippit(void) {\n"
               "  return -33;\n"
               "}\n");
  MCtest(tcci_add_string(itp, file0, buf));

  MCtest(checkit0(-33));
  MCtest(checkit1(98));
}

void _test_func_ptr_indirect_call(TCCInterpState *itp)
{
  // void *(*mc_routine)(void *) = (void *(*)(void *))state_args[0];
  //   thread_res = mc_routine(wrapped_state);

  // char buf[2048];
  // sprintf(buf, "#include <stdio.h>\n"
  //              "\n"
  //              "struct albert {\n"
  //              "  int (*hookup)(void);\n"
  //              "};\n"
  //              "\n"
  //              "int _gethookup(void) {\n"
  //              "  return 199;\n"
  //              "}\n"
  //              "\n"
  //              "int doit(int expect) {\n"
  //              "  struct albert bay;\n"
  //              "  bay.hookup = &_gethookup;\n"
  //              "  int res = bay.hookup();\n"
  //              "  return res - expect;\n"
  //              "}\n");
  // MCtest(tcci_add_string(itp, "testies.c", buf));

  // puts("#### Now Error One ###\n\n");

  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "struct albert {\n"
               "  int (*hookup)(void);\n"
               "};\n"
               "\n"
               "int _gethookup(void) {\n"
               "  return 199;\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  struct albert bay;\n"
               "  bay.hookup = &_gethookup;\n"
               "  struct albert *indir = &bay;\n"
               "  int res = indir->hookup();\n"
               "  return res - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(199));
}

int getnb44(void) { return 44; }

void _test_use_set_global_symbol(TCCInterpState *itp)
{
  tcci_set_global_symbol(itp, "_getxtnb", &getnb44);

  char buf[2048];
  sprintf(buf, "#include <stdio.h>\n"
               "\n"
               "extern int _getxtnb(void);\n"
               "\n"
               "int doit(int expect) {\n"
               "  int a = _getxtnb();\n"
               "  return a - expect;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "testies.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(44));
}

#define titp(tfunc)                 \
  if (!itp->debug_verbose)          \
    printf("test '%s'...", #tfunc); \
  tfunc(itp);                       \
  printf("SUCCESS\n");

void test_itp()
{
  char buf[2048];
  int a, b;

  // Initialize the interpreter
  TCCInterpState *itp = tcci_new();
  tcci_set_Werror(itp, 1);

  const char *va_list_fp = "dep/tinycc/lib/va_list.c";
  tcci_add_files(itp, &va_list_fp, 1);

  itp->debug_verbose = 0;
  titp(_test_int_func_replace);
  titp(_test_int_func_replace_mem_alloc);
  titp(_test_struct_func_replace);
  titp(_test_func_with_args_replace);
  titp(_test_variadic_func_replace);
  titp(_test_func_ptr_replace);
  titp(_test_func_ptr_with_args_replace);
  titp(_test_struct_func_ptr_replace);
  titp(_test_variadic_func_ptr_replace);
  // itp->debug_verbose = 1;
  titp(_test_static_func_replace);
  titp(_test_static_func_replace_from_files);
  titp(_test_use_set_global_symbol);
  titp(_test_func_ptr_indirect_call);

  itp->debug_verbose = 0;
  exit(0);
}