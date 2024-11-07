
set( tests
    "00_simple_rendering.cpp"
    "01_simple_msl.cpp"
    "02_framebuffer.cpp"
    "03_multi_geometry.cpp"
    "04_framebuffer_mrt.cpp"
    "05_array_object_raw.cpp"
    "06_array_object.cpp"
    "07_texture_array_raw.cpp"
    "08_texture_array.cpp"
    "09_00_stream_out.cpp"
    "09_01_stream_out.cpp"
    "09_02_stream_out.cpp"
    "09_03_stream_out.cpp"
    "10_00_gs_pass_through_raw.cpp"
    "10_01_gs_pass_through.cpp"
    "10_02_gs_generate.cpp"
    "10_03_gs_streamout.cpp"
    "11_msl_auto.cpp"
    "12_msl_libraries.cpp"
    "13_msl_buildins_1.cpp"
    "13_msl_buildins_2.cpp"
    "14_noise_texture.cpp"
    "15_wire_shader_slots.cpp"
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
      PRIVATE motor::noise 
      PRIVATE motor::io 
      PRIVATE motor::concurrent 
      PRIVATE motor::memory
      PRIVATE motor::application
      PRIVATE motor::graphics
      PRIVATE motor::platform
      PRIVATE motor::gfx
      PRIVATE motor::geometry
      PRIVATE motor::format )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_graphics" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()
