cmake_minimum_required(VERSION 3.16)

add_library(bt SHARED BLEHelper.cpp
                      BLEDevice.cpp
                      ByteEncDecoder.cpp)
target_link_libraries(bt PRIVATE logger
                         PUBLIC gattlib)

install(TARGETS bt)
