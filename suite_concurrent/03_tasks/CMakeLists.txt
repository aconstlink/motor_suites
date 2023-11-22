
set( sources

    main.cpp

    )

motor_vs_src_dir( sources ) 

add_executable( ${app_name} ${sources} ) 
target_link_libraries( ${app_name} 
    motor::log
    motor::memeory )

set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")
