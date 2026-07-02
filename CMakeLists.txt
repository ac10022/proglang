cmake_minimum_required(VERSION 3.10)
 
project(proglang)

set(SOURCES
	src/file.c
	src/lexer.c
	src/main.c
)

add_executable(proglang ${SOURCES})

target_include_directories(proglang
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(proglang PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)
