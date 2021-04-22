# macro(TARGET_V8)
#     find_package(v8 CONFIG REQUIRED)
#     target_include_directories(${TARGET_NAME} PRIVATE ${V8_INCLUDE_DIR})
#     target_link_libraries(${TARGET_NAME} ${V8_LIBRARIES})
# endmacro()


macro(TARGET_V8)

# find_package(PkgConfig)
# if(PKG_CONFIG_FOUND)
#     pkg_check_modules(V8 QUIET IMPORTED_TARGET GLOBAL v8 v8_libplatform)
#     if(V8_FOUND)
#         find_file(V8_SNAPSHOT_BLOB snapshot_blob.bin HINTS /usr/bin)
#         message(STATUS "V8 snapshot found at ${V8_SNAPSHOT_BLOB}")
#         set(V8_TARGET PkgConfig::V8)
#     endif()
# endif()
# if(NOT V8_FOUND)
    find_package(V8 REQUIRED)
    set(V8_TARGET V8::V8)
    if(TARGET V8::V8LIBBASE)
        set(V8_TARGET "${V8_TARGET} V8::V8LIBBASE")
    endif()
    if(TARGET V8::V8LIBPLATFORM)
        set(V8_TARGET "${V8_TARGET} V8::V8LIBPLATFORM")
    endif()
# endif()

target_link_libraries(${TARGET_NAME} ${V8_LIBRARIES})

endmacro()