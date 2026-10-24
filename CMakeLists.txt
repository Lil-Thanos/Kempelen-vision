cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(
        CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>:$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>"
    )
endif()

project(ia-chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV CONFIG REQUIRED)

add_executable(ia-chess
    "ia-chess.cpp"
    "ia-chess.h"
    "src/frameCapture.h"
    "src/frameCapture.cpp"
    "src/overlay.h"
    "src/overlay.cpp"
    "src/stockfish.h"
    "src/stockfish.cpp"
    "src/fen.h"
    "src/detector.h"
    "src/detector.cpp"
    "src/asyncEngine.h"
)

 add_custom_command(TARGET ia-chess POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/engine
            $<TARGET_FILE_DIR:ia-chess>/engine
)

target_link_libraries(ia-chess PRIVATE ${OpenCV_LIBS})

message(STATUS "OpenCV version : ${OpenCV_VERSION}")