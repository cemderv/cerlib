function(cerlib_file_embed_generate source_file output_filename_cpp output_filename_hpp c_name)
  message(STATUS "[cerlib embed] Generating ${output_filename_hpp}")
  message(STATUS "[cerlib embed] Generating ${output_filename_cpp}")

  file(READ ${source_file} content HEX)

  # Separate into individual bytes.
  string(REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" SEPARATED_HEX ${content})

  set(outputCpp "")

  set(counter 0)
  foreach (hex IN LISTS SEPARATED_HEX)
    string(APPEND outputCpp "std::byte(0x${hex}),")
    MATH(EXPR counter "${counter}+1")
    if (counter GREATER 16)
      string(APPEND outputCpp "\n  ")
      set(counter 0)
    endif ()
  endforeach ()

  get_filename_component(relativeHppFilename ${output_filename_hpp} NAME)

  # .cpp file contents
  set(outputCpp "#include \"${relativeHppFilename}\"

const std::byte ${c_name}_data[] = {
    ${outputCpp}
}\;

const uint32_t ${c_name}_size = sizeof(${c_name}_data)\;
")

  # .hpp file contents
  set(outputHpp "#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string_view>

extern const std::byte ${c_name}_data[]\;
extern const uint32_t ${c_name}_size\;

static inline auto ${c_name}_span() -> std::span<const std::byte> {
  return std::span<const std::byte>(${c_name}_data, ${c_name}_size)\;
}

static inline auto ${c_name}_string_view() -> std::string_view {
  return std::string_view(reinterpret_cast<const char*>(${c_name}_data), ${c_name}_size)\;
}
")

  get_filename_component(output_dir ${output_filename_hpp} DIRECTORY)

  if (NOT EXISTS ${output_dir})
    file(MAKE_DIRECTORY ${output_dir})
  endif ()

  file(WRITE ${output_filename_cpp} ${outputCpp})
  file(WRITE ${output_filename_hpp} ${outputHpp})
endfunction()


function(cerlib_embed_file target_name absolute_src_filename)
  set(output_dir ${CMAKE_BINARY_DIR}/embedded_files/${target_name})

  if (NOT EXISTS ${output_dir})
    file(MAKE_DIRECTORY ${output_dir})
  endif ()

  message(STATUS "[cerlib embed] Embedding file ${absolute_src_filename}")

  target_include_directories(${target_name} PRIVATE ${output_dir})

  get_filename_component(short_filename ${absolute_src_filename} NAME)

  set(output_filename_hpp "${output_dir}/${short_filename}.hpp")
  set(output_filename_cpp "${output_dir}/${short_filename}.cpp")
  string(MAKE_C_IDENTIFIER ${short_filename} c_name)

  if (NOT EXISTS ${output_filename_cpp} OR NOT EXISTS ${output_filename_hpp})
    cerlib_file_embed_generate(${absolute_src_filename} ${output_filename_cpp} ${output_filename_hpp} ${c_name})
  endif ()

  target_sources(${target_name} PRIVATE ${output_filename_hpp} ${output_filename_cpp})

  set(script_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

  add_custom_command(
    OUTPUT ${output_filename_hpp} ${output_filename_cpp}
    COMMAND ${CMAKE_COMMAND}
    -DRUN_FILE_EMBED_GENERATE=ON
    -DSRC_FILENAME=${absolute_src_filename}
    -DOUTPUT_FILENAME_CPP=${output_filename_cpp}
    -DOUTPUT_FILENAME_HPP=${output_filename_hpp}
    -DC_NAME=${c_name}
    -P ${script_dir}/bin2header.cmake
    MAIN_DEPENDENCY ${absolute_src_filename}
    COMMENT "${absolute_src_filename} -> ${output_filename_cpp}"
  )

  source_group("Embedded" FILES ${absolute_src_filename})
  source_group("Embedded\\Generated" FILES ${output_filename_hpp} ${output_filename_cpp})
endfunction()


if (RUN_FILE_EMBED_GENERATE)
  cerlib_file_embed_generate(${SRC_FILENAME} ${OUTPUT_FILENAME_CPP} ${OUTPUT_FILENAME_HPP} ${C_NAME})
endif ()
