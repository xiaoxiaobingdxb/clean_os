cmake_minimum_required(VERSION 3.5.0)

project(test LANGUAGES C)

# add_subdirectory(../common)
# add_dependencies(test common)


enable_testing()

#unit test
add_executable(hlist_test hlist_test.c)
add_test(NAME hlist_test
COMMAND $<TARGET_FILE:hlist_test>)