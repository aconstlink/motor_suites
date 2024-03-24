cmake_minimum_required( VERSION 3.20 )
project( "motor_suites" )

# required by motor
set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )

add_subdirectory( motor )
list( APPEND CMAKE_MODULE_PATH 
  "${CMAKE_CURRENT_LIST_DIR}/motor/cmake/macros" )

include( visual_studio_generate_bat_dll )
include( emacs_default_directory )

motor_emacs_default_directory( "${CMAKE_CURRENT_LIST_DIR}/motor" )

set( CMAKE_DEBUG_POSTFIX d )

add_subdirectory( "suite_basics" )
add_subdirectory( "suite_memory" )
add_subdirectory( "suite_concurrent" )
add_subdirectory( "suite_io" )
add_subdirectory( "suite_math" )
add_subdirectory( "suite_controls" )
add_subdirectory( "suite_msl" )
add_subdirectory( "suite_application" )
add_subdirectory( "suite_platform" )
add_subdirectory( "suite_graphics" )
add_subdirectory( "suite_tool" )
add_subdirectory( "suite_audio" )
add_subdirectory( "suite_format" )
add_subdirectory( "suite_gfx" )
add_subdirectory( "suite_proto" )

install( EXPORT suites-targets DESTINATION "cmake" )


