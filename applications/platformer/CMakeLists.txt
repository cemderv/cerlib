set(GameSourceFiles
  src/Animation.cpp
  src/Animation.hpp
  src/Enemy.cpp
  src/Enemy.hpp
  src/Gem.cpp
  src/Gem.hpp
  src/Level.cpp
  src/Level.hpp
  src/Main.cpp
  src/PlatformerGame.cpp
  src/PlatformerGame.hpp
  src/Player.cpp
  src/Player.hpp
  src/Tile.hpp
)

cerlib_add_executable(
  NAME Platformer
  COMPANY cemdervis
  VERSION 0.0.1
  FILES ${GameSourceFiles}
)
