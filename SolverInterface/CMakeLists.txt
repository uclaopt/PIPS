include_directories(../PIPS-S/Basic)
add_library(ClpBALPInterface ClpBALPInterface.cpp)
add_library(CbcBALPInterface CbcBALPInterface.cpp)


set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

add_executable(cbcExtensive cbcExtensive.cpp)
add_executable(cbcExtensiveSMPS cbcExtensiveSMPS.cpp)

target_link_libraries(cbcExtensive pipss stochInput CbcBALPInterface ${CBC_LIBS} ${CLP_LIB} ${COIN_LIBS} ${MATH_LIBS})
target_link_libraries(cbcExtensiveSMPS pipss stochInput CbcBALPInterface ${CBC_LIBS} ${CLP_LIB} ${COIN_LIBS} ${MATH_LIBS})
