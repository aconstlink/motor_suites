

set( tests
    "00_database.cpp"
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
    PRIVATE motor::io 
    PRIVATE motor::concurrent 
    PRIVATE motor::memory )

  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_io" )

  set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
  target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

endforeach()
