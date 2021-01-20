
#include "tinycc/tcc.h"

#define MCtest(function)                                           \
  {                                                                \
    int mc_res = function;                                         \
    if (mc_res) {                                                  \
      printf("--" #function "line:%i:ERR:%i\n", __LINE__, mc_res); \
      exit(mc_res);                                                \
    }                                                              \
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
  tcci_add_string(itp, "printings.c", buf);

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(7));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "int _getnb(void) {\n"
               "  return 414;\n"
               "}\n");
  tcci_add_string(itp, "printings2.c", buf);

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
               "  int alpha;"
               "};\n"
               "\n"
               "struct pepe _getpepe(void) {\n"
               "  return (struct pepe){ 3 };\n"
               "}\n"
               "\n"
               "int doit(int expect) {\n"
               "  struct pepe pp = _getpepe();\n"
               "  return pp.alpha - expect;\n"
               "}\n");
  tcci_add_string(itp, "printings.c", buf);

  dbp("####### Invoking doit() #######");

  // -- invoke the address
  int (*doit)(int) = (int (*)(int))tcci_get_symbol(itp, "doit");
  MCtest(doit(3));

  // -- change the function to print to 8
  sprintf(buf, "\n"
               "struct pepe {\n"
               "  int alpha;"
               "};\n"
               "\n"
               "struct pepe _getpepe(void) {\n"
               "  return (struct pepe){ 14 };\n"
               "}\n");
  tcci_add_string(itp, "printings2.c", buf);

  // -- invoke the address again
  dbp("==================");
  MCtest(doit(14));
}

#define titp(tfunc)                   \
  if (!itp->debug_verbose)            \
    printf("test '%s'...\n", #tfunc); \
  tfunc(itp);

void test_itp()
{
  char buf[2048];
  int a, b;

  // Initialize the interpreter
  TCCInterpState *itp = tcci_new();
  tcci_set_Werror(itp, 1);

  itp->debug_verbose = 0;
  titp(_test_int_func_replace);
  titp(_test_struct_func_replace);

  // itp->debug_verbose = 1;

  itp->debug_verbose = 0;
  exit(0);
}