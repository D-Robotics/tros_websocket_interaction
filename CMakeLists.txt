cmake_minimum_required(VERSION 3.8)
project(tros_websocket_interaction)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/proto
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/include
)

link_directories(
  third_party/protobuf/lib
)

add_executable(tros_websocket_client
  src/client.cpp
  src/protobuf/x3.pb.cc 
  )
# add_executable(tros_websocket_server src/server.cpp)

set(OPENCV_LIB opencv_world)

# Link libraries
target_link_libraries(tros_websocket_client 
  ssl 
  crypto 
  z 
  libprotobuf.a
  ${OPENCV_LIB}
)

# target_link_libraries(tros_websocket_server 
#   ssl 
#   crypto 
#   z 
#   libprotobuf.a
#   ${OPENCV_LIB}
# )

install(TARGETS tros_websocket_client DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)  
install(DIRECTORY config DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)  
install(DIRECTORY launch DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)  
