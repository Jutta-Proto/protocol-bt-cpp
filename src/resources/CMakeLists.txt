cmake_minimum_required(VERSION 3.16)

# Ensure machine files exist:
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/machinefiles)
    message(FATAL_ERROR "Machine files not found. Please make sure you read the chapter about machine files in the README.md first.")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/machinefiles/JOE_MACHINES.TXT)
    message(FATAL_ERROR "'JOE_MACHINES.TXT' machine file not found. Please make sure you read the chapter about machine files in the README.md first.")
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/machinefiles DESTINATION ${CMAKE_BINARY_DIR})
