#include "raylib.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Array types
struct st_packedStringArray
{
	unsigned char** items;
	unsigned int count;
	unsigned int capacity;
};

struct st_numbers
{
	unsigned int* items;
	unsigned int count;
	unsigned int capacity;
};

struct st_levelData;
struct st_levels{
	struct st_levelData* items;
	unsigned int count;
	unsigned int capacity;
};

#define st_daAppend(xs, x)\
	do {\
	if ( xs.count >= xs.capacity ) {\
		if ( xs.capacity == 0u ) xs.capacity = 16u;\
		else xs.capacity *= 2u;\
		xs.items = realloc( xs.items, xs.capacity * sizeof( *xs.items ) );\
	}\
	xs.items[ xs.count++ ]  = x;\
	} while ( 0 )

void st_daUnloadPackedStringArray( struct st_packedStringArray arr )
{
	if ( arr.items != NULL )
	{
		for ( unsigned int i = 0; i < arr.count; ++i )
		{
			free( arr.items[ i ] );
			arr.items[ i ] = NULL;
		}
		free( arr.items );
		arr.items = NULL;
		arr.count = 0;
		arr.capacity = 0;
	}
}

struct st_vector2d{
	double x;
	double y;
};

struct st_fileAccess{
	const unsigned char *data;
	const int count;
	int offset;
};

struct st_levelData{
	unsigned char* name;
	struct st_vector2d positions[];
};

struct st_player{
	unsigned char* name;
	float relatedness;
	float competence;
	float immersion;
	float fun;
	float autonomy;
	float physical;
	float analytical;
	float socioemotional;
	float insight;
	struct st_levelData* level;
};

static struct st_packedStringArray st_splitLine( const unsigned char* line, const char* delimiter ) 
{
	assert( line && delimiter );
	const size_t delimiterSize = strlen( delimiter );
	assert( delimiterSize > 0 );

	struct st_packedStringArray chunks = {0};

	const unsigned char* str = line;
	const unsigned char* end = (unsigned char*) strstr( (const char*)str, delimiter );
	while ( end )
	{
		size_t chunkSize = end - str + 1u; 
		unsigned char* chunk = malloc( chunkSize );
		strncpy_s( (char*)chunk, chunkSize, (const char*)str, chunkSize - 1);
		chunk[ chunkSize - 1 ] = '\0';

		st_daAppend( chunks, chunk );

		str = end + delimiterSize;
		end = (unsigned char*) strstr( (const char*)str, delimiter );
	}

	// Read last chunk
	if ( *str != '\0' )
	{
		size_t chunkSize = strlen( (const char*)line ) - (str - line) + 1u; 
		unsigned char* chunk = malloc( chunkSize );
		strncpy_s( (char*)chunk, chunkSize, (const char*)str, chunkSize - 1);
		chunk[ chunkSize - 1 ] = '\0';

		st_daAppend( chunks, chunk );
	}

	return chunks;
}

static unsigned char* st_readLine( struct st_fileAccess *file )
{
	static char buffer[500] = {0};

	unsigned int idx = file->offset;
	unsigned int lineSize = 0u;

	while( idx < file->count )
	{
		unsigned char c = file->data[ idx ];
		if ( '\n' == c || '\0' == c )
		{
			file->offset = idx + 1u;

			buffer[ lineSize ] = 0u; 
			lineSize++;
			unsigned char* line = malloc( sizeof(unsigned char) * lineSize );
			strcpy_s( (char*)line, lineSize, buffer );

			return line;
		}
		else if ( '\r' != c )
		{
			buffer[ lineSize ] = c; 
			lineSize++;
		}
		idx++;
	}
	file->offset = 0;
	return NULL;
}

static void st_loadUser( const char* name, const char* filePath )
{
	fprintf( stdout, "Load user %s with file %s\n", name, filePath );
	unsigned int dataSize = 0u;
	unsigned char* data = LoadFileData( filePath, (int*) &dataSize );

	assert( data && dataSize );
	struct st_fileAccess userGameplayFile = {
		.data = data,
		.count = dataSize,
		.offset = 0u
	};

	unsigned char* line = NULL;
	unsigned int rows = 0;
	while ( ( line = st_readLine( &userGameplayFile ) ) ) {
		rows++;

		struct st_packedStringArray cols = st_splitLine( line, ", " );
		assert( cols.count >= 2 && "User csv data has at least two columns" );

		if ( strcmp( (const char*)cols.items[0], "[elsc]" ) == 0 )
		{
			cols.items[2]; // name
			cols.items[3]; // start/complete

		}
		else if ( strcmp( (const char*)cols.items[0], "[pos]" ) == 0 )
		{
			double x = atof( (const char*)cols.items[2] ); //x
			double y = atof( (const char*)cols.items[3] ); //y
			printf(" %f %f\n", x, y);
		}
		else if ( strcmp( (const char*)cols.items[0], "[pd]" ) == 0 )
		{
			// reset count or smth
		}

		st_daUnloadPackedStringArray( cols );

		free( line );
		line = NULL;
	}
	fprintf( stdout, "%s has %u rows\n", name, rows );

	UnloadFileData( data );
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
	struct st_fileAccess userMetricsFile = {
		.data = usersMetricsData,
		.count = usersMetricsDataSize,
		.offset = 0u
	};
	unsigned char* line = NULL;
	while ( ( line = st_readLine( &userMetricsFile ) ) )
	{
		fprintf(stdout, "%s", line);

		free( line );
		line = NULL;
	}
	UnloadFileData( usersMetricsData );

	// load users gameplay data
	static const char *usersDir = "csv/users";
	FilePathList userFiles;
	userFiles = LoadDirectoryFiles( usersDir );

	for ( unsigned int fileIdx = 0; fileIdx < userFiles.count; ++fileIdx )
	{
		const char* filePath = userFiles.paths[ fileIdx ];
		st_loadUser( GetFileNameWithoutExt( filePath ), filePath  );
	}

	UnloadDirectoryFiles(userFiles);
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
