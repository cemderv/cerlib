set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

include(CTest)

add_executable(cerlibTests)

target_sources(cerlibTests PRIVATE
  src/Vector2Tests.cpp
  src/Vector3Tests.cpp
  src/Vector4Tests.cpp
  src/MatrixTests.cpp
  src/LexerTests.cpp
  src/MathTests.cpp
  src/ParserTests.cpp
  src/ShaderWriterTests.cpp
  src/ObjectTests.cpp
  src/ColorTests.cpp
  src/FormattingTests.cpp
)

if (CERLIB_ENABLE_RENDERING_TESTS)
  target_sources(cerlibTests PRIVATE
    src/SpriteRenderingTests.cpp
    src/RenderingTestHelper.hpp
    src/RenderingTestHelper.cpp
  )
endif ()

enable_default_cpp_flags(cerlibTests)

target_link_libraries(cerlibTests PRIVATE
  cerlib
  snitch::snitch
)

target_include_directories(cerlibTests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/
)

target_compile_definitions(cerlibTests PRIVATE
  "-DREFERENCE_IMAGES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/reference_images\""
  "-DTEST_ASSETS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../docs/assets\""
)

set_target_properties(cerlibTests PROPERTIES FOLDER "cerlib")
set_target_properties(snitch PROPERTIES FOLDER "Dependencies")

