

set( subdirs
    "00_basic_allocator"
    "00_memory"
    "01_basics"
    "02_containers"
    )

if( WIN32 )
  #list( APPEND subdirs "some_file" )
endif()

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_core" )
endforeach()
