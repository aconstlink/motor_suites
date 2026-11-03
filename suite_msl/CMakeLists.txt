

set( tests
    "00_msl_basics.cpp"
    "01_msl_from_file.cpp"
    "02_parser_test.cpp"
    "03_noise_buildins.cpp"
    "04_hlsl_clip_depth.cpp"
    )

if( WIN32 )
  #list( APPEND subdirs "some_file" )
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
      PRIVATE motor::msl
      PRIVATE motor::io )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_msl" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()

if( WIN32 )
  target_link_libraries( 04_hlsl_clip_depth PRIVATE d3dcompiler )
endif()

if( BUILD_TESTING )
  add_test( NAME msl_hlsl_clip_depth COMMAND 04_hlsl_clip_depth )
  set_tests_properties( msl_hlsl_clip_depth PROPERTIES TIMEOUT 30 LABELS "msl;graphics" )
endif()
