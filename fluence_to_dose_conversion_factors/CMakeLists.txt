cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(ConvFactors)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(convFactors convFactors.cc ${sources} ${headers})
add_executable(testStoppingPower test_stopping_power.cc)
add_executable(testNeutronSecondaries test_neutron_secondaries.cc)
target_link_libraries(testNeutronSecondaries ${Geant4_LIBRARIES})
target_link_libraries(testStoppingPower ${Geant4_LIBRARIES})
target_link_libraries(convFactors ${Geant4_LIBRARIES})

file(COPY ${PROJECT_SOURCE_DIR}/run.mac DESTINATION ${PROJECT_BINARY_DIR})

install(TARGETS convFactors DESTINATION bin)
