cmake_minimum_required(VERSION 3.7)
project(mask
        LANGUAGES CXX
        # major.minor.patch/revision.tweak
        VERSION 1.1.0.0)

set(CMAKE_CXX_STANDARD 20)

set(SRC_LIST Main.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})

add_definitions(
        -DMASK_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
        -DMASK_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
        -DMASK_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
        -DMASK_VERSION_TWEAK=${CMAKE_PROJECT_VERSION_TWEAK}
        )

install(TARGETS ${PROJECT_NAME} DESTINATION bin)