antlr_target(IEC61131 "${PROJECT_SOURCE_DIR}/include/ahorn/compiler/grammar/IEC61131.g4" VISITOR)

file(GLOB_RECURSE compiler_src_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/compiler "*.cpp")
file(GLOB_RECURSE compiler_header_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/compiler" "*.h")

add_library(lib_compiler
        ${compiler_src_files}
        ${compiler_header_files}
        ${ANTLR_IEC61131_CXX_OUTPUTS}
        ${ANTLR_IEC61131_OUTPUTS})

target_include_directories(lib_compiler PUBLIC
        "${PROJECT_SOURCE_DIR}/include/ahorn/"
        # add antrl4cpp artifacts to project environment
        "${ANTLR4_INCLUDE_DIRS}"
        "${ANTLR_IEC61131_OUTPUT_DIR}")

target_compile_features(lib_compiler PUBLIC cxx_std_14)

target_link_libraries(lib_compiler lib_ir antlr4_static spdlog)

source_group(TREE "${PROJECT_SOURCE_DIR}/include/ahorn/compiler" PREFIX "Header Files" FILES ${compiler_header_files})