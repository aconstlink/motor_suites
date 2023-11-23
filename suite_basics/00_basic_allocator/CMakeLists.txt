

set( sources main.cpp )
motor_vs_src_dir( sources ) 

add_executable( ${app_name} ${sources} ) 
target_link_libraries( ${app_name} 
    PRIVATE motor::core 
    PRIVATE motor::memory )
