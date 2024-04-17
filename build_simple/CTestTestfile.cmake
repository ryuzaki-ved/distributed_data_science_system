# CMake generated Testfile for 
# Source directory: D:/Big Projects/Distributed Data Science System
# Build directory: D:/Big Projects/Distributed Data Science System/build_simple
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(DDSTests "D:/Big Projects/Distributed Data Science System/build_simple/Debug/dds_tests.exe")
  set_tests_properties(DDSTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Big Projects/Distributed Data Science System/CMakeLists.txt;65;add_test;D:/Big Projects/Distributed Data Science System/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(DDSTests "D:/Big Projects/Distributed Data Science System/build_simple/Release/dds_tests.exe")
  set_tests_properties(DDSTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Big Projects/Distributed Data Science System/CMakeLists.txt;65;add_test;D:/Big Projects/Distributed Data Science System/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(DDSTests "D:/Big Projects/Distributed Data Science System/build_simple/MinSizeRel/dds_tests.exe")
  set_tests_properties(DDSTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Big Projects/Distributed Data Science System/CMakeLists.txt;65;add_test;D:/Big Projects/Distributed Data Science System/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(DDSTests "D:/Big Projects/Distributed Data Science System/build_simple/RelWithDebInfo/dds_tests.exe")
  set_tests_properties(DDSTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Big Projects/Distributed Data Science System/CMakeLists.txt;65;add_test;D:/Big Projects/Distributed Data Science System/CMakeLists.txt;0;")
else()
  add_test(DDSTests NOT_AVAILABLE)
endif()
