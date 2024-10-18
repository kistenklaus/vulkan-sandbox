
add_executable(sandbox 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(sandbox PUBLIC wrs-logging wrs-gpu wrs)

target_compile_options(sandbox PUBLIC -O3)

set_target_properties(sandbox PROPERTIES LINKER_LANGUAGE CXX)
