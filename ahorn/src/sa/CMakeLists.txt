file(GLOB_RECURSE sa_src_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/sa "*.cpp")
file(GLOB_RECURSE sa_header_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/ahorn/sa" "*.h")

add_library(lib_sa
        ${sa_src_files}
        ${sa_header_files})

target_include_directories(lib_sa PUBLIC
        "${PROJECT_SOURCE_DIR}/include/ahorn/"
        ${CRAB_INCLUDE_DIRS})

target_compile_features(lib_sa PUBLIC cxx_std_17)

target_link_libraries(lib_sa lib_pass lib_cfg ${CRAB_LIBS} spdlog gtest)

source_group(TREE "${PROJECT_SOURCE_DIR}/include/ahorn/sa" PREFIX "Header Files" FILES ${sa_header_files})