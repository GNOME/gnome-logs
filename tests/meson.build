# Tests
tests_env = [
    'LANG=C',
    'TZ=UTC',
    'MALLOC_CHECK_=2',
    'MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256))',
    'G_SLICE=debug-blocks'
]
test_gl_util_sources = files(
    'test-gl-util.c',
    '../src/gl-util.c'
)

if logs_enable_tests
    selftest = executable(
        'test-gl-util',
        sources : test_gl_util_sources,
        include_directories : [top_inc, src_inc],
        dependencies : gl_deps
    )
    test('test-gl-util', selftest, env : tests_env)

    #TODO: run test_SCRIPT too

    # basictest target
    run_target('basictest',
        command : [
            'env', tests_env,
            join_paths(meson.source_root(), 'tests/basic.py')
        ]
    )
endif
