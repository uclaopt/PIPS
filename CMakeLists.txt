project(PIPSAll)

cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

# For debugging build, set CMAKE_BUILD_TYPE to DEBUG
# For optimized build, set CMAKE_BUILD_TYPE to RELEASE
# For optimized build with debug info, set CMAKE_BUILD_TYPE to RELWITHDEBINFO
# CMAKE_BUILD_TYPE should be set at the command line or in a toolchain file
if(CMAKE_BUILD_TYPE)
  message("CMake build type is ${CMAKE_BUILD_TYPE}")
  message("Build type default flags are: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
else()
  message("CMAKE_BUILD_TYPE not set!")
endif(CMAKE_BUILD_TYPE)
  
option(BUILD_ALL "Build all of PIPS (PIPS-S, PIPS-IPM, PIPS-NLP)" ON)
option(BUILD_PIPS_S "Build PIPS-S" OFF)
option(BUILD_PIPS_IPM "Build PIPS-IPM" OFF)
option(BUILD_PIPS_NLP "Build PIPS-NLP" OFF)

if(BUILD_ALL)
  set(BUILD_PIPS_S ON)
  set(BUILD_PIPS_IPM ON)
  set(BUILD_PIPS_NLP ON)
endif()

message(STATUS "BUILD_ALL=${BUILD_ALL}")
message(STATUS "BUILD_PIPS_S=${BUILD_PIPS_S}")
message(STATUS "BUILD_PIPS_IPM=${BUILD_PIPS_IPM}")
message(STATUS "BUILD_PIPS_NLP=${BUILD_PIPS_NLP}")

#with dumping matrices 1st stage (and global soon)

if(DUMP)
  add_definitions(-DDUMP)
endif()


#with testing of (selected) executables
option(WITH_MAKETEST "Enable 'make test'" ON)

#with timing for PIPS-IPM

#with PIPS IPM AND NLP profile metric
option(WITH_TIMING "Build with timing recording and reporting for PIPS-NLP" OFF)
option(WITH_VERBOSE "Build with extra verbosity level" OFF)
if(WITH_TIMING)
	add_definitions(-DTIMING -DSTOCH_TESTING -DNLPTIMING)
endif()
if(WITH_VERBOSE)
	add_definitions(-DVERBOSE)
endif()

# DOCUMENTATION

# SO FAR ONLY FOR PIPS-NLP
if (BUILD_PIPS_DOC)
  find_package(Doxygen)
  option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})
endif()

if(BUILD_DOCUMENTATION)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is needed to build the documentation.")
  endif()
  # GENERATING FOR PIPS-NLP
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/PIPS-NLP/Docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/PIPS-NLP/Doxyfile)

  configure_file(${doxyfile_in} ${doxyfile} @ONLY)

  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

  install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
endif()



# PIPS-IPM and PIPS-NLP require OpenMP
if (BUILD_PIPS_IPM OR BUILD_PIPS_NLP)
  find_package(OpenMP)
endif()

# include different "whole archive" linking options depending on system
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message(STATUS "Detected Darwin turning off --whole-archive")
  set(WHOLE_ARCHIVE "")
  set(NO_WHOLE_ARCHIVE "")

  # Clang doesn't have a Fortran compiler in its suite (yet),
  # so detect libraries for gfortran; we need equivalents to
  # libgfortran and libquadmath, which are implicitly
  # linked by flags in CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES
  find_package(GFortranLibs REQUIRED)

  # Add directory containing libgfortran and libquadmath to
  # linker. Should also contain libgomp, if not using
  # Intel OpenMP runtime
  link_directories(${GFORTRAN_LIBRARIES_DIR})
  if((BUILD_PIPS_IPM OR BUILD_PIPS_NLP) AND NOT OpenMP_FOUND)
    message(STATUS "OpenMP not found!")

    # Try to find Intel OpenMP library first, since this
    # OpenMP runtime is the default in Clang 3.7
    find_package(LibIomp)
    if(LIBIOMP_FOUND)
      if(APPLE)
	# Apple clang's -fopenmp flag only works with -lgomp
	set(OpenMP_CXX_FLAGS "-liomp5")
      else()
	# LLVM clang's trunk works with libiomp
	set(OpenMP_CXX_FLAGS "-fopenmp -liomp5")
      endif()
      set(OpenMP_C_FLAGS "${OpenMP_CXX_FLAGS}")
      include_directories(${LIBIOMP_INCLUDE_DIR})
      link_directories(${LIBIOMP_LIBRARIES_DIR})
      message(STATUS "Intel OpenMP libraries directory is ${LIBIOMP_LIBRARIES_DIR}")

    else() # Otherwise, fall back to GOMP (gcc OpenMP) runtime
      set(OpenMP_CXX_FLAGS "-fopenmp -lgomp")
      set(OpenMP_C_FLAGS "${OpenMP_CXX_FLAGS}")
      include_directories(${LIBGOMP_INCLUDE_DIR})
    endif()
  endif()
else (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(HAVE_CLANG FALSE)
  set(WHOLE_ARCHIVE "-Wl,--whole-archive")
  set(NO_WHOLE_ARCHIVE "-Wl,--no-whole-archive")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#if(COMMAND cmake_policy)
#	cmake_policy(SET CMP0003 NEW)
#endif(COMMAND cmake_policy)

#if (BUILD_SHARED_LIBS)
#	message(STATUS "Using shared libraries")
#	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a")
#endif (BUILD_SHARED_LIBS)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.dylib;.a")

#find_package(MPI REQUIRED)
#if (MPI_FOUND)
#	message(STATUS "MPI is found: ${MPI_CXX_INCLUDE_PATH}")
#endif(MPI_FOUND)

message(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
if(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a") #need this to find PARDISO
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dy")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneQ-static")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a") #need this to find PARDISO
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dy")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if(IS_XE6 OR IS_XK7 OR IS_XC30)
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -dynamic")
    else(IS_XE6 OR IS_XK7 OR IS_XC30)
    	set(CMAKE_CXX_COMPILER "mpicxx")
    	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fpermissive")
    endif(IS_XE6 OR IS_XK7 OR IS_XC30)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	message(STATUS "running on mac")
        if (NOT CMAKE_CXX_COMPILER)
  	   set(CMAKE_CXX_COMPILER "mpicxx")
        endif (NOT CMAKE_CXX_COMPILER)
	set(IS_MACOS TRUE)
endif(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")


# inspired by elemental-bgp
if(MATH_LIBS)
	message(STATUS "Using user-defined MATH_LIBS=${MATH_LIBS}")
elseif(IS_DIRECTORY $ENV{MKLROOT})
  #set(MATH_LIBS "-Wl,--start-group  $ENV{MKLROOT}/lib/intel64/libmkl_intel_lp64.a $ENV{MKLROOT}/lib/intel64/libmkl_sequential.a $ENV{MKLROOT}/lib/intel64/libmkl_core.a -Wl,--end-group -lpthread -lgfortran -ldl")
    set(MATH_LIBS "-Wl,--start-group  -L$ENV{MKLROOT}/lib/intel64 -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -lmkl_avx -lmkl_def -liomp5 -lgfortran -lpthread -lm -ldl")
    #    set(MATH_LIBS "-mkl=parallel")
    message(STATUS "Using MKLROOT:$ENV{MKLROOT} MATH_LIBS=${MATH_LIBS}")
else(MATH_LIBS)
    message(STATUS "Searching system path for MATH_LIBS")
    enable_language(Fortran)
    message(STATUS "FC = ${FC}")
    message(STATUS "Fortran compiler name is ${CMAKE_Fortran_COMPILER}")
    message(STATUS "Fortran compiler ID is ${CMAKE_Fortran_COMPILER_ID}")
    find_package(LAPACK REQUIRED)
    message(STATUS " LAPACK_LIBRARIES:  ${LAPACK_LIBRARIES}")
    message(STATUS " CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES:  ${CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES}")
    set(MATH_LIBS ${LAPACK_LIBRARIES} -ldl -l${CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES} ${OpenMP_CXX_FLAGS})
    message(STATUS " MATH_LIBS:  ${MATH_LIBS}")
endif(MATH_LIBS)

find_library(DL_LIBRARY NAMES dl)
#find_library(FL_LIBRARY NAMES fl PATHS /usr/lib /usr/local/lib /usr/local/opt/flex)

set(SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs)

#CBC
if(COIN_DIR)
  message(STATUS "Using user-defined COIN_DIR=${COIN_DIR}")
else(COIN_DIR)
  set(COIN_DIR ${SHARED_DIR}/CBC/src)
endif(COIN_DIR)
if(NOT EXISTS "${COIN_DIR}/lib/pkgconfig/coinutils.pc")
	message(FATAL_ERROR "Please run \"./wgetCBC.sh\" in ${SHARED_DIR}/CBC")
endif(NOT EXISTS "${COIN_DIR}/lib/pkgconfig/coinutils.pc")

set(ENV{PKG_CONFIG_PATH} ${COIN_DIR}/lib/pkgconfig)
find_package(PkgConfig REQUIRED)
pkg_check_modules(COINUTILS REQUIRED coinutils)
pkg_check_modules(CLP REQUIRED clp)
pkg_check_modules(CBC REQUIRED cbc)
find_library(CLP_LIB NAMES libClp.a Clp PATHS ${COIN_DIR}/lib)

message(STATUS "Find all COINUTILS_LIBRARIES:${COINUTILS_LIBRARIES}")
set(COIN_LIBS "")
foreach(f ${COINUTILS_LIBRARIES})
	if(NOT ${f} MATCHES "m" AND NOT ${f} MATCHES "blas" AND NOT ${f} MATCHES "lapack") # skip libm
	    message(STATUS "find ${f}...")
		find_library(X NAMES ${f} HINTS "${COIN_DIR}/lib" PATHS /usr/lib /usr/local/lib /usr/lib64/ /usr/lib/x86_64-linux-gnu/)
		message(STATUS "found ${f}:=${X}")
		set(COIN_LIBS ${COIN_LIBS} ${X})
		unset(X CACHE)
	endif(NOT ${f} MATCHES "m" AND NOT ${f} MATCHES "blas" AND NOT ${f} MATCHES "lapack")
endforeach(f)
message(STATUS "COIN_LIBS:=${COIN_LIBS}")

message(STATUS "CBC_LIBRARY_DIRS: ${CBC_LIBRARY_DIRS}")
find_library(CBCSolver_LIB NAMES libCbcSolver.a CbcSolver HINTS ${CBC_LIBRARY_DIRS})
find_library(CBC_LIB NAMES libCbc.a Cbc HINTS ${CBC_LIBRARY_DIRS})
find_library(OSICLP_LIB NAMES libOsiClp.a OsiClp HINTS ${CBC_LIBRARY_DIRS})
find_library(OSI_LIB NAMES libOsi.a Osi HINTS ${CBC_LIBRARY_DIRS})
find_library(CGL_LIB NAMES libCgl.a Cgl HINTS ${CBC_LIBRARY_DIRS})

set(CBC_LIBS ${CBCSolver_LIB} ${CBC_LIB} ${CGL_LIB} ${OSICLP_LIB} ${OSI_LIB})
include_directories(${COINUTILS_INCLUDE_DIRS})
message(STATUS "CBC_LIBS:=${CBC_LIBS}")
message(STATUS "CBC_INC:=${COINUTILS_INCLUDE_DIRS}")

if(AMPL_DIR)
   find_library(AMPL_LIBRARY NAMES amplsolver.a PATHS ${AMPL_DIR}/lib)
   find_path(AMPL_INCLUDE_DIR asl_pfgh.h HINTS ${AMPL_DIR}/include)
else()
   set(AMPL_DIR ${SHARED_DIR}/ASL/src)
   find_library(AMPL_LIBRARY NAMES amplsolver.a PATHS ${AMPL_DIR})
   find_path(AMPL_INCLUDE_DIR asl_pfgh.h HINTS ${AMPL_DIR})
endif(AMPL_DIR)

message(STATUS "${AMPL_LIBRARY}")
if(AMPL_LIBRARY)
  message(STATUS "AMPL_LIBRARY:=${AMPL_LIBRARY}")
  message(STATUS "AMPL_INCLUDE_DIR:=${AMPL_INCLUDE_DIR}")
  include_directories(${AMPL_INCLUDE_DIR})
  set(HAVE_AMPL TRUE)
else()
  set(HAVE_AMPL FALSE)
  if(BUILD_PIPS_NLP)
    message(FATAL_ERROR "amplsolver library is not found, but PIPS-NLP requires ASL. Please run ./wgetASL.sh in ${SHARED_DIR}/ASL")
  else()
    message(STATUS "amplsolver library is not found. Won't build code that requires ASL. Please run ./wgetASL.sh in ${SHARED_DIR}/ASL")
  endif()
endif(AMPL_LIBRARY)

if(BUILD_PIPS_IPM OR BUILD_PIPS_NLP)
  #HSL Solver and METIS
 if(NOT MA27_DIR)
    set(MA27_DIR ${SHARED_DIR}/MA27/src)
  endif(NOT MA27_DIR)
  if(NOT MA57_DIR)
    set(MA57_DIR ${SHARED_DIR}/MA57/src)
  endif(NOT MA57_DIR)

  find_library(MA27_LIBRARY libma27.a PATHS ${MA27_DIR}/lib)
  find_library(MA57_LIBRARY libma57.a PATHS ${MA57_DIR}/lib)
  if(NOT MA27_LIBRARY AND NOT MA57_LIBRARY)
    message(FATAL_ERROR "Could not find MA27 nor MA57. We need one HSL solver")
  endif(NOT MA27_LIBRARY AND NOT MA57_LIBRARY)
  if(MA27_LIBRARY)
    set(HAVE_MA27 TRUE)
    message(STATUS "MA27_LIBRARY = ${MA27_LIBRARY}")
  endif(MA27_LIBRARY)

  if(NOT METIS_DIR)
    set(METIS_DIR ${SHARED_DIR}/METIS)
  endif(NOT METIS_DIR)
  find_library(PARMETIS_LIBRARY NAME libparmetis.a libparmetis.dylib HINTS ${METIS_DIR}/lib PATHS ${METIS_DIR}/lib)
  message(STATUS ${PARMETIS_LIBRARY})
  find_library(METIS_LIBRARY NAME  libmetis.a libmetis.dylib HINTS ${METIS_DIR}/lib PATHS ${METIS_DIR}/lib)
  message(STATUS ${METIS_LIBRARY})
  find_path(METIS_INCLUDE_DIR NAME metis.h HINTS ${METIS_DIR}/include)
  message(STATUS ${METIS_INCLUDE_DIR})

  if(METIS_INCLUDE_DIR AND METIS_LIBRARY)
    include_directories(${METIS_INCLUDE_DIR})
    set(HAVE_METIS TRUE)
  else()
    message(FATAL_ERROR "METIS not found! METIS is required for PIPS-IPM and PIPS-NLP. Please run ./wgetMETIS.sh in ${SHARED_DIR}/METIS")
  endif(METIS_INCLUDE_DIR AND METIS_LIBRARY)
  if(MA57_LIBRARY)
    message(STATUS "MA57_LIBRARY = ${MA57_LIBRARY}")
    include_directories(${METIS_DIR}/Lib)
    set(HAVE_MA57 TRUE)
  else()
    set(HAVE_MA57 FALSE)
  endif(MA57_LIBRARY)

  #MA86 is not needed at this time
  #find_library(MA86_LIBRARY hsl_ma86 PATHS ${MA86_DIR}/lib)

  #PARDISO Solver
  set(PARDISO_DIR ${SHARED_DIR}/PARDISO/src)
  set(HAVE_PARDISO FALSE)
  if(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")
    set(HAVE_PARDISO TRUE)
    #find_library(PARDISO_LIBRARY NAMES libpardiso490-BGP_32.so PATHS ${PARDISO_DIR})
    #message(STATUS ${PARDISO_LIBRARY})
    set(PARDISO_LIBRARY "${PARDISO_DIR}/libpardiso490-BGP_32.so")
    set(PARDISO_LIBRARY32 "${PARDISO_DIR}/libpardiso490-BGP_32_32fact.so")
    #set(PARDISO_LIBRARY "libpardiso490-BGP_32.so")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    message(STATUS "BGP")
  else(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")
    if(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneQ-static")
      set(HAVE_PARDISO TRUE)
      set(PARDISO_LIBRARY "${PARDISO_DIR}/libpardiso490-BGQ_64.so")
      set(PARDISO_LIBRARY32 "${PARDISO_DIR}/libpardiso490-BGQ_64.so")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS) ${OpenMP_EXE_LINKER_FLAGS}")
      message(STATUS "BGQ")
    else(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneQ-static")
      if(IS_XE6)
        #find_library(PARDISO_LIBRARY NAMES "libpardiso495-GNU470-XE6.so"	  PATHS "/scratch/rosa/petra/shared_libs/")
	set(PARDISO_LIBRARY "/scratch/rosa/petra/shared_libs/libpardiso495-GNU430-XE6.so")
	message(STATUS "PARDISO library found, using ${PARDISO_LIBRARY}")
      else(IS_XE6)
	if(IS_XK7)
	  set(PARDISO_LIBRARY "/scratch/todi/petra/shared_libs/libpardiso495-GNU430-XK7.so")
	else(IS_XK7)
	  if(IS_XC30)
	    set(PARDISO_LIBRARY "${PARDISO_LIBS_STATIC}")
	  else(IS_XC30)
	    set(PARDISO_LIBRARY ${PARDISO_DIR}/libpardiso.so)
	  endif(IS_XC30)
	endif(IS_XK7)
      endif(IS_XE6)

      if(EXISTS ${PARDISO_LIBRARY} OR IS_XC30)
	FIND_PACKAGE( OpenMP REQUIRED )
	set(HAVE_PARDISO TRUE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
      else(EXISTS ${PARDISO_LIBRARY} OR IS_XC30)
	set(PARDISO_LIBRARY "")
      endif(EXISTS ${PARDISO_LIBRARY} OR IS_XC30)
    endif(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneQ-static")
  endif(${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")

  message(STATUS "Pardiso lib: ${PARDISO_LIBRARY}  and HAVE_PARDISO is ${HAVE_PARDISO}")


  if(NOT MUMPS_DIR)
    set(MUMPS_DIR ${SHARED_DIR}/MUMPS)
  endif(NOT MUMPS_DIR)
  find_library(MUMPS_LIBRARY libdmumps.a PATHS ${MUMPS_DIR}/lib)
  find_path(MUMPS_INCLUDE_DIR NAME dmumps_c.h HINTS ${MUMPS_DIR}/include)
  if(MUMPS_INCLUDE_DIR AND MUMPS_LIBRARY AND PARMETIS_LIBRARY)
    # we assume the other libraries are in there
    set(HAVE_MUMPS TRUE)

    # you may need parmetis if mumps was build with parmetis (we recommend using parmetis). For more info
    # see the readme file in ThirdPartyLibs/MUMPS and the sample build file in PIPS-NLP/Core/LinearSolvers/MumpsSolver
    # In short: insert something like the following line to MUMPS_LIBRARY below
    # -L/home/petra1/work/installs/parmetis-4.0.3/lib -lparmetis

    # hack(ish) way of specifying the many mumps libraries
    # - put all the  mumps libraries: -ldmumps -lmumps_common -lpord
    # - scalapack and blacs should be set in ${SCALAPACK_LIBRARIES} using Toolchain.cmake (or hardcoded here)
    # - mpifort also needed
    # - parmetis 5.0 or later required by mumps but no need to specify it here: it should be detected by the build system at this point
    if(NOT SCALAPACK_LIBRARIES)
      MESSAGE("SCALAPACK_LIBRARIES for MUMPS not set. Default linking for Ubuntu.")
      set(SCALAPACK_LIBRARIES "-lscalapack-openmpi -lblacs-openmpi  -lblacsF77init-openmpi -lblacsCinit-openmpi") 
    endif(NOT SCALAPACK_LIBRARIES)
    # -lmpi_mpifh includes the OpenMPI Fortran MPI interface used when
    # building MUMPS.
    # building MUMPS. MPICH requires -lmpifort
    if(NOT MUMPS_FORT_LIB)
      MESSAGE("MUMPS_FORT_LIB not set for linking the Fortran MPI interface library required by MUMPS. Using OpenMPI Fortran interface -lmpi_mpifh")
      set(MUMPS_FORT_LIB "-lmpi_mpifh")
    endif(NOT MUMPS_FORT_LIB)
    set(MUMPS_LIBRARY "-L${MUMPS_DIR}/lib -ldmumps -lmumps_common -lpord ${MUMPS_FORT_LIB} ${SCALAPACK_LIBRARIES}")
    MESSAGE( "SCALAPACK_LIBRARIES: ${SCALAPACK_LIBRARIES}")
    MESSAGE( "MUMPS_LIBRARIES: ${MUMPS_LIBRARY}")
    include_directories(${MUMPS_INCLUDE_DIR})
  else ()
    message(STATUS "MUMPS library and/or headers are not present. Will build PIPS without MUMPS")
    set(MUMPS_LIBRARY "")
  endif(MUMPS_INCLUDE_DIR AND MUMPS_LIBRARY AND PARMETIS_LIBRARY)

  #setting CXX_FLAGS
  if(HAVE_MA27)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_MA27")
  else(HAVE_MA27)
    set(MA27_LIBRARY "")
  endif(HAVE_MA27)

  if(HAVE_MA57)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_MA57")
  else(HAVE_MA57)
    set(MA57_LIBRARY "")
  endif(HAVE_MA57)

  if(HAVE_MUMPS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_MUMPS")
  else(HAVE_MUMPS)
    set(MUMPS_LIBRARY "")
  endif(HAVE_MUMPS)

  #if(HAVE_MA86)
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_MA86")
  #else(HAVE_MA57)
  #  set(MA86_LIBRARY "")
  #endif(HAVE_MA86)

  if(HAVE_PARDISO)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_PARDISO")
  else(HAVE_PARDISO)
    set(PARDISO_LIBRARY "")
  endif(HAVE_PARDISO)

endif()

if (BUILD_PIPS_S AND BUILD_PIPS_IPM)
  #ConicBundle
  set(CB_DIR "${SHARED_DIR}/ConicBundle/src")
  set(CB_LIB "${CB_DIR}/lib/libcb.a")
  if(EXISTS ${CB_LIB})
    set(HAVE_CB TRUE)
  else(EXISTS ${CB_LIB})
    set(HAVE_CB FALSE)
    message(STATUS "Building without ConicBundle library")
  endif(EXISTS ${CB_LIB})

  #SCIP is not used at this time
  #set(SCIP_DIR "${SHARED_DIR}/SCIP/src")
  #set(SCIP_LIBDIR "${SCIP_DIR}/lib")
  #set(SCIP_INCDIR "${SCIP_DIR}/src")
  #if(EXISTS "${SCIP_LIBDIR}/libscip.a")
  #	set(HAVE_SCIP TRUE)
  #	set(SCIP_LIBS ${SCIP_LIBDIR}/libscip.a;${SCIP_LIBDIR}/libobjscip.a;${SCIP_LIBDIR}/liblpiclp.a;${SCIP_LIBDIR}/libnlpi.a)
  #	if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")
  #		set(SCIP_LIBS ${SCIP_LIBS};"readline";"ncurses")
  #	endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")
  #else(EXISTS "${SCIP_LIBDIR}/libscip.a")
  #	set(HAVE_SCIP FALSE)
  #	message(STATUS "Building without SCIP")
  #endif(EXISTS "${SCIP_LIBDIR}/libscip.a")

  # Gurobi is not used at this time
  # # see if we have gurobi
  # # must set GUROBI_DIR environmental variable with root of gurobi architecture directory
  # # e.g. gurobi461/linux64
  # if(IS_DIRECTORY $ENV{GUROBI_DIR})
  # 	find_library(GUROBIC NAMES gurobi46 HINTS "$ENV{GUROBI_DIR}/lib")
  # 	find_library(GUROBICPP NAMES "gurobi_c++" HINTS "$ENV{GUROBI_DIR}/lib")
  # 	find_package (Threads)
  # 	set(HAVE_GUROBI TRUE)
  # 	set(GUROBI_LIBS ${GUROBIC} ${GUROBICPP} ${CMAKE_THREAD_LIBS_INIT})
  # 	message(STATUS "Building with Gurobi")
  # else(IS_DIRECTORY $ENV{GUROBI_DIR})
  # 	set(HAVE_GUROBI FALSE)
  # 	message(STATUS "Building without Gurobi")
  # endif(IS_DIRECTORY $ENV{GUROBI_DIR})
  # if(HAVE_GUROBI)
  # 	add_subdirectory(src/GUROBI)
  # endif(HAVE_GUROBI)
endif()

#Boost
if (BUILD_PIPS_S OR BUILD_PIPS_IPM)
  find_package(Boost COMPONENTS log log_setup regex system filesystem thread chrono date_time)
  include_directories(${Boost_INCLUDE_DIR})
  #message("---- ${Boost_INCLUDE_DIR}") #message("---- ${Boost_LIBRARIES}")
  if (NOT Boost_FOUND)
    message("! A Boost version containing BOOST.LOG has NOT been found.")
    message("! Please specify the location of the Boost library containing 'log' package.")
    message("! For this create a 'Toolchain.cmake' file or edit the one from the root ")
    message("! directory and use 'set(BOOST_ROOT \"/path/to/boost\")'.")
    message("! Then run  'cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain.cmake ..' in the build directory.")
    message(FATAL_ERROR "Configuration aborted due to missing BOOST libraries!")
  endif(NOT Boost_FOUND)
endif()

###############################################################################

###############################################################################


# must set TAU_MAKEFILE, TAU_OPTIONS environment variables
#set(CMAKE_CXX_COMPILER "tau_cxx.sh")

#IF(CMAKE_COMPILER_IS_GNUCXX)
#	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#ENDIF(CMAKE_COMPILER_IS_GNUCXX)

include_directories(Input)
include_directories(Input/OPF_Matpower)
include_directories(SolverInterface)

# Required for all of PIPS
add_subdirectory(Input)
add_subdirectory(Utilities) #TODO(oxberry1@llnl.gov): Does this code do anything?

# Required for PIPS-S
if (BUILD_PIPS_S)
  add_subdirectory(SolverInterface)
  add_subdirectory(PIPS-S)
endif(BUILD_PIPS_S)

# Required for PIPS-IPM
if (BUILD_PIPS_IPM)
  add_subdirectory(PIPS-IPM)
endif()

# These directories depend on both PIPS-S and PIPS-IPM
if (BUILD_PIPS_S AND BUILD_PIPS_IPM)
  add_subdirectory(Lagrange)
  #add_subdirectory(UCRolling)
  add_subdirectory(UCTests)
  add_subdirectory(BasisBootstrap)
endif()

# Required for PIPS-NLP
if (BUILD_PIPS_NLP)
  add_subdirectory(PIPS-NLP)
endif(BUILD_PIPS_NLP)

# this will print a warning message, should figure out how to silence it
#add_library(pipsForSwift SHARED)
# add any libraries needed by swift app here
#target_link_libraries(pipsForSwift pipss stochInput ClpRecourseSolver ${CLP_LIB} ${COIN_LIBS} ${MATH_LIBS})

#if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")
#add_subdirectory(src/Tests)
#add_subdirectory(src/SuboptimizationSerial)
#add_subdirectory(src/Utilities)
#add_subdirectory(src/SMPS)
#endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "BlueGeneP-static")

##########################################################
# CMake Tests
##########################################################
if (WITH_MAKETEST)
enable_testing()

if(BUILD_PIPS_IPM)
  add_test(NAME PIPS-IPM-multipleTests COMMAND sh ${PROJECT_SOURCE_DIR}/PIPS-IPM/Test/pipsipmMultiTests.sh $<TARGET_FILE:pipsipmFromRaw> ${PROJECT_SOURCE_DIR}/PIPS-S/Test/rawInput)
endif(BUILD_PIPS_IPM)

if(BUILD_PIPS_S)
  add_test(NAME PIPS-S-multipleTests COMMAND sh ${PROJECT_SOURCE_DIR}/PIPS-S/Test/pipssMultiTests.sh $<TARGET_FILE:pipssFromRaw> ${PROJECT_SOURCE_DIR}/PIPS-S/Test/rawInput)
endif(BUILD_PIPS_S)

if(BUILD_PIPS_NLP)
  add_test(NAME PIPS-NLP-simpleTest1 COMMAND $<TARGET_FILE:parmodel1> -objcheck)
  add_test(NAME PIPS-NLP-simpleTest2 COMMAND $<TARGET_FILE:parmodel2> -objcheck)
  add_test(NAME PIPS-NLP-simpleTest3 COMMAND $<TARGET_FILE:parmodel3> -objcheck)
  add_test(NAME PIPS-NLP-linkingTest COMMAND $<TARGET_FILE:parmodelLink> -objcheck)
endif(BUILD_PIPS_NLP)

endif(WITH_MAKETEST)

message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_STANDARD_LIBRIES: ${CMAKE_CXX_STANDARD_LIBRARIES}")
message(STATUS "CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "inc_dirs: ${inc_dirs}")
get_property(link_dirs DIRECTORY PROPERTY LINK_DIRECTORIES)
message(STATUS "link_dirs: ${link_dirs}")
