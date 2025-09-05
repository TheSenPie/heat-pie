cmake_minimum_required( VERSION 3.30 )

project( PlayerPositionPlot LANGUAGES C )

set(CMAKE_C_VISIBILITY_PRESET hidden)

add_library( raylib SHARED IMPORTED )

set_target_properties( raylib 
  PROPERTIES LINKER_LANGUAGE C
  IMPORTED_LOCATION raylib.lib 
  IMPORTED_IMPLIB raylib.lib ) 

add_executable( ${PROJECT_NAME} src/heatpie.c )

add_library( StringTheoryPlugin SHARED
  plugins/string_theory.h
  plugins/string_theory.c
  plugins/string_theory-data_loader.c
  plugins/string_theory-gui.c
)

target_include_directories( StringTheoryPlugin PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}"

  raylib-5.5_win64_msvc16/include/
)

include(GenerateExportHeader)
generate_export_header(StringTheoryPlugin)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address )
  target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address )
endif()

target_include_directories( ${PROJECT_NAME} PUBLIC raylib-5.5_win64_msvc16/include/ )

if(WIN32)
  target_link_libraries( StringTheoryPlugin PRIVATE raylib winmm gdi32 )
  target_link_libraries( ${PROJECT_NAME}
  PUBLIC
    raylib winmm gdi32 
  PRIVATE
    StringTheoryPlugin
  )
else()
  target_link_libraries( StringTheoryPlugin PRIVATE raylib )
  target_link_libraries( ${PROJECT_NAME}
  PUBLIC
    raylib
  PRIVATE
    StringTheoryPlugin
  )
endif()

add_definitions(-DPLATFORM_DESKTOP)

