add_executable( tsflib-test 
	test_main.cpp
	test_units.cpp
	test_curve.cpp
	test_clock.cpp
	test_record.cpp
	test_filters.cpp
	)

set_target_properties(tsflib-test PROPERTIES CXX_STANDARD 23)

target_include_directories(tsflib-test PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(tsflib-test ${tsf_lib_deps} tsflib ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_test( NAME tsflib-test COMMAND tsflib-test )