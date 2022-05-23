file(GLOB_RECURSE se_src_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/se "*.cpp")
file(GLOB_RECURSE se_header_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/ahorn/se" "*.h")

add_library(lib_se
        ${se_src_files}
        ${se_header_files})

target_include_directories(lib_se PUBLIC
        "${PROJECT_SOURCE_DIR}/include/ahorn/")

target_compile_features(lib_se PUBLIC cxx_std_17)

target_link_libraries(lib_se lib_cfg spdlog gtest)

source_group(TREE "${PROJECT_SOURCE_DIR}/include/ahorn/se" PREFIX "Header Files" FILES ${se_header_files})