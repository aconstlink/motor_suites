

set( sources
    
    main.cpp
    
    )

motor_vs_src_dir( sources ) 

add_executable( ${app_name} ${sources} ) 
target_link_libraries( ${app_name} 
    PRIVATE motor::concurrent 
    PRIVATE motor::memory )
