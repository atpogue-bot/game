find_package(Lua 5.4 REQUIRED)

add_library(app)

target_include_directories(
  app
  PRIVATE ${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  app
  PRIVATE ${LUA_LIBRARIES} gfx
  PUBLIC game
)

add_subdirectory(src)
