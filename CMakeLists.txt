cmake_minimum_required(VERSION 3.29)
project(assign2_buffer_manager C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(assign2_buffer_manager
    buffer_mgr.h
    buffer_mgr_stat.c
    buffer_mgr_stat.h
    dberror.c
    dberror.h
    dt.h
    storage_mgr.h
    test_assign2_1.c
    test_assign2_2.c
    test_helper.h
    buffer_mgr.c
    cmake-build-debug/storage_mgr.c
    cmake-build-debug/storage_mgr.h
    storage_mgr.c)
