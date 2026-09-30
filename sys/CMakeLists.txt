add_library(sys)

target_include_directories(
  sys
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL3_INCLUDE_DIRS}
)

target_link_libraries(
  sys
  PUBLIC core SDL3::SDL3 glm::glm
)

add_subdirectory(src)

