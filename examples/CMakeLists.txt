add_subdirectory(hello-world)
add_subdirectory(time-counter)
add_subdirectory(client)

if (LINK_SSL)
    #    add_subdirectory(ssl-hello-world)
endif()
