

set( subdirs
    "00_basics"
    )

if( WIN32 )
  #list( APPEND subdirs "some_file" )
endif()

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_core" )
endforeach()
