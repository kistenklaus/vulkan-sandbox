
add_executable(sandbox 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(sandbox PUBLIC wrs-logging wrs-gpu wrs)

set_target_properties(sandbox PROPERTIES LINKER_LANGUAGE CXX)
