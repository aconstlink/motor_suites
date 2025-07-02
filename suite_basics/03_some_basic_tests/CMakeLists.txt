

set( sources
    
    main.cpp
    
    )

motor_vs_src_dir( sources ) 

add_executable( ${app_name} ${sources} ) 

target_link_libraries( ${app_name} 
    PUBLIC motor::log 
    PUBLIC motor::memory        
    )

add_test(NAME ${app_name} COMMAND ${app_name} )
