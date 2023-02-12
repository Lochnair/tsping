project('tsping', 'c', version: '0.2.2')

add_project_arguments('-D_GNU_SOURCE', language: 'c')

add_project_arguments('-D_TSPING_VERSION="' + meson.project_version() + '"', language: 'c')

deps = [dependency('threads')]

argp_test = '''
#include <stdlib.h>
#include <argp.h>

int
main (int argc, char **argv)
{
  argp_parse (0, argc, argv, 0, 0, 0);
  exit (0);
}
'''

cc = meson.get_compiler('c')
argp_result = cc.links(argp_test, name : 'argp test')

if not argp_result
    argp_dep = cc.find_library('argp', required: true)
    deps += argp_dep
endif

executable('tsping', 'main.c', dependencies: deps)
