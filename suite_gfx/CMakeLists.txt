
set( tests
    "00_text_renderer.cpp"
    "01_00_primitive_renderer_2d.cpp"
    "01_01_primitive_renderer_2d.cpp"
    "02_primitive_renderer_3d.cpp"
    "03_sprite_renderer.cpp"
    "04_sprite_sheet.cpp"
    "05_spline_rendering_3d.cpp"
    "06_keyframes_3d.cpp"
    )

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
      PRIVATE motor::memory
      PRIVATE motor::application
      PRIVATE motor::graphics
      PRIVATE motor::platform
      PRIVATE motor::gfx
      PRIVATE motor::property
      PRIVATE motor::format )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_gfx" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()
