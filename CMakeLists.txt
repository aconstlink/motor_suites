cmake_minimum_required( VERSION 3.6 )
project( "motor_suites" )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )

add_subdirectory( motor )
list( APPEND CMAKE_MODULE_PATH 
  "${CMAKE_CURRENT_LIST_DIR}/motor/cmake/macros" )

include( visual_studio_generate_bat_dll )
include( emacs_default_directory )

motor_emacs_default_directory( "${CMAKE_CURRENT_LIST_DIR}/motor" )

#set( CMAKE_CXX_STANDARD 17 )

add_subdirectory( "suite_core" )
#add_subdirectory( "suite_proto" )
#add_subdirectory( "suite_math" )
#add_subdirectory( "suite_graphics" )
#add_subdirectory( "suite_physics" )

motor_vs_gen_bat_for_dlls()

