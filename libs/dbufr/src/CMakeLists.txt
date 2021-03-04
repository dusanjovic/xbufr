add_library(dbufr STATIC
  bitreader.cpp
  bufrdecoder.cpp
  bufrfile.cpp
  bufrmessage.cpp
  bufrutil.cpp
  descriptor.cpp
  descriptortablea.cpp
  descriptortableb.cpp
  descriptortabled.cpp
  descriptortablef.cpp
  tablea.cpp
  tableb.cpp
  tabled.cpp
  tablef.cpp

  sqlite3.c
)

set_source_files_properties(sqlite3.c PROPERTIES COMPILE_DEFINITIONS "SQLITE_OMIT_LOAD_EXTENSION=1;SQLITE_THREADSAFE=0")

target_include_directories(dbufr
  PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_compile_definitions(dbufr PUBLIC FMT_HEADER_ONLY)

function(dbufr_bin target_name target_source)
  add_executable(${target_name} ${target_source})
  target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
  target_link_libraries(${target_name} dbufr)
endfunction()

dbufr_bin(load_tables load_tables.cpp)

install(
  TARGETS dbufr load_tables
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")

install(CODE "execute_process(COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/run_load_tables.sh\"
                              WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/bin\"
                             )"
       )
