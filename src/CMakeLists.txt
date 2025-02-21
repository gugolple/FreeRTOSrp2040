# Pull in SDK (must be before project)
include(${PATH_PIO_SDK_CMAKE})

# Pull in FreeRTOS
include(${PATH_FREERTOS_CMAKE})

project(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(main_blinky
        main.c
        common.c
)

target_compile_definitions(main_blinky PRIVATE
        mainCREATE_SIMPLE_BLINKY_DEMO_ONLY=1
)

target_include_directories(main_blinky PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_options( main_blinky PUBLIC
        ### Gnu/Clang C Options
        $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
)

target_link_libraries(main_blinky pico_stdlib FreeRTOS-Kernel FreeRTOS-Kernel-Heap1)
pico_add_extra_outputs(main_blinky)
