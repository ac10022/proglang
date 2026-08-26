cmake_minimum_required(VERSION 3.10)
 
project(proglang)

add_compile_options(-Wall -Wextra -Wno-psabi -Wno-unused-parameter -Wno-unused-variable)
#											 ^^^ 				   ^^^ remove these when project is done

# add only sources here
set(SOURCES
	src/file.c
	src/lexer.c
	src/parser.c
	src/main.c
	src/optimiser.c
	src/cleanup.c
	src/codegen.c
)

add_executable(proglang ${SOURCES}
 src/arena.c
 include/arena.h)

# this will tell compiler to look into proglang/include/ if it sees
#	#include "lexer.h"
# so no need to add headers into SOURCES
#
# it also makes project independent from include paths, i.e.
# 	#include "../include/lexer.h"
target_include_directories(proglang
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(proglang PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)
