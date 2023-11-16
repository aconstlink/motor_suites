

set( tests
    "00_test_task.cpp"
    "01_parallel_for.cpp"
    "02_tasks.cpp"
    "03_tasks.cpp"
    "04_thread_pool.cpp"
    "05_parallel_for.cpp"
    "06_loose_scheduler.cpp"
    )

if( WIN32 )
  #list( APPEND subdirs "some_file" )
endif()

foreach( _test ${tests} )

  string( LENGTH ${_test} len )
  math( EXPR app_name_len "${len}-4")
  string( SUBSTRING ${_test} 0 ${app_name_len} app_name )

  message( length: ${app_name_len})
  message( substr: ${app_name})

  motor_vs_src_dir( ${_test} ) 

  add_executable( ${app_name} ${_test} ) 
  target_link_libraries( ${app_name} 
    PRIVATE motor::concurrent 
    PRIVATE motor::memory )

  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_concurrent" )

endforeach()
