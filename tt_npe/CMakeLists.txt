# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: © 2025 Tenstorrent AI ULC

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")

#-----------------------------------------------------------
#------- src file globs ------------------------------------
#-----------------------------------------------------------
# cpp files for shared lib build
file(GLOB_RECURSE TT_NPE_SRC 
	"${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/*.cpp"
)
# cpp files for unit tests
file(GLOB_RECURSE TT_NPE_UT_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/cpp/test/*.cpp"
)
file(GLOB TT_NPE_PUBLIC_HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/*.hpp")
string(JOIN ";" TT_NPE_PUBLIC_HEADER_CONCAT ${TT_NPE_PUBLIC_HEADERS})

#-----------------------------------------------------------
#------- common link deps ----------------------------------
#-----------------------------------------------------------
add_library(npe_common_libs INTERFACE)
target_link_libraries(
    npe_common_libs
    INTERFACE
        fmt::fmt-header-only
        magic_enum
        nlohmann_json::nlohmann_json
        simdjson
        Boost::container
        Boost::unordered
        Boost::program_options
        libzstd_static 
)
#-----------------------------------------------------------
#------- library (libtt_npe.so) config ----------------------
#-----------------------------------------------------------
add_library(tt_npe SHARED ${TT_NPE_SRC})

target_link_libraries(tt_npe PUBLIC 
	npe_common_libs
)

target_include_directories(tt_npe PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE
    ${zstd_SOURCE_DIR}/lib # required bc libzstd doesn't export include dirs properly -_-*
)

set_target_properties(tt_npe PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${TT_NPE_PUBLIC_HEADER_CONCAT}"
)

#-----------------------------------------------------------
#------- Python Bindings -----------------------------------
#-----------------------------------------------------------
pybind11_add_module(tt_npe_pybind cpp/pybind/bindings.cpp)
target_link_libraries(tt_npe_pybind PRIVATE tt_npe)

#-----------------------------------------------------------
#------- unit test config ----------------------------------
#-----------------------------------------------------------
add_executable(tt_npe_ut ${test} ${TT_NPE_UT_SRC})
target_link_libraries(tt_npe_ut PRIVATE 
	npe_common_libs 
    test_common_libs
	tt_npe
)
target_include_directories(tt_npe_ut PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
    PRIVATE
    ${zstd_SOURCE_DIR}/lib # required bc libzstd doesn't export include dirs properly -_-*
)

#-----------------------------------------------------------
#------- Installation --------------------------------------
#-----------------------------------------------------------
install(TARGETS tt_npe tt_npe_pybind 
    RUNTIME 
        DESTINATION bin
    LIBRARY 
        DESTINATION lib
    PUBLIC_HEADER 
        DESTINATION include
)
install(
    FILES 
        py/pycli/tt_npe.py
        py/util/npe_analyze_noc_trace_dir.py
        py/examples/programmatic_workload_generation.py
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION bin
)
