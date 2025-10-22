cmake_minimum_required(VERSION 3.20)
project(libbitcoin-system VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# VS CODE DROPDOWN
set(BUILD_TYPE "Release" CACHE STRING "Build Type: Release|Debug")
set(LIB_TYPE "STATIC" CACHE STRING "Library Type: STATIC|SHARED")
option(ENABLE_TESTS "Build tests" OFF)
option(ENABLE_SHANI ON)
option(ENABLE_AVX2 ON)
option(ENABLE_SSE41 ON)
option(ENABLE_AVX512 ON)

# Convert BUILD_TYPE and LIB_TYPE to lowercase for Boost and INSTALL_PREFIX
string(TOLOWER ${BUILD_TYPE} BOOST_VARIANT)
string(TOLOWER ${LIB_TYPE} BOOST_LINK)
string(TOLOWER "${BUILD_TYPE}_${LIB_TYPE}" INSTALL_DIR)

set(INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${INSTALL_DIR}")
set(BUILD_DIR "${CMAKE_BINARY_DIR}/temp_${BUILD_TYPE}_${LIB_TYPE}")

include(ExternalProject)

# BOOST
ExternalProject_Add(boost
    URL https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PREFIX ${BUILD_DIR}/boost
    CONFIGURE_COMMAND ./bootstrap.sh --prefix=${INSTALL_PREFIX} --with-libraries=iostreams,locale,program_options,regex,thread,url,test
    BUILD_COMMAND ./b2 -j16 variant=${BOOST_VARIANT} threading=multi link=${BOOST_LINK} cxxflags="-Wno-enum-constexpr-conversion" -sNO_BZIP2=1 -sNO_ZSTD=1 -d0 -q --reconfigure
    INSTALL_COMMAND ./b2 install -j16
    BUILD_IN_SOURCE TRUE
)

# SECP256K1
ExternalProject_Add(secp256k1
    GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
    GIT_TAG a660a4976efe880bae7982ee410b9e0dc59ac983
    PREFIX ${BUILD_DIR}/secp256k1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND autoreconf -fiv && ./configure --prefix=${INSTALL_PREFIX} --disable-shared --enable-static --disable-tests --enable-experimental --enable-module-recovery --enable-module-schnorrsig --enable-module-musig --enable-module-ellswift
    BUILD_COMMAND make -j16
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE TRUE
)

# LIBBITCOIN
file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(${PROJECT_NAME} ${LIB_TYPE} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${INSTALL_PREFIX}/include
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<STREQUAL:${BUILD_TYPE},Release>:-O3>
    $<$<STREQUAL:${BUILD_TYPE},Debug>:-O0 -g>
    -Wall -Wextra -Wno-reorder -Wno-missing-field-initializers -Wno-missing-braces
    -Wno-comment -Wno-deprecated-copy -Wno-ignored-attributes -Wno-long-long
    -fno-var-tracking-assignments -Wno-stringop-overread
    $<$<BOOL:${ENABLE_SHANI}>:-msha>
    $<$<BOOL:${ENABLE_AVX2}>:-mavx2>
    $<$<BOOL:${ENABLE_SSE41}>:-msse4.1>
    $<$<BOOL:${ENABLE_AVX512}>:-mavx512f -mavx512bw>
    $<$<STREQUAL:${BUILD_TYPE},Release>:-DNDEBUG>
)

target_link_libraries(${PROJECT_NAME}
    ${INSTALL_PREFIX}/lib/libboost_iostreams.${LIB_TYPE}
    ${INSTALL_PREFIX}/lib/libboost_locale.${LIB_TYPE}
    ${INSTALL_PREFIX}/lib/libboost_program_options.${LIB_TYPE}
    ${INSTALL_PREFIX}/lib/libboost_regex.${LIB_TYPE}
    ${INSTALL_PREFIX}/lib/libboost_thread.${LIB_TYPE}
    ${INSTALL_PREFIX}/lib/libboost_url.${LIB_TYPE}
    ${INSTALL_PREFIX}/lib/libsecp256k1.a
    pthread rt dl
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    BOOST_SPIRIT_USE_PHOENIX_V3 HAVE_CONFIG_H BOOST_ALL_NO_LIB
)

add_dependencies(${PROJECT_NAME} boost secp256k1)

install(TARGETS ${PROJECT_NAME} 
    ARCHIVE DESTINATION ${INSTALL_PREFIX}/lib
    LIBRARY DESTINATION ${INSTALL_PREFIX}/lib
)
install(DIRECTORY include/ DESTINATION ${INSTALL_PREFIX}/include)

configure_file(libbitcoin-system.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system.pc DESTINATION ${INSTALL_PREFIX}/lib/pkgconfig)

# TESTS
if(ENABLE_TESTS)
    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
    add_executable(libbitcoin-system-test ${TEST_SOURCES})
    target_include_directories(libbitcoin-system-test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${INSTALL_PREFIX}/include
    )
    target_link_libraries(libbitcoin-system-test
        ${PROJECT_NAME}
        ${INSTALL_PREFIX}/lib/libboost_unit_test_framework.${LIB_TYPE}
    )
    target_compile_options(libbitcoin-system-test PRIVATE
        -O0 -g -Wall -Wextra -Wno-reorder -Wno-missing-field-initializers
    )
    add_test(NAME libbitcoin-system-test COMMAND libbitcoin-system-test)
endif()
