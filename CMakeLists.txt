cmake_minimum_required(VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE ON)

project(nsf2vgm VERSION 0.1.0)

add_executable(nsfdump 
	blip_buf.c
	nesapu.c
	nescpu.c
	nesbus.c
	nesfloat.c
	nsf.c
	nsfreader_file.c
	nsfdump.c)

add_executable(nsftime
	blip_buf.c
	nesapu.c
	nescpu.c
	nesbus.c
	nesfloat.c
	nsf.c
	nsfreader_file.c
	nsftime.c
)