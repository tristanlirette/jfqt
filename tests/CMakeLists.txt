qt6_add_executable(test_line_parser test_line_parser.cpp ../src/line_parser.cpp)
target_include_directories(test_line_parser PRIVATE ../src)
target_link_libraries(test_line_parser PRIVATE Qt6::Test)
add_test(NAME line_parser COMMAND test_line_parser)
