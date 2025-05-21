cmake_minimum_required(VERSION 3.0.0)
project(detectTests VERSION 0.1.0)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(amqplib 
  -lqpid-proton-proactor
  -lqpid-proton-core 
  -lqpid-proton-cpp
  -lqpid-proton
  pthread 
  sasl2
  -lMLW_AMQP
) 

add_executable(addMultipleITSTest src/addMultipleITSTest.cpp ../src/local_ms_detector.cpp)
target_link_libraries(addMultipleITSTest PUBLIC 
    gtest_main 
    its
    ${amqplib}
)

add_executable(detectPositionConsistencyTest src/detectPositionConsistencyTest.cpp)
target_link_libraries(detectPositionConsistencyTest PUBLIC 
    gtest_main 
    predictor
    its
)


add_test(NAME addMultipleITSTest
COMMAND addMultipleITSTest)

add_test(NAME detectPositionConsistencyTest
        COMMAND detectPositionConsistencyTest)

