cmake_minimum_required(VERSION 3.20)

project(bitcoin-system VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# VS CODE DROPDOWN
set(BUILD_TYPE "Release" CACHE STRING "Build Type: Release|Debug")
set_property(CACHE BUILD_TYPE PROPERTY STRINGS Release Debug)
set(LIB_TYPE "STATIC" CACHE STRING "Library Type: STATIC|SHARED")
set_property(CACHE LIB_TYPE PROPERTY STRINGS STATIC SHARED)
option(ENABLE_TESTS "Build tests" OFF)
option(ENABLE_SHANI ON)
option(ENABLE_AVX2 ON)
option(ENABLE_SSE41 ON)
option(ENABLE_AVX512 ON)

# Convert BUILD_TYPE and LIB_TYPE to lowercase for INSTALL_PREFIX
string(TOLOWER ${BUILD_TYPE} BUILD_VARIANT)
string(TOLOWER ${LIB_TYPE} LIB_LINK)
string(TOLOWER "${BUILD_TYPE}_${LIB_TYPE}" INSTALL_DIR)

# Lokaler Build-Pfad und zentraler Install-Pfad
set(BUILD_DIR "build/temp_${BUILD_TYPE}_${LIB_TYPE}")
set(INSTALL_PREFIX "../build/${INSTALL_DIR}")

# Konvertiere INSTALL_PREFIX zu absolutem Pfad
get_filename_component(ABS_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_PREFIX}" ABSOLUTE)

include(ExternalProject)

# BOOST (mit dynamischem Build und Installationspfad)
ExternalProject_Add(boost
    URL https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PREFIX ${BUILD_DIR}/boost
    CONFIGURE_COMMAND ./bootstrap.sh --prefix=${ABS_INSTALL_PREFIX} --with-libraries=iostreams,program_options,regex,thread,url,test
    BUILD_COMMAND ./b2 -j4 variant=${BUILD_VARIANT} threading=multi link=${LIB_LINK} cxxflags="-Wno-enum-constexpr-conversion" -sNO_BZIP2=1 -sNO_ZSTD=1 -d2 -q --reconfigure
    INSTALL_COMMAND ./b2 install -j4 --prefix=${ABS_INSTALL_PREFIX}
    BUILD_IN_SOURCE TRUE
)

# SECP256K1 (mit dynamischem Build und Installationspfad)
ExternalProject_Add(secp256k1
    GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
    GIT_TAG a660a4976efe880bae7982ee410b9e0dc59ac983
    PREFIX ${BUILD_DIR}/secp256k1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND autoreconf -fiv && ./configure --prefix=${ABS_INSTALL_PREFIX} --enable-shared=$<IF:$<STREQUAL:${LIB_TYPE},SHARED>,yes,no> --enable-static=$<IF:$<STREQUAL:${LIB_TYPE},STATIC>,yes,no> --disable-tests --enable-experimental --enable-module-recovery --enable-module-schnorrsig --enable-module-musig --enable-module-ellswift
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE TRUE
)

# LIBBITCOIN-SYSTEM
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

# Dynamische Verknüpfung basierend auf LIB_TYPE
target_link_directories(${PROJECT_NAME} PRIVATE ${INSTALL_PREFIX}/lib)
if(${LIB_TYPE} STREQUAL "SHARED")
    set(LIB_SUFFIX "")
else()
    set(LIB_SUFFIX ".a")
endif()

target_link_libraries(${PROJECT_NAME}
    -lboost_iostreams${LIB_SUFFIX}
    -lboost_program_options${LIB_SUFFIX}
    -lboost_regex${LIB_SUFFIX}
    -lboost_thread${LIB_SUFFIX}
    -lboost_url${LIB_SUFFIX}
    -lsecp256k1${LIB_SUFFIX}
    pthread rt dl
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    BOOST_SPIRIT_USE_PHOENIX_V3 HAVE_CONFIG_H BOOST_ALL_NO_LIB
)

add_dependencies(${PROJECT_NAME} boost secp256k1)

# Installiere das Artefakt
install(TARGETS ${PROJECT_NAME} 
    ARCHIVE DESTINATION ${INSTALL_PREFIX}/lib
    LIBRARY DESTINATION ${INSTALL_PREFIX}/lib
    RUNTIME DESTINATION ${INSTALL_PREFIX}/bin
)
install(DIRECTORY include/ DESTINATION ${INSTALL_PREFIX}/include)

configure_file(bitcoin-system-cmake.pc.in ${CMAKE_CURRENT_BINARY_DIR}/../build/${INSTALL_DIR}/lib/pkgconfig/bitcoin-system.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../build/${INSTALL_DIR}/lib/pkgconfig/bitcoin-system.pc DESTINATION ${INSTALL_PREFIX}/lib/pkgconfig)

# TESTS
if(ENABLE_TESTS)
    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
    add_executable(${PROJECT_NAME}-test ${TEST_SOURCES})
    target_include_directories(${PROJECT_NAME}-test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${INSTALL_PREFIX}/include
    )
    target_link_directories(${PROJECT_NAME}-test PRIVATE ${INSTALL_PREFIX}/lib)
    target_link_libraries(${PROJECT_NAME}-test
        ${PROJECT_NAME}
        -lboost_unit_test_framework${LIB_SUFFIX}
    )
    target_compile_options(${PROJECT_NAME}-test PRIVATE
        -O0 -g -Wall -Wextra -Wno-reorder -Wno-missing-field-initializers -Wno-comment -Wno-sign-compare
    )
    add_test(NAME ${PROJECT_NAME}-test COMMAND ${PROJECT_NAME}-test)
endif()
