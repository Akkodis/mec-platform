cmake_minimum_required(VERSION 3.0.0)
project(localMSDetector VERSION 0.1.0)

set(BUILD_SHARED_LIBS FALSE) 

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# find_package(ProtonCpp REQUIRED)
find_package(Boost 1.71)
# find_package(MySQLConnectorCPP REQUIRED)

set(amqplib 
  PRIVATE
  -lqpid-proton-proactor
  -lqpid-proton-core 
  -lqpid-proton-cpp
  -lqpid-proton
  #-L/usr/local/lib/libqpid-proton-proactor.so #.1.7.0
  #-L/usr/local/lib/libqpid-proton-core.so #.10.10.0
  #-L/usr/local/lib/libqpid-proton-cpp.so #.12.7.1
  #-L/usr/local/lib/libqpid-proton.so #.11.11.0
  pthread 
  sasl2 
  -lMLW_AMQP
  #-L/usr/local/lib/libMLQ_AMQP.so.1.0
) 

set(kalmanlib
  openblas 
  lapack  
  superlu 
  armadillo)

# enable_testing()
# add_subdirectory(test/googletest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/kalman)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
include_directories(/usr/local/include)
include_directories(/usr/local/include/eigen3)
include_directories(/usr/local/include/mlw_amqp)
include_directories(${Boost_INCLUDE_DIR})
include_directories(/usr/include /usr/include/conncpp)

add_library(predictor src/predictor.cpp)
target_link_libraries(predictor PRIVATE ${Boost_LIBRARIES} gtsam ${kalmanlib})

add_library(its src/its.cpp  src/its_utils.cpp)
target_link_libraries(its PRIVATE ${Boost_LIBRARIES} predictor)

add_library(localMSDetector src/local_ms_detector.cpp src/main.cpp)
target_link_libraries(localMSDetector PRIVATE ${Boost_LIBRARIES} its gtsam /usr/lib/x86_64-linux-gnu/libmysqlcppconn.so.7 )

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE localMSDetector its ${Boost_LIBRARIES} predictor ${amqplib} /usr/lib/x86_64-linux-gnu/libmysqlcppconn.so.7 )

# add_subdirectory(test)