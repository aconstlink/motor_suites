

set( tests
    "00_vector.cpp"
    "01_matrix.cpp"
    "02_angle.cpp"
    "03_spline_keyframe.cpp"
    )

if( MSVC )
  #message("hello")
  list( APPEND tests "w_00_simd.cpp" )
endif()

foreach( _test ${tests} )

    string( LENGTH ${_test} len )
    math( EXPR app_name_len "${len}-4")
    string( SUBSTRING ${_test} 0 ${app_name_len} app_name )

    #message( length: ${app_name_len})
    #message( substr: ${app_name})

    motor_vs_src_dir( ${_test} ) 

    add_executable( ${app_name} ${_test} ) 
    target_link_libraries( ${app_name} 
      PRIVATE motor::math 
      PRIVATE motor::log
      
       )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_math" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()
