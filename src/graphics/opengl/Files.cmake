set(opengl_files
  OpenGLBuffer.cpp
  OpenGLBuffer.hpp
  OpenGLGraphicsDevice.cpp
  OpenGLGraphicsDevice.hpp
  OpenGLImage.cpp
  OpenGLImage.hpp
  OpenGLPrerequisites.cpp
  OpenGLPrerequisites.hpp
  OpenGLPrivateShader.cpp
  OpenGLPrivateShader.hpp
  OpenGLShaderProgram.cpp
  OpenGLShaderProgram.hpp
  OpenGLSpriteBatch.cpp
  OpenGLSpriteBatch.hpp
  OpenGLUserShader.cpp
  OpenGLUserShader.hpp
  OpenGLVao.cpp
  OpenGLVao.hpp
  OpenGLWindow.cpp
  OpenGLWindow.hpp
  khrplatform.h
)

if (EMSCRIPTEN OR ANDROID OR IOS)
  list(APPEND opengl_files
    gles/glad.h
    gles/glad.cpp
  )
  list(APPEND opengl_include_dirs gles)
  list(APPEND opengl_compile_defs -DCERLIB_GFX_IS_GLES)
else ()
  list(APPEND opengl_files
    desktop/glad.h
    desktop/glad.cpp
  )
  list(APPEND opengl_include_dirs desktop)
endif ()
