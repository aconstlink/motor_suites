

set( subdirs
    "00_mtr"
    "01_container"
    )

if( WIN32 )
  #list( APPEND subdirs "some_file" )
endif()

foreach( _subdir ${subdirs} )

    set( app_name "${_subdir}" )
    add_subdirectory( ${_subdir} )
    set_target_properties( ${app_name} PROPERTIES FOLDER "suite_memory" )

    install( TARGETS ${app_name} EXPORT suites-targets 
        RUNTIME DESTINATION bin )
endforeach()
