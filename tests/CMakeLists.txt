# Test targets for mbpoll
# Unit tests for serial module functions

# Enable testing with CTest
enable_testing()

# Serial module tests
add_executable(test_serial test_serial.c ../src/serial.c)
target_include_directories(test_serial PRIVATE ../src)
add_test(NAME test_serial COMMAND test_serial)

# Utils module tests
add_executable(test_utils test_utils.c ../src/utils.c)
target_include_directories(test_utils PRIVATE ../src)
target_compile_definitions(test_utils PRIVATE SKIP_FAILURE_EXIT)
add_test(NAME test_utils COMMAND test_utils)
