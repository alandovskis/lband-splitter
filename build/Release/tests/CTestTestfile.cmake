# CMake generated Testfile for 
# Source directory: /Users/alex/src/splitter/tests
# Build directory: /Users/alex/src/splitter/build/Release/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/Users/alex/src/splitter/build/Release/tests/splitter_tests[1]_include.cmake")
add_test(splitter_unit_tests "/Users/alex/src/splitter/build/Release/tests/splitter_tests")
set_tests_properties(splitter_unit_tests PROPERTIES  LABELS "unit" TIMEOUT "300" _BACKTRACE_TRIPLES "/Users/alex/src/splitter/tests/CMakeLists.txt;70;add_test;/Users/alex/src/splitter/tests/CMakeLists.txt;0;")
