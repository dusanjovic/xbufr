function(dbufr_bin target_name target_source)
  add_executable(${target_name} ${target_source})
  target_link_libraries(${target_name} dbufr)
endfunction()

dbufr_bin(dbufr_dump_table dbufr_dump_table.cpp)
dbufr_bin(dbufr_dump       dbufr_dump.cpp)

install(
  TARGETS dbufr_dump_table dbufr_dump
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)
