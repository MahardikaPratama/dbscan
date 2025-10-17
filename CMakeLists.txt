cmake_minimum_required(VERSION 3.10)
project(dbscan-clustering-cpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/*.cpp)

set(SOURCES)
foreach(_src IN LISTS ALL_SOURCES)

    # skip any files generated in the build directory
    string(FIND "${_src}" "${CMAKE_BINARY_DIR}" _found)
    if(NOT _found EQUAL -1)
        continue()
    endif()

    # skip CMake internal dirs
    if(_src MATCHES "/CMakeFiles/")
        continue()
    endif()

    # skip common developer or virtualenv folders that may contain unrelated .cpp files
    if(_src MATCHES "/\\.venv/" OR _src MATCHES "/venv/" OR _src MATCHES "/\\.venv_metrics/" OR _src MATCHES "/\\.git/")
        continue()
    endif()

    list(APPEND SOURCES "${_src}")
endforeach()

if(NOT SOURCES)
    message(FATAL_ERROR "No source files found (*.cpp). Ensure kmeans.cpp or other .cpp files exist in the project.")
endif()

add_executable(dbscan ${SOURCES})

target_include_directories(dbscan PRIVATE ${CMAKE_SOURCE_DIR})

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(dbscan PRIVATE OpenMP::OpenMP_CXX)
    message(STATUS "Found OpenMP: linking OpenMP::OpenMP_CXX")
else()
    message(WARNING "OpenMP not found. The code includes <omp.h> and may fail to build or run multi-threaded optimally.")
endif()

set_target_properties(dbscan PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

install(TARGETS dbscan RUNTIME DESTINATION bin)