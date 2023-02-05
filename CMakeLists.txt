cmake_minimum_required(VERSION 3.1...3.24)
project(DMA_Bench VERSION 1.0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DOCA REQUIRED doca-dma doca-flow doca-common doca-telemetry 
doca-apsh doca-argp doca-dpi doca-regex  doca-sha doca-ipsec doca-comm-channel doca-compress)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNEBUG")

add_library(params STATIC src/params.c)
target_link_libraries(params ${DOCA_LIBRARIES})
target_link_directories(params PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(params PUBLIC ${DOCA_INCLUDE_DIRS})
target_compile_options(params PUBLIC "-D DOCA_ALLOW_EXPERIMENTAL_API")

add_library(common STATIC src/common.c)
target_link_libraries(common ${DOCA_LIBRARIES})
target_link_directories(common PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(common PUBLIC ${DOCA_INCLUDE_DIRS})
target_compile_options(common PUBLIC "-D DOCA_ALLOW_EXPERIMENTAL_API")

add_library(client STATIC src/client.c)
target_link_libraries(client ${DOCA_LIBRARIES})
target_link_directories(client PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(client PUBLIC ${DOCA_INCLUDE_DIRS})
target_compile_options(client PUBLIC "-D DOCA_ALLOW_EXPERIMENTAL_API")

add_library(server STATIC src/server.c)
target_link_libraries(server ${DOCA_LIBRARIES})
target_link_directories(server PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(server PUBLIC ${DOCA_INCLUDE_DIRS})
target_compile_options(server PUBLIC "-D DOCA_ALLOW_EXPERIMENTAL_API")

add_executable(main src/main.c)
target_link_libraries(main params common client server ${DOCA_LIBRARIES})
target_link_directories(main PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(main PUBLIC ${DOCA_INCLUDE_DIRS})
target_compile_options(main PUBLIC "-D DOCA_ALLOW_EXPERIMENTAL_API")