cmake_minimum_required(VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE ON)

project(nsf2vgm VERSION 0.1.0)


if (WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()


add_subdirectory(lib/cJSON)
add_subdirectory(lib/cwalk)
add_subdirectory(lib/rogueutil)

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
	nsfrip.c
	nsfrip_vgm.c
	nsftime.c
)

add_executable(nsf2vgm
	blip_buf.c
	nesapu.c
	nescpu.c
	nesbus.c
	nesfloat.c
	nsf.c
	nsfreader_file.c
	nsfrip.c
	nsfrip_vgm.c
	ansicon.c
	nsf2vgm.c
)
target_link_libraries(nsf2vgm cJSON cwalk)