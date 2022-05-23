file(GLOB_RECURSE pass_src_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/pass "*.cpp")
file(GLOB_RECURSE pass_header_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/ahorn/pass" "*.h")

add_library(lib_pass
        ${pass_src_files}
        ${pass_header_files})

target_include_directories(lib_pass PUBLIC
        "${PROJECT_SOURCE_DIR}/include/ahorn/")

target_compile_features(lib_pass PUBLIC cxx_std_17)

target_link_libraries(lib_pass lib_cfg spdlog gtest)

source_group(TREE "${PROJECT_SOURCE_DIR}/include/ahorn/pass" PREFIX "Header Files" FILES ${pass_header_files})