project('tsping', 'c')

cc = meson.get_compiler('c')
argp_dep = cc.find_library('argp', required: get_option('argp_standalone'))
thread_dep = dependency('threads')

deps = [argp_dep, thread_dep]

executable('tsping', 'main.c', dependencies: deps)
