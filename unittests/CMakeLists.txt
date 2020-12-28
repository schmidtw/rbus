include(ExternalProject)
#include(GoogleTest)

ExternalProject_Add(
  gtest
  URL https://github.com/google/googletest/archive/master.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
  INSTALL_COMMAND "")

ExternalProject_Get_Property(gtest source_dir binary_dir)

add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES
  "IMPORTED_LOCATION" "${binary_dir}/lib/libgtest.a"
  "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)
set_target_properties(libgmock PROPERTIES
  "IMPORTED_LOCATION" "${binary_dir}/lib/libgmock.a"
  "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

include_directories("${source_dir}/googletest/include" "${source_dir}/googlemock/include" ../include)

# someone needs to figure out how to run this stuff under valgrind
add_custom_target(test_memcheck
  COMMAND "${CMAKE_CTEST_COMMAND} --force-new-ctest-process --test-action memcheck"
  COMMAND cat "${CMAKE_BINARY_DIR}/Testing/Temporary/MemoryChecker.*.log")

add_executable(rbus_gtest.bin 
  rbusValueTest.cpp
  rbusFunctionalityTest.cpp
  rbusObjectTest.cpp
  rbusPropertyTest.cpp
  rbusFilterTest.cpp
  main.cpp)
add_dependencies(rbus_gtest.bin rbus)
target_link_libraries(rbus_gtest.bin rbus libgtest libgmock gcov)

install (TARGETS rbus_gtest.bin
    RUNTIME DESTINATION bin)

if (ENABLE_CODE_COVERAGE)
    install(CODE "execute_process(COMMAND find src/ unittests/ -name *.gcno -exec tar -rvf rbus_src_gcno.tar {} \;)")
    install(FILES ${CMAKE_BINARY_DIR}/rbus_src_gcno.tar DESTINATION bin)
endif()

# not sure why this isn't working
#gtest_discover_tests(rbus_gtest.bin)
