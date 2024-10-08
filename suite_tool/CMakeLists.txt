
set( tests
    "00_imgui.cpp"
    "01_imgui.cpp"
    "02_profiling.cpp"
    "03_sprite_editor.cpp"
    "04_00_imnodes_wire.cpp"
    "04_01_imnodes_wire.cpp"
    "04_02_imnodes_wire.cpp"
    "05_imgui_properties.cpp"
    "06_imgui_scene_visitor.cpp"
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
      PRIVATE motor::tool )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_tool" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()
