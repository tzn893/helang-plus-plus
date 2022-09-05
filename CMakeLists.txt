project(helang)

message(STATUS "llvm path ${LLVM_PATH}")
set(CMAKE_PREFIX_PATH ${LLVM_PATH}/lib/cmake)

cmake_minimum_required(VERSION 3.21)
set (CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} "/MD")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} "/MD")

file(GLOB HELANG_LIB_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/*.cpp")
file(GLOB HELANG_LIB_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/*.h")

file(GLOB HELANG_C_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/compiler/*.cpp")
file(GLOB HELANG_C_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/compiler/*.h")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

#llvm_map_components_to_libnames(llvm_libs core)

execute_process(
    COMMAND llvm-config --libs
    OUTPUT_VARIABLE llvm_all_libs
)

string(STRIP ${llvm_all_libs} llvm_libs)
string(REPLACE " " ";" llvm_libs ${llvm_libs})

message(STATUS "llvm include directories ${LLVM_INCLUDE_DIRS}")
message(STATUS "llvm lib files ${llvm_libs}")

add_library(helang_lib ${HELANG_LIB_SOURCE} ${HELANG_LIB_HEADER})
add_executable(helang "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_executable(helang-c ${HELANG_C_SOURCE} ${HELANG_C_HEADER})

target_include_directories(helang     PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/lib" ${LLVM_INCLUDE_DIRS})
target_include_directories(helang_lib PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(helang-c PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/lib)

target_link_libraries(helang_lib ${llvm_libs})
target_link_libraries(helang-c helang_lib ${llvm_libs})
target_link_libraries(helang helang_lib)

add_dependencies(helang helang-c)

file(GLOB TEMPLATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/template/*.c")

add_custom_command(TARGET helang POST_BUILD      
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${LLVM_PATH}/bin/clang.exe ${TEMPLATE_FILE}
        $<TARGET_FILE_DIR:helang>)

