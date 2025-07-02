

set( subdirs
    "00_basic_allocator"
    "00_memory"
    "01_basics"
    "02_containers"
    "03_some_basic_tests"
    "04_vector_pod"
    "05_static_ids"
    "06_00_double_dispatch"
    "06_01_double_dispatch"
    "07_document"
    )

if( WIN32 )
  #list( APPEND subdirs "some_file" )
endif()

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_basics" )

  install( TARGETS ${app_name} EXPORT suites-targets 
    RUNTIME DESTINATION bin )

  

endforeach()
