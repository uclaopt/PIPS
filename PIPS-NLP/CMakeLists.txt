#add_definitions(-DTIMING -DSTOCH_TESTING) # timing output

include_directories(Core/Abstract Core/Vector Core/Utilities Core/QpGen
  Core/SparseLinearAlgebra Core/DenseLinearAlgebra Core/Readers
  Core/LinearSolvers/Ma27Solver Core/LinearSolvers/Ma57Solver
  Core/LinearSolvers/Ma86Solver Core/LinearSolvers/PardisoSolver Core/LinearSolvers/MumpsSolver
  Core/LinearSolvers/BiCGStabSolver
  Core/LinearSolvers/UmfPackSolver)
include_directories(Core/StochLinearAlgebra)
include_directories(Core/NlpSolvers Core/NlpGen)
include_directories(Core/NlpInfo)
include_directories(Core/NlpStoch)
include_directories(Core/CInterface)
#include_directories(Core/MtxSchurDecomp)
include_directories(Core/StructureSolver)
include_directories(.)

if(HAVE_PETSC)
include_directories(Core/PetscLinearAlgebra)
endif(HAVE_PETSC)

add_subdirectory(Core)
add_subdirectory(Test)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

add_library(global_var 
	global_var.cpp
	./Core/Utilities/PerfMetrics.cpp
)

#message("HAVE_PARDISO: ${HAVE_PARDISO}")
#message("HAVE_MA57: ${HAVE_MA57}")

if(HAVE_MA57)

#set(MYBLAS_LIB "/usr/lib/libblas.a")
set(MYBLAS_LIB "/usr/local/lib/libopenblas.a")

if(HAVE_PETSC)

### parallel filter line search IPM with MA57/Pardiso, use NL as the input, seperate Hessian
  add_executable(pipsnlp_parallel Drivers/parallelAmplDriver.cpp)
  target_link_libraries(pipsnlp_parallel
    genNLparallel StochInfoFromAMPL nlpstoch
    pipsnlpFLSStoch
#    ReducedSpaceSolver blockSCSolver
	reducedSolverSpecial
#    schurDecompSolver
    nlpgensparse LU_solvers
    nlpbase nlpsparse nlpdense nlpstochla
	global_var
    ${COIN_LIBS}
    ${MA27_LIBRARY} ${MA57_LIBRARY}
    ${UMF_ALLINONE_LIBRARY}
    ${PARMETIS_LIBRARY}
    ${METIS_LIBRARY} ${AMPL_LIBRARY} ${MATH_LIBS}
#    ${PETSC_LIBRARY}
    ${PARDISO_LIBRARY}
    )

### serial filter line search IPM with option to choose MA57/Pardiso. Schur-comp method is available
  add_executable(pipsnlp_serial Drivers/serialAmplDriver.cpp)
  target_link_libraries(pipsnlp_serial
     genNLserial updateFromAMPL  nlpcNlpGenSparse  nlpwithsolver
     nlpgensparse  LU_solvers
#	schurDecompSolver
#     ReducedSpaceSolver
	reducedSolverSpecial
    petsc_solver_sparse
    nlpsparse nlpdense
    pipsnlpFLS  nlpbase
	global_var
#	nlpstoch
#	nlpstochla
    ${COIN_LIBS}
    ${AMPL_LIBRARY}
    ${MA57_LIBRARY}  ${MA27_LIBRARY}
#    ${UMF_ALLINONE_LIBRARY}
#    ${UMFPACK_LIBRARY_PIPS}
#    ${AMD_LIBRARY}
    ${PARMETIS_LIBRARY}
    ${METIS_LIBRARY}
    ${PARDISO_LIBRARY}
    ${PETSC_LIBRARY}
    ${MATH_LIBS} )
else(HAVE_PETSC)

###  parallel filter line search IPM with MA57/Pardiso, use NL as the input, seperate Hessian
  add_executable(pipsnlp_parallel Drivers/parallelAmplDriver.cpp)
  target_link_libraries(pipsnlp_parallel
    genNLparallel StochInfoFromAMPL
    pipsnlpFLSStoch nlpstoch nlpstochla
    nlpgensparse nlpbase nlpsparse nlpdense
#    schurDecompSolver
#    ReducedSpaceSolver blockSCSolver
	reducedSolverSpecial
	global_var
	nlpstoch
     ${MA57_LIBRARY} ${MA27_LIBRARY} ${MUMPS_LIBRARY}  
     ${PARMETIS_LIBRARY} ${METIS_LIBRARY} 
    ${PARDISO_LIBRARY} 
    LU_solvers ${UMF_ALLINONE_LIBRARY} 
    ${AMPL_LIBRARY} ${COIN_LIBS} ${MATH_LIBS} 
    )

### serial filter line search IPM with option to choose MA57/Pardiso. Schur-comp method is available
  add_executable(pipsnlp_serial Drivers/serialAmplDriver.cpp)
  target_link_libraries(pipsnlp_serial
    genNLserial updateFromAMPL
    pipsnlpFLS nlpcNlpGenSparse nlpwithsolver
    nlpgensparse nlpbase nlpsparse nlpdense
	global_var
#	nlpstoch
#	nlpstochla
#	 schurDecompSolver
#     ReducedSpaceSolver reducedSolverSpecial
    ${MA27_LIBRARY} ${MA57_LIBRARY} 
    ${PARMETIS_LIBRARY} ${METIS_LIBRARY} 
    ${PARDISO_LIBRARY}
    LU_solvers ${UMF_ALLINONE_LIBRARY}
    ${AMPL_LIBRARY} ${COIN_LIBS} ${MATH_LIBS}
    )

### parallel filter line search IPM with DCOPF network aggregation preconditioner
  add_executable(pipsqpFromDCOPF Drivers/parallelDcopfDriver.cpp)
  target_link_libraries(pipsqpFromDCOPF
	sInfoQPLP nlpstoch
	dcopfcinput nlpstochInputBase  aggregationstoch 
    pipsnlpFLSStoch nlpstochla
    reducedSolverSpecial
    nlpgensparse nlpbase nlpsparse nlpdense
    global_var
    ${MA27_LIBRARY} ${MA57_LIBRARY} ${MUMPS_LIBRARY}
    ${PARMETIS_LIBRARY} ${METIS_LIBRARY} 
    ${PARDISO_LIBRARY}
    LU_solvers ${UMF_ALLINONE_LIBRARY}
    ${AMPL_LIBRARY} ${COIN_LIBS} ${MATH_LIBS}
   )

endif(HAVE_PETSC)

endif(HAVE_MA57)


add_library(pipsnlp SHARED Drivers/pipsipmNlp_C_callbacks.cpp)
target_link_libraries(pipsnlp
  #${WHOLE_ARCHIVE}
  updateFromCallBack  
  nlpcNlpGenSparse  
  nlpwithsolver
  nlpgensparse
  LU_solvers
  nlpsparse 
  nlpdense
  pipsnlpFLS  
  nlpbase
  global_var
  nlpstoch
  nlpstochla	
  reducedSolverSpecial
  ${COIN_LIBS}
  ${MA57_LIBRARY}
  ${MA27_LIBRARY}
  ${UMF_ALLINONE_LIBRARY}
  ${PARMETIS_LIBRARY} ${METIS_LIBRARY} 
  ${PARDISO_LIBRARY}
  #${NO_WHOLE_ARCHIVE}
  ${MATH_LIBS} )

 
#parallel structure interface 
add_library(parpipsnlp SHARED Drivers/parallelPipsNlp_C_Callback.cpp)
  target_link_libraries(parpipsnlp
  reducedSolverSpecial
  ${WHOLE_ARCHIVE}
  global_var
  updateFromCallBack  
  #nlpcNlpGenSparse  
  #nlpwithsolver
  LU_solvers
  nlpsparse 
  nlpdense
  pipsnlpFLSStoch 
  nlpstoch 
  nlpstochla 
  nlpgensparse 
  nlpbase 
  genStructparallel
  structStochInfo
  ${MUMPS_LIBRARY}
  #pipsnlpFLS  
  ${COIN_LIBS}
  ${MA57_LIBRARY}
  ${MA27_LIBRARY}
  ${UMF_ALLINONE_LIBRARY}
  ${PARMETIS_LIBRARY} ${METIS_LIBRARY} 
  ${PARDISO_LIBRARY}
  ${NO_WHOLE_ARCHIVE}
  ${MATH_LIBS} )

install(TARGETS parpipsnlp LIBRARY DESTINATION lib)
install(TARGETS pipsnlp LIBRARY DESTINATION lib)
install(DIRECTORY ${COIN_DIR}/lib DESTINATION . FILES_MATCHING PATTERN "libCoinUtils.*")


