cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)

project(usb_hid_sim)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(usb_hid_sim)

target_sources(usb_hid_sim PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/main.c
	)

target_include_directories(usb_hid_sim PUBLIC
	${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(usb_hid_sim PUBLIC pico_stdlib pico_unique_id tinyusb_device tinyusb_board)

pico_add_extra_outputs(usb_hid_sim)
