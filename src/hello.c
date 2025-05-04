#include "raylib.h"
#include "assert.h"

#include <stdio.h>

static unsigned int st_readLine( const unsigned char *data, const int offset, const int size )
{
	unsigned int idx = offset;
	while( idx < size )
	{
		if ( '\n' == data[idx] )
		{
			idx++; // consume newline charachter
			fprintf( stdout, "\n" );
			break;
		}
		fprintf( stdout, "%c", data[idx] );
		idx++;
	}
	return idx;
}

static void st_loadCsv()
{
	// csv folder
	static const char *csvDir =  "csv";
	assert( DirectoryExists(csvDir) );

	// load player metrics
	static const char *usersMetricsFile = "csv/player_metrics.csv";
	assert( FileExists(usersMetricsFile) );
	int usersMetricsDataSize;
	unsigned char *usersMetricsData;
	usersMetricsData = LoadFileData( usersMetricsFile, &usersMetricsDataSize );

	// process line by line
	assert( usersMetricsData && usersMetricsDataSize );
	unsigned int usersMetricsDataOffset = 0u;
	while ( usersMetricsDataOffset < usersMetricsDataSize ) {
		usersMetricsDataOffset = st_readLine( usersMetricsData, usersMetricsDataOffset, usersMetricsDataSize );
	}

	// load users gameplay data
	static const char *usersDir = "csv/users";
	FilePathList userFiles;
	userFiles = LoadDirectoryFiles( usersDir );
}

int main(void)
{
	const unsigned int screenWidth = 800u;
	const unsigned int screenHeight = 450u;

	InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window" );

	SetTargetFPS( 60u );

	st_loadCsv();

	// Main game loop
	while ( !WindowShouldClose() )
	{
		BeginDrawing();
			
			ClearBackground( RAYWHITE );

			DrawText( "Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY );

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
