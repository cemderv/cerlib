cerlib_ensure_dependency_added(ImGui)

cerlib_Log("Including ImGui as part of cerlib")

project(cerlib_imgui LANGUAGES CXX)

add_library(cerlib_imgui STATIC)