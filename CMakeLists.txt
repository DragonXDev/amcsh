cmake_minimum_required(VERSION 3.10)
project(amcsh VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -flto -fomit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")

# Find required packages
find_package(Threads REQUIRED)

# On macOS, libedit is keg-only, so we need to set the paths manually
if(APPLE)
    include_directories(/opt/homebrew/opt/libedit/include)
    link_directories(/opt/homebrew/opt/libedit/lib)
    set(LIBEDIT_LIBRARIES edit)
endif()

# Source files
set(SOURCES
    src/main.c
    src/parser.c
    src/executor.c
    src/builtins.c
    src/history.c
    src/completion.c
    src/job_control.c
    src/thread_pool.c
    src/cmd_cache.c
)

# Header files
set(HEADERS
    include/amcsh.h
    include/parser.h
    include/executor.h
    include/builtins.h
    include/history.h
    include/completion.h
    include/job_control.h
)

# Create executable
add_executable(amcsh ${SOURCES})

# Include directories
target_include_directories(amcsh PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(amcsh PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBEDIT_LIBRARIES}
)
