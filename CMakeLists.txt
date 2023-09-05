cmake_minimum_required(VERSION 3.20.1)
project(voicechange VERSION 0.0.0)

set(CMAKE_C_EXTENSIONS      ON)
set(CMAKE_C_STANDARD        11)
set(CMAKE_CXX_EXTENSIONS    ON)
set(CMAKE_CXX_STANDARD      11)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_C_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    add_compile_options(/utf-8)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/soundtouch)

add_library(${PROJECT_NAME}
STATIC
""
)

add_subdirectory(src)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
            PRIVATE ${LINK_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME}
            PRIVATE ${LINK_LIBRARIES}
            PUBLIC m)
endif()
