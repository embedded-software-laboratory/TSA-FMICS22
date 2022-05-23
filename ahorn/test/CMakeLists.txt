add_subdirectory("${PROJECT_SOURCE_DIR}/extern/googletest" "extern/googletest")

mark_as_advanced(
        BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
        gmock_build_tests gtest_build_samples gtest_build_tests
        gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

set_target_properties(gtest PROPERTIES FOLDER extern)
set_target_properties(gtest_main PROPERTIES FOLDER extern)
set_target_properties(gmock PROPERTIES FOLDER extern)
set_target_properties(gmock_main PROPERTIES FOLDER extern)

macro(package_add_test TESTNAME)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} gtest gmock gtest_main)
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(${TESTNAME}
            # set a working directory so your project root so that you can find test data via paths relative to the project root
            WORKING_DIRECTORY ${PROJECT_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
            )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

macro(package_add_test_with_libraries TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
    add_executable(${TESTNAME} ${FILES})
    target_link_libraries(${TESTNAME} gtest gmock gtest_main ${LIBRARIES})
    gtest_discover_tests(${TESTNAME}
            WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
            )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

# COMPILER
package_add_test_with_libraries(test_lib_compiler test_lib_compiler.cpp lib_compiler "")

# IR
set(libs lib_ir lib_cfg lib_compiler spdlog)
package_add_test_with_libraries(test_lib_ir test_lib_ir.cpp "${libs}" "")

# CFG
set(libs lib_cfg lib_compiler spdlog)
package_add_test_with_libraries(test_lib_cfg test_lib_cfg.cpp "${libs}" "")

# SA
set(libs lib_sa lib_pass lib_ir lib_compiler spdlog)
set(test_lib_sa_src_files
        lib_sa/test_lib_sa.cpp)
package_add_test_with_libraries(test_lib_sa "${test_lib_sa_src_files}" "${libs}" "")

# PASS
set(libs lib_pass lib_ir lib_compiler spdlog)
set(test_lib_pass_src_files
        lib_pass/test_lib_pass_ssa.cpp
        lib_pass/test_lib_pass_tac.cpp)
package_add_test_with_libraries(test_lib_pass "${test_lib_pass_src_files}" "${libs}" "")

# SE
set(libs lib_se lib_pass lib_ir lib_compiler spdlog)
set(test_lib_se_src_files
        lib_se/test_lib_se_ahorn.cpp
        lib_se/test_lib_se_arcade.cpp
        lib_se/test_lib_se_baseline.cpp
        lib_se/test_lib_se_cbmc.cpp
        lib_se/test_lib_se_oa.cpp
        lib_se/test_lib_se_shadow.cpp
        lib_se/test_lib_se_smart.cpp
        lib_se/test_lib_se_simulator.cpp)
package_add_test_with_libraries(test_lib_se "${test_lib_se_src_files}" "${libs}" "")