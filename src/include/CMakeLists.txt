cmake_minimum_required(VERSION 3.16)

target_include_directories(bt PUBLIC  
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_sources(bt PRIVATE
     # Header files (useful in IDEs)
     bt/BLEDevice.hpp
     bt/BLEHelper.hpp
     bt/ByteEncDecoder.hpp)

target_include_directories(jutta_bt_proto PUBLIC  
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_sources(jutta_bt_proto PRIVATE
     # Header files (useful in IDEs)
    jutta_bt_proto/CoffeeMaker.hpp
    jutta_bt_proto/Utils.hpp)

target_include_directories(logger PUBLIC  
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_sources(logger PRIVATE
     # Header files (useful in IDEs)
     logger/Logger.hpp)

install(DIRECTORY bt DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY jutta_bt_proto DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY logger DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})