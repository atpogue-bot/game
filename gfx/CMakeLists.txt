add_library(gfx)

target_include_directories(
  gfx
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  gfx
  PUBLIC sys glm::glm
)

add_subdirectory(src)
