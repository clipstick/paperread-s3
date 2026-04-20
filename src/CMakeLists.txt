# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# The main application component depends on the epdiy component so that
# headers like <epdiy.h> and "epd_board.h" from components/epdiy/src are
# available to sources under src/.
idf_component_register(SRCS ${app_sources} REQUIRES epdiy)

# ULP is only needed for ESP32 classic (FSM ULP), not for ESP32-S3
# ESP32-S3 uses EXT1 wake source instead for button wake from deep sleep
if(CONFIG_IDF_TARGET_ESP32)
    # 1. The ULP app name must be unique (if multiple components use ULP).
    set(ulp_app_name ulp_main)
    #
    # 2. Specify all assembly source files.
    #    Files should be placed into a separate directory (in this case, ulp/),
    #    which should not be added to COMPONENT_SRCS.
    set(ulp_s_sources "../ulp/main.S")
    #
    # 3. List all the component source files which include automatically
    #    generated ULP export file, ${ulp_app_name}.h:
    set(ulp_exp_dep_srcs "main.cpp")
    #
    # 4. Call function to build ULP binary and embed in project using the argument
    #    values above.
    ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")
endif()
