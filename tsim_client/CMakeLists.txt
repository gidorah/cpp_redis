###
# compilation options
###
IF (NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF (NOT WIN32)


###
# includes
###
include_directories(${PROJECT_SOURCE_DIR}/includes
                    ${CPP_REDIS_INCLUDES}
                    ${BOOST_INCLUDE_DIR})

link_directories(${BOOST_LIBRARY_DIR})


###
# executable || cmake . -DBUILD_TSIM=true
###
add_executable(tsim_client tsim_client.cpp Call_Back_Timer.h Data_Controller.cpp Data_Controller.h 
              Redis_Handler.cpp Redis_Handler.h Redis_Handler_Extension.cpp Redis_Handler_Extension.h
              Shared_Memory_Handler.cpp Shared_Memory_Handler.h Shared_Memory_Extension.cpp Shared_Memory_Extension.h )

target_link_libraries(tsim_client cpp_redis)
