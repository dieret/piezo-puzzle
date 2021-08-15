# Largely inspired by https://www.kuon.ch/post/2018-07-11-avr-cmake/
# https://github.com/mkleemann/cmake-avr/blob/master/generic-gcc-avr.cmake

cmake_minimum_required(VERSION 3.13)

project(piezo_puzzle LANGUAGES C)

set(PRODUCT_NAME main)

# Make sure this is updated proerly
# ==============================================================================

add_compile_definitions(F_CPU=1000000UL)
add_compile_definitions(BAUDRATE=1200UL)

set(MCU atmega8)

# ==============================================================================

set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)
set(PROG_TYPE stk500v2)

# Find port for programmer
execute_process(COMMAND pavr2cmd --prog-port OUTPUT_VARIABLE PORT OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Port for programmer: ${PORT}")

# todo: what are all of these flags doing?
add_compile_options(
    -mmcu=${MCU} # MCU
    -std=gnu99 # C99 standard
    -Os # optimize
    -Wall # enable warnings
    -Wno-main
    -Wundef
    -pedantic
    -Wstrict-prototypes
    -Werror
    -Wfatal-errors
    -Wl,--relax,--gc-sections
    -g
    -gdwarf-2
    -funsigned-char # a few optimizations
    -funsigned-bitfields
    -fpack-struct
    -fshort-enums
    -ffunction-sections
    -fdata-sections
    -fno-split-wide-types
    -fno-tree-scev-cprop
)

file(GLOB SRC_FILES "*.c")  # Find all files in src folder

# link_directories(${CMAKE_SOURCE_DIR}/../my_library)
# add_library(uart STATIC ${CMAKE_SOURCE_DIR}/../../my_library/src/uart.c)
# target_include_directories(uart PUBLIC ${CMAKE_SOURCE_DIR}/../../my_library/include/)
# set_target_properties(uart PROPERTIES LINKER_LANGUAGE C)

# Create one target
add_executable(${PRODUCT_NAME} ${SRC_FILES})
# target_link_libraries(${PRODUCT_NAME} uart)


# Rename the output to .elf as we will create multiple files
set_target_properties(${PRODUCT_NAME} PROPERTIES OUTPUT_NAME ${PRODUCT_NAME}.elf)

# Strip binary for upload
add_custom_target(strip ALL avr-strip ${PRODUCT_NAME}.elf DEPENDS ${PRODUCT_NAME})

# Transform binary into hex file, we ignore the eeprom segments in the step
add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.hex DEPENDS strip)

# Upload the firmware with avrdude
add_custom_target(upload avrdude  -c "${PROG_TYPE}" -p "${MCU}" -P "${PORT}" -U "flash:w:${PRODUCT_NAME}.hex" DEPENDS hex)

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PRODUCT_NAME}.hex;${PRODUCT_NAME}.eeprom;${PRODUCT_NAME}.lst")
