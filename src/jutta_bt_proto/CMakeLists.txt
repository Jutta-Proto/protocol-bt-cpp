cmake_minimum_required(VERSION 3.16)

add_library(jutta_bt_proto SHARED CoffeeMaker.cpp
                                  Utils.cpp
                                  CoffeeMakerLoader.cpp)

target_link_libraries(jutta_bt_proto PUBLIC bt date eventpp
                                     PRIVATE logger tinyxml2::tinyxml2 gattlib)

# Set version for shared libraries.
set_target_properties(jutta_bt_proto PROPERTIES
                                     VERSION ${${PROJECT_NAME}_VERSION}
                                     SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR})

install(TARGETS jutta_bt_proto)
