set(CJSON_SRC cJSON)

add_library(cJSON INTERFACE)

target_sources(cJSON INTERFACE ${CJSON_SRC}/cJSON.c)

target_include_directories(cJSON INTERFACE ${CJSON_SRC})