# cmake_minimum_required(VERSION <specify CMake version here>)
cmake_minimum_required(VERSION 3.19)
project(feeder_yijinjing_ctp)

SET(CMAKE_BUILD_TYPE "Debug")  # Release Debug
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall")
add_compile_options(-std=c++17 -Wall -O2)

SET(ROOT_DIR "/home/work/sys/lib")

set(sub_dir all_include all_lib)
list(APPEND sub_dir boost-1.70.0 libcoral-1.1.0 libfeeder-1.1.0 libflatbuffers-1.12.0 libiconv-1.15 libprotobuf-3.8.0 libswordfish-1.1.0 libx-1.1.0 libyaml-cpp-0.6.3 libzmq-4.3.1 libclickhouse-cpp-2.0.0)
foreach(it ${sub_dir})
    list(APPEND all_include ${ROOT_DIR}/${it}/include)
    list(APPEND all_lib ${ROOT_DIR}/${it}/lib)
endforeach()

SET(CTP_VERSION "libctp-6.3.15")

include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/${CTP_VERSION}/include)
include_directories(${all_include})

link_directories(${CMAKE_CURRENT_LIST_DIR}/lib/${CTP_VERSION}/lib ${all_lib})

aux_source_directory (./src/ctp_feeder SRC_LIST)

SET(FEEDER "yijinjing_ctp_feeder")
SET(READER "reader")

add_executable (${FEEDER} ${SRC_LIST})

target_link_libraries(${FEEDER}
        thostmduserapi_se thosttraderapi_se LinuxDataCollect journal x stdc++fs yaml-cpp pthread dl z clickhouse-cpp-lib-static boost_date_time boost_filesystem boost_regex boost_system  boost_chrono boost_log boost_program_options boost_thread boost_iostreams)

add_executable(${READER} src/reader/reader.cc)
target_link_libraries(${READER}
        journal boost_date_time boost_filesystem boost_regex boost_system  boost_chrono boost_log boost_program_options boost_thread boost_iostreams z protobuf protobuf-lite sodium zmq ssl crypto iconv pthread dl)


FILE(COPY Dockerfile image.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(GLOB COMMON_LIB_NAME "${ROOT_DIR}/common_lib/lib/*so*")
FILE(COPY ${COMMON_LIB_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB LIBS_NAME "${CMAKE_CURRENT_LIST_DIR}/lib/${CTP_VERSION}/lib/*so*")
FILE(COPY ${LIBS_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

FILE(COPY ${COMMON_LIB_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/image.sh "n=$1; v=`./$n -v`; ldd $n | grep /sys/lib/ | awk '{print $3}' | xargs -i cp -L -n {} .; docker build -t $n:$v .; rm -rf \${n}_$v.docker.tar.gz; docker save $n:$v > \${n}_$v.docker.tar && gzip \${n}_$v.docker.tar")
add_custom_target(image COMMAND sh image.sh ${FEEDER} DEPENDS ${FEEDER})