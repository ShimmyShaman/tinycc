
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
  MCtest(tcci_add_string(itp, "printings.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(7));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "int _getnb(void) {\n"
               "  return 414;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings.c", buf));

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
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(27));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "int _getadd(int a, int b) {\n"
               "  return b + a * b;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(3));

  // -- change the function
  sprintf(buf, "\n"
               "int _convert(void) {\n"
               "  return 7;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "printings2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(7));

  // -- change the function again
  sprintf(buf, "\n"
               "int _convert(void) {\n"
               "  return 113;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings.c", buf));

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(10));

  // -- change the function
  sprintf(buf, "\n"
               "int _transform(int a, int b) {\n"
               "  return 7 * a + b;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "printings2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(31));

  // -- change the function again
  sprintf(buf, "\n"
               "int _transform(int a, int b) {\n"
               "  return 113 - a * b;\n"
               "}\n");
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings.c", buf));

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
  MCtest(tcci_add_string(itp, "printings2.c", buf));

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
  MCtest(tcci_add_string(itp, "printings2.c", buf));

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(4));
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
  titp(_test_struct_func_replace);
  titp(_test_func_with_args_replace);
  titp(_test_variadic_func_replace);
  titp(_test_func_ptr_replace);
  titp(_test_func_ptr_with_args_replace);
  titp(_test_struct_func_ptr_replace);
  // itp->debug_verbose = 1;
  titp(_test_variadic_func_ptr_replace);

  itp->debug_verbose = 0;
  exit(0);
}