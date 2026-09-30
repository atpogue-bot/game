add_subdirectory (content)
add_subdirectory (biome)
target_sources (game PRIVATE command.cc command-buffer.cc flocking.cc world.cc)
