cmake_minimum_required( VERSION 3.30 )

project( PlayerPositionPlot LANGUAGES C )

add_library( raylib SHARED IMPORTED )

set_target_properties( raylib 
	PROPERTIES LINKER_LANGUAGE C
	IMPORTED_LOCATION raylib.dll 
	IMPORTED_IMPLIB raylibdll.lib ) 

add_executable( ${PROJECT_NAME} src/hello.c )

target_include_directories( ${PROJECT_NAME} PUBLIC raylib-5.5_win64_msvc16/include/ )

target_link_libraries( ${PROJECT_NAME} raylib )

#add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy 
#	"${PROJECT_SOURCE_DIR}/raylib-5.5_win64_msvc16/lib/raylib.dll"
#	"${PROJECT_SOURCE_DIR}/raylib-5.5_win64_msvc16/lib/raylib.lib"
#	"${PROJECT_SOURCE_DIR}/raylib-5.5_win64_msvc16/lib/raylibdll.lib"
#	$<TARGET_FILE_DIR:${PROJECT_NAME}>
#	BYPRODUCTS 
#	"raylib.dll"
#        "raylib.lib"
#        "raylibdll.lib"
#	COMMENT "Copying raylib libs to output directory" )

