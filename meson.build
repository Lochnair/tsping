project('tsping', 'c')

argp_dep = dependency('argp', required: get_option('argp_standalone'))
thread_dep = dependency('threads')

deps = [argp_dep, thread_dep]

executable('tsping', 'main.c', dependencies: deps)
