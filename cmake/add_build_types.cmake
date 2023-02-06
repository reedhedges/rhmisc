
# Adds additional build types (values for CMAKE_BUILD_TYPE) that enable sanitizer (dynamic analysis) compile options (for testing) to all targets (if target is defined after including this file)  (with add_compile_options()).
# RelWithMemorySanitizers DebugWithMemorySanitizers RelWithUBAndDataSanitizers DebugWithUBAndDataSanitizers RelWithAllSanitizers DebugWithAllSanitizers MinSizeRelWithMemorySanitizers MinSizeRelWithUBAndDataSanitizers MinSizeRelWithAllSanitizers.  
# Modes starting with Debug.* also add -g -Og.  Modes starting with MinSizeRel.* also add -Os.  Modes starting with Rel.* also add -O3.
# Some sanitizers require linking to additional libraries (e.g. libubsan and libasan).  These are added to link libraries (with link_libraries()) as well.
# IN addition to adding compile options with add_compile_options() and extra libraries added with link_libraries(),
# SANITIZER_COMPILE_OPTIONS_LIST and SANITIZER_LINK_LIBRARIES_LIST are set to CMake lists (semicolon-separated) of flags and SANITIZER_COMPILE_OPTIONS_STRING and SANITIZER_LINK_LIBRARIES_STRING variables are set to strings with space-separated compiler or linker flags. (You can use these variables to pass options to other projects.)
# Note: You may need to also add these options (in addition to standard CMake build types) to other build tools such as conan and your IDE so they can be recognized or selected.")
# Note: this currently can't be used when generating "Multi-Config" makefiles; it only checks CMAKE_BUILD_TYPE during CMake configuration.
# Note: If using gcc, -fsanitizer-recover options are included, but you must also run with halt_on_erro=0 set in the ASAN_OPTIONS environment variable to tell the asan library to continue, e.g.:
#   ASAN_OPTIONS=halt_on_error=0  ./testSanitizers

# TODO option to check cmake options rather than build types.
# TODO add thread sanitizer mode
# TODO do something correct for "Multi-Config" makefile generation. Use generator expressions instead of checking CMAKE_BUILD_TYPE.
# TODO check for MSVC
# TODO also check C compiler id?
# TODO add option to enable certain sanitizers/safety features on normal Release and Debug modes (rather than ...With...Sanitizers modes) , which are suitible for actual release without as much performance impact as others.
# TODO warn if CMAKE_BUILD_TYPE doesn't match any of our build types


set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "RelWithMemorySanitizers" "DebugWithMemorySanitizers" "MinSizeRelWithMemorySanitizers" "RelWithUBAndDataSanitizers" "DebugWithUBAndDataSanitizers" "MinSizeRelWithUBAndDataSanitizers" "RelWithThreadSanitizers" "DebugWithThreadSanitizers" "MinSizeRelWithThreadSanitizers" "RelWithAllSanitizers" "DebugWithAllSanitizers" "MinSizeRelWithAllSanitizers")

message(STATUS "${PROJECT_NAME}: add_build_types(): Adding more options for CMAKE_BUILD_TYPE: RelWithMemorySanitizers DebugWithMemorySanitizers RelWithUBAndDataSanitizers DebugWithUBAndDataSanitizers RelWithThreadSanitizers DebugWithThreadSanitizers RelWithAllSanitizers DebugWithAllSanitizers MinSizeRelWithMemorySanitizers MinSizeRelWithUBAndDataSanitizers MinSizeRelWithThreadSanitizers MinSizeRelWithAllSanitizers.  You may need to also add these options (in addition to standard CMake build types) to other build tools such as conan and your IDE so they can be recognized or selected.")

if(NOT CMAKE_BUILD_TYPE)
  message(WARNING "${PROJECT_NAME}: add_build_types(): CMAKE_BUILD_TYPE not set. (Not adding any additional compile options.)")
  return()
endif()

set(debug_compile_options -g -Og)
set(release_compile_options -O3)
set(minsizerel_compile_options -Os)

set(SANITIZER_COMPILE_OPTIONS_LIST "")
set(SANITIZER_LINK_LIBRARIES_LIST "")

if(CMAKE_BUILD_TYPE MATCHES "MinSizeRel.*")
  add_compile_options(${minsizerel_compile_options})
elseif(CMAKE_BUILD_TYPE MATCHES "Rel.*")
  add_compile_options(${release_compile_options})
elseif(CMAKE_BUILD_TYPE MATCHES "Debug.*")
  add_compile_options(${debug_compile_options})
else()
  message(WARNING "${PROJECT_NAME}: add_build_types(): Unrecognized CMAKE_BUILD_TYPE \"${CMAKE_BUILD_TYPE}\": Must match one of these patterns: \"Debug.*\", \"Rel.*\", or \"MinSizeRel.*\".")
endif()



if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(memory_sanitizers_compile_options -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope)
  # TODO:  -fvtable-verify=std)
  set(memory_sanitizers_link_libraries asan)
  # TODO What library do we link to for -fvtable-verify to work?
  set(undefined_and_data_sanitizers_compile_options -fsanitize=undefined -fsanitize-recover=undefined -fsanitize=float-cast-overflow -fsanitize=float-divide-by-zero -fno-omit-frame-pointer)
  set(undefined_and_data_sanitizers_link_libraries ubsan)
  set(thread_sanitizers_compile_options -fsanitize=thread -fno-omit-frame-pointer)
  set(thread_sanitizers_link_libraries tsan)
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(memory_sanitizers_compile_options -fsanitize=address -fno-omit-frame-pointer) 
  # Can't use this with -fsanitize=address; do we need separate stack sanitizers mode? -- -fsanitize=safe-stack)
  set(memory_sanitizers_link_libraries asan)
  set(undefined_and_data_sanitizers_compile_options -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=unsigned-integer-overflow -fsanitize=local-bounds -fsanitize=nullability-arg -fsanitize=nullability-assign -fsanitize=nullability-return)
  set(undefined_and_data_sanitizers_link_libraries ubsan)
  set(thread_sanitizers_compile_options -fsanitize=thread)
  set(thread_sanitizers_link_libraries tsan)
else()
  message(WARNING "${PROJECT_NAME}: add_build_types(): Unrecognized C++ compiler \"${CMAKE_CXX_COMPILER_ID}\", can't add sanitizer options. (Currently recognized C++ compiler IDs are \"GNU\" and \".*Clang\".")
endif()

# TODO on aarch64 GCC and clang have Branch Target Identification (-mbranch-protection=none|bti|pac-ret+leaf|pac-ret+leaf+b-key|pac-ret|standard) (requires aarch64 hardware support) and Controlflow Enforcement Technology (-fcf-protection=full|branch|return|none|check).

if(CMAKE_BUILD_TYPE MATCHES ".*WithMemorySanitizers" OR CMAKE_BUILD_TYPE MATCHES ".*WithAllSanitizers")
  add_compile_options(-g ${memory_sanitizers_compile_options})
  link_libraries(${memory_sanitizers_link_libraries})
  list(APPEND SANITIZER_COMPILE_OPTIONS_LIST ${memory_sanitizers_compile_options})
  list(APPEND SANITIZER_LINK_LIBRARIES_LIST ${memory_sanitizers_link_libraries})
  message(STATUS "${PROJECT_NAME}: add_build_types(): Enabling memory/address/stack sanitizer compiler options for ${CMAKE_CXX_COMPILER_ID} C++ compiler: -g ${memory_sanitizers_compile_options}")
  #if(CMAKE_BUILD_TYPE MATCHES "Rel.*")
  #  add_compile_options(${memory_sanitizers_compile_options_release})
  #endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES ".*WithUBAndDataSanitizers" OR CMAKE_BUILD_TYPE MATCHES ".*WithAllSanitizers")
  add_compile_options(-g ${undefined_and_data_sanitizers_compile_options})
  link_libraries(${undefined_and_data_sanitizers_link_libraries})
  list(APPEND SANITIZER_COMPILE_OPTIONS_LIST ${undefined_and_data_sanitizers_compile_options})
  list(APPEND SANITIZER_LINK_LIBRARIES_LIST ${undefined_and_data_sanitizers_link_libraries})
  message(STATUS "${PROJECT_NAME}: add_build_types(): Enabling undefined behavior/data/type sanitizer compiler options for ${CMAKE_CXX_COMPILER_ID} C++ compiler: -g ${undefined_and_data_sanitizers_compile_options}")
endif()

if(CMAKE_BUILD_TYPE MATCHES ".*WithThreadSanitizers" OR CMAKE_BUILD_TYPE MATCHES ".*WithThreadSanitizer" OR CMAKE_BUILD_TYPE MATCHES ".*WithAllSanitizers")
  add_compile_options(-g ${thread_sanitizers_compile_options})
  link_libraries(${thread_sanitizers_link_libraries})
  list(APPEND SANITIZER_COMPILE_OPTIONS_LIST ${thread_sanitizers_compile_options})
  list(APPEND SANITIZER_LINK_LIBRARIES_LIST ${thread_sanitizers_link_libraries})
  message(STATUS "${PROJECT_NAME}: add_build_types(): Enabling thread sanitizer compiler options for ${CMAKE_CXX_COMPILER_ID} C++ compiler: -g ${thread_sanitizers_compile_options}")
endif()

list(JOIN SANITIZER_COMPILE_OPTIONS_LIST " " SANITIZER_COMPILE_OPTIONS_STRING)
list(JOIN SANITIZER_LINK_LIBRARIES_LIST " " SANITIZER_LINK_LIBRARIES_LIST)

# Read back what add_compile_options() and link_libraries() actually stored:
get_directory_property(dir_comp_opts COMPILE_OPTIONS)
get_directory_property(dir_link_libs LINK_LIBRARIES)
message(STATUS "${PROJECT_NAME}: add_build_types(): COMPILE_OPTIONS is now ${dir_comp_opts}")
message(STATUS "${PROJECT_NAME}: add_build_types(): LINK_LIBRARIES is now ${dir_link_libs}")
message(STATUS "${PROJECT_NAME}: add_build_types(): SANITIZER_COMPILE_OPTIONS_LIST is now \"${SANITIZER_COMPILE_OPTIONS_LIST}\"")
message(STATUS "${PROJECT_NAME}: add_build_types(): SANITIZER_LINK_LIBRARIES_LIST is now \"${SANITIZER_LINK_LIBRARIES_LIST}\"")
message(STATUS "${PROJECT_NAME}: add_build_types(): SANITIZER_COMPILE_OPTIONS_STRING is now \"${SANITIZER_COMPILE_OPTIONS_STRING}\"")
message(STATUS "${PROJECT_NAME}: add_build_types(): SANITIZER_LINK_LIBRARIES_STRING is now \"${SANITIZER_LINK_LIBRARIES_STRING}\"")
