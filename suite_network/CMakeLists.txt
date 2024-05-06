

set( tests
    "00_client_localhost.cpp"
    "00_server_localhost.cpp"
    "01_client_twitch_irc.cpp"
    "xx_server_localhost_just_listen.cpp"
    #"01_server_client_app.cpp"
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
      PRIVATE motor::social
      PRIVATE motor::application
      PRIVATE motor::platform
      PRIVATE motor::io )

    set_target_properties( ${app_name} PROPERTIES FOLDER "suites/suite_network" )

    set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
    target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")

    install( TARGETS ${app_name} EXPORT suites-targets 
      RUNTIME DESTINATION bin )

endforeach()
