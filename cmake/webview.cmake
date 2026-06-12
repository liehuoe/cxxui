if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(MSWebView2 QUIET)
    if(NOT MSWebView2_FOUND)
        add_library(MSWebView2::headers INTERFACE IMPORTED)
        target_compile_features(MSWebView2::headers INTERFACE cxx_std_11)
        # Download MSWebView2
        cmake_policy(PUSH)
        # Avoid warning related to FetchContent and DOWNLOAD_EXTRACT_TIMESTAMP
        if(POLICY CMP0135)
            cmake_policy(SET CMP0135 NEW)
        endif()
        if(NOT COMMAND FetchContent_Declare)
            include(FetchContent)
        endif()
        set(FC_NAME microsoft_web_webview2)
        FetchContent_Declare(${FC_NAME}
            URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.3405.78")
        FetchContent_MakeAvailable(${FC_NAME})
        set(MSWebView2_ROOT "${${FC_NAME}_SOURCE_DIR}")
        cmake_policy(POP)
        # MSWebView2_INCLUDE_DIR
        find_path(MSWebView2_INCLUDE_DIR WebView2.h
        PATHS
            "${MSWebView2_ROOT}/build/native"
            "${MSWebView2_ROOT}"
        PATH_SUFFIXES include
        NO_CMAKE_FIND_ROOT_PATH)
        set_target_properties(MSWebView2::headers PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${MSWebView2_INCLUDE_DIR}")
    endif()
    target_link_libraries(${PROJECT_NAME} INTERFACE MSWebView2::headers)
endif()
