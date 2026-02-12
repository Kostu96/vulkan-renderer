include(FetchContent)

set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    EXCLUDE_FROM_ALL
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
    GIT_TAG "release-3.4.0"
    FIND_PACKAGE_ARGS 3.4.0
)
FetchContent_MakeAvailable(SDL3)
