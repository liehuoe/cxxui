function(make_example target)
    add_executable(${target} ${target}.cpp)
    if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wshadow -pedantic-errors -Werror)
        # 静态编译
        target_link_options(${target} PRIVATE -static-libgcc -static-libstdc++)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${target} PRIVATE /W4 /WX)
        target_compile_options(${target} PRIVATE /utf-8 /permissive)
        # 静态编译
        set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        # 支持win7
        target_compile_definitions(${target} PRIVATE _WIN32_WINNT=0x0601 WINVER=0x0601)
    endif()
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_link_libraries(${target} PRIVATE ${CMAKE_PROJECT_NAME})
endfunction()

make_example(basic_win)
make_example(web_win)