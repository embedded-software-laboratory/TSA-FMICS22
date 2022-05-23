file(GLOB_RECURSE ir_src_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/ir "*.cpp")
file(GLOB_RECURSE ir_header_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/ir" "*.h")

add_library(lib_ir
        ${ir_src_files}
        ${ir_header_files})

find_package(Z3 REQUIRED)
target_include_directories(lib_ir PUBLIC
        "${PROJECT_SOURCE_DIR}/include/ahorn/" ${Z3_CXX_INCLUDE_DIRS})

target_link_libraries(lib_ir ${Boost_LIBRARIES} ${Z3_LIBRARIES} gtest spdlog)

target_compile_features(lib_ir PUBLIC cxx_std_14)

source_group(TREE "${PROJECT_SOURCE_DIR}/include/ahorn/ir" PREFIX "Header Files" FILES ${ir_header_files})