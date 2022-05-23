file(GLOB_RECURSE cfg_src_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/cfg "*.cpp")
file(GLOB_RECURSE cfg_header_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/cfg" "*.h")

add_library(lib_cfg
        ${cfg_src_files}
        ${cfg_header_files})

target_include_directories(lib_cfg PUBLIC
        "${PROJECT_SOURCE_DIR}/include/ahorn/")

target_compile_features(lib_cfg PUBLIC cxx_std_14)

target_link_libraries(lib_cfg lib_ir gtest)

source_group(TREE "${PROJECT_SOURCE_DIR}/include/ahorn/cfg" PREFIX "Header Files" FILES ${cfg_header_files})