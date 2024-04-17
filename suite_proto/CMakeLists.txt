
set( 00_world_2d_additions
    "00_world_2d/world.h"
)

set( tests
    "00_world_2d.cpp"
    "01_picking_2d.cpp"
    "02_branching_spline.cpp"
    )

foreach( _test ${tests} )

    string( LENGTH ${_test} len )
    math( EXPR app_name_len "${len}-4")
    string( SUBSTRING ${_test} 0 ${app_name_len} app_name )

    #message( length: ${app_name_len})
    #message( substr: ${app_name})

    # append all additional files for 
    # the current sample applications
    set( additions ${app_name}_additions )
    foreach( _addition ${${additions}} )
    list( APPEND _test ${_addition} )
    endforeach()

    motor_vs_src_dir( ${_test} ) 

    add_executable( ${app_name} ${_test}  ) 
    target_link_libraries( ${app_name} 
      PRIVATE motor::io 
      PRIVATE motor::concurrent 
      PRIVATE motor::memory
      PRIVATE motor::application
      PRIVATE motor::graphics
      PRIVATE motor::platform
      PRIVATE motor::gfx
      PRIVATE motor::property
      PRIVATE motor::format
      PRIVATE motor::noise )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_proto" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()
