cmake_minimum_required(VERSION 3.10)
 
project(proglang)

add_compile_options(-Wall -Wextra -Wno-psabi)

set(SOURCES
	src/file.c
	src/lexer.c
	src/parser.c
	src/main.c
)

add_executable(proglang ${SOURCES}
 src/optimiser.c)

target_include_directories(proglang
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(proglang PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)
