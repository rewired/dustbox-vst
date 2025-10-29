function(dustbox_enable_warnings target strict)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /bigobj)
        if(strict)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wdelete-non-virtual-dtor)
        if(strict)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor)
        if(strict)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
