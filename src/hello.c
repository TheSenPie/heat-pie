#include "raylib.h"
#include "rlgl.h"

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

struct st_vector2d;
struct st_packedPositionArray
{
	struct st_vector2d* items;
	unsigned int count;
	unsigned int capacity;
};

struct st_levelData;
struct st_levelsData
{
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

#define st_daShrinkToFit(xs)\
	do {\
		assert( xs.count > 0 );\
		xs.capacity = xs.count;\
		xs.items = realloc( xs.items, xs.capacity * sizeof( *xs.items ) );\
	} while ( 0 )

void st_daUnloadPackedStringArray( struct st_packedStringArray* arr )
{
	assert( arr );

	if ( arr->items != NULL )
	{
		for ( unsigned int i = 0; i < arr->count; ++i )
		{
			free( arr->items[ i ] );
			arr->items[ i ] = NULL;
		}
		free( arr->items );
		arr->items = NULL;
		arr->count = 0;
		arr->capacity = 0;
	}
}

void st_daUnloadPackedPositionArray( struct st_packedPositionArray* arr )
{
	assert( arr );

	if ( arr->items != NULL )

	{
		free( arr->items );
		arr->items = NULL;
		arr->count = 0;
		arr->capacity = 0;
	}
}

struct st_vector2d{
	float x;
	float y;
};

struct st_fileAccess{
	const unsigned char *data;
	const int count;
	int offset;
};

struct st_levelData{
	unsigned char* name;
	struct st_packedPositionArray positions;
};

void st_unloadLevelData( struct st_levelData* data )
{
	assert( data );

	if ( data->name != NULL )
	{
		free( data->name );
		data->name = NULL;

		st_daUnloadPackedPositionArray( &data->positions );
	}
}

void st_daUnloadLevelsData( struct st_levelsData* data )
{
	assert( data );
	if ( data->items != NULL )
	{
		for ( unsigned int i = 0; i < data->count; ++i )
		{
			struct st_levelData* level = &data->items[i];
			st_unloadLevelData( level );
		}
		free( data->items );
		data->items = NULL;
		data->count = 0;
		data->capacity = 0;
	}
}

struct st_playerData{
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
	struct st_levelsData levelsData;
};

struct st_playersData{
	struct st_playerData* items;
	unsigned int count;
	unsigned int capacity;
};

struct st_positionsSSBO{
	struct st_packedPositionArray positions;
	unsigned int ssboHandle;
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

static void st_loadUser( const char* name, const char* filePath, struct st_playerData* playerData )
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

	struct st_levelData currentLevel = {0};
	bool levelStart = false;

	unsigned char* line = NULL;
	unsigned int rows = 0;
	while ( ( line = st_readLine( &userGameplayFile ) ) )
	{
		rows++;

		struct st_packedStringArray cols = st_splitLine( line, ", " );
		assert( cols.count >= 2 && "User csv data has at least two columns" );

		const bool isLevelStatusEvent = strcmp( (const char*)cols.items[0], "[elsc]" ) == 0;
		if (  isLevelStatusEvent && 
			strcmp( (const char*)cols.items[3], "start" ) == 0  &&
			!levelStart )
		{
			levelStart =  true;
			size_t nameSize = strlen( (const char*) cols.items[2] );
			currentLevel.name = malloc( nameSize + 1 ); // name
			strcpy_s( (char*)currentLevel.name, nameSize + 1, (const char*) cols.items[2] );
		}
		else if ( levelStart )
		{
			if ( isLevelStatusEvent && strcmp( (const char*)cols.items[3], "start" ) == 0 )
			{
				fprintf(stdout, "ERR Received level:start in the middle of processing level data for %s \n", currentLevel.name );

				free( currentLevel.name );
				size_t nameSize = strlen( (const char*) cols.items[2] );
				currentLevel.name = malloc( nameSize + 1 ); // name
				strcpy_s( (char*)currentLevel.name, nameSize + 1, (const char*) cols.items[2] );

				st_daUnloadPackedPositionArray( &currentLevel.positions );
			}
			if ( isLevelStatusEvent && strcmp( (const char*)cols.items[3], "complete" ) == 0 )
			{
				levelStart = false;
				st_daAppend( (playerData->levelsData), currentLevel );
				currentLevel.name = NULL;
				currentLevel.positions.items = NULL;
				currentLevel.positions.capacity = 0;
				currentLevel.positions.count = 0;
			}
			if ( strcmp( (const char*)cols.items[0], "[pos]" ) == 0 )
			{
				struct st_vector2d pos = {
					.x = atof( (const char*)cols.items[2] ),
					.y = atof( (const char*)cols.items[3] )
				};

				st_daAppend( currentLevel.positions, pos );
			}
		}

		st_daUnloadPackedStringArray( &cols );

		free( line );
		line = NULL;
	}

	fprintf( stdout, "%s has %u rows\n", name, rows );
	UnloadFileData( data );

	if ( levelStart )
		st_daAppend( (playerData->levelsData), currentLevel );
}

void st_loadUserMetrics( struct st_fileAccess *userMetricsFile, struct st_playersData* out )
{
	// skip first line
	st_readLine( userMetricsFile );

	unsigned char* line = NULL;
	while ( ( line = st_readLine( userMetricsFile ) ) )
	{
		struct st_packedStringArray cols = st_splitLine( line, "," );
		assert( cols.count == 10 && "User metrics csv has ten columns" );

		unsigned char* playerName = cols.items[0];

		struct st_playerData* playerData = NULL;
		for ( unsigned int i = 0; i < out->count; ++i )
			if ( strcmp( (const char*)out->items[i].name, (const char*)playerName ) == 0 )
			{
				playerData = &out->items[i];
				break;
			}
		assert( playerData );

		playerData->relatedness = atof( (const char*) cols.items[1] );
		playerData->competence = atof( (const char*) cols.items[2] );
		playerData->immersion = atof( (const char*) cols.items[3] );
		playerData->fun = atof( (const char*) cols.items[4] );
		playerData->autonomy = atof( (const char*) cols.items[5] );
		playerData->physical = atof( (const char*) cols.items[6] );
		playerData->analytical = atof( (const char*) cols.items[7] );
		playerData->socioemotional = atof( (const char*) cols.items[8] );
		playerData->insight = atof( (const char*) cols.items[9] );
		
		free( line );
		line = NULL;
	}
}

static void st_printUserDataDebug( struct st_playerData playerData )
{
	fprintf(stdout, "----------------------------------------\n");
	fprintf(stdout, "User: %s\n", playerData.name );
	fprintf(stdout, "relatedness: %f, competence: %f, immersion: %f, fun: %f, autonomy: %f\n",
			playerData.relatedness,
			playerData.competence, 
			playerData.immersion,
			playerData.fun,
			playerData.autonomy);
	fprintf(stdout, "physical: %f, analytical: %f, socioemotional: %f, insight: %f\n",
			playerData.physical,
			playerData.analytical,
			playerData.socioemotional,
			playerData.insight);
	for ( unsigned int i = 0; i < playerData.levelsData.count; ++i )
	{
		struct st_levelData levelData = playerData.levelsData.items[i];
		fprintf(stdout, "level: %s with %u position points\n", levelData.name, levelData.positions.count);
	}
	fprintf(stdout, "----------------------------------------\n");
}

static struct st_playersData st_loadCsv()
{
	// csv folder
	static const char *csvDir =  "csv";
	assert( DirectoryExists(csvDir) );

	// load users gameplay data
	static const char *usersDir = "csv/users";
	FilePathList userFiles;
	userFiles = LoadDirectoryFiles( usersDir );

	struct st_playersData playersData = {0};

	for ( unsigned int fileIdx = 0; fileIdx < userFiles.count; ++fileIdx )
	{
		const char* filePath = userFiles.paths[ fileIdx ];
		const char* playerName = GetFileNameWithoutExt( filePath );
		const size_t playerNameSize = strlen( playerName );
		struct st_playerData playerData = {0};
		playerData.name = malloc( playerNameSize + 1 );
		strcpy_s( (char*)playerData.name, playerNameSize + 1, playerName );
		st_loadUser( playerName, filePath, &playerData );
		st_daAppend( playersData, playerData );
	}

	// load player metrics
	static const char *usersMetricsFile = "csv/player_metrics.csv";
	assert( FileExists(usersMetricsFile) );
	int usersMetricsDataSize;
	unsigned char *usersMetricsData;
	usersMetricsData = LoadFileData( usersMetricsFile, &usersMetricsDataSize );

	assert( usersMetricsData && usersMetricsDataSize );
	struct st_fileAccess userMetricsFile = {
		.data = usersMetricsData,
		.count = usersMetricsDataSize,
		.offset = 0u
	};
	st_loadUserMetrics( &userMetricsFile, &playersData );
	UnloadFileData( usersMetricsData );

	UnloadDirectoryFiles(userFiles);

	for ( unsigned int i = 0; i < playersData.count; ++i )
		st_printUserDataDebug( playersData.items[i] );

	return playersData;
}

struct st_pointDataFilter {
	// ranges are inclusive
	struct st_vector2d relatednessRange;
	struct st_vector2d competenceRange;
	struct st_vector2d immersionRange;
	struct st_vector2d funRange;
	struct st_vector2d autonomyRange;
	struct st_vector2d physicalRange;
	struct st_vector2d analyticalRange;
	struct st_vector2d socioemotionalRange;
	struct st_vector2d insightRange;

	const char* const levelName;
};

bool st_testPlayerMotivationAndChallenge( const struct st_playerData* const playerData, struct st_pointDataFilter pointFilter )
{
	return pointFilter.relatednessRange.x <= playerData->relatedness && playerData->relatedness <= pointFilter.relatednessRange.y
	&& pointFilter.competenceRange.x <= playerData->competence && playerData->competence <= pointFilter.competenceRange.y
	&& pointFilter.immersionRange.x <= playerData->immersion && playerData->immersion <= pointFilter.immersionRange.y
	&& pointFilter.funRange.x <= playerData->fun && playerData->fun <= pointFilter.funRange.y
	&& pointFilter.autonomyRange.x <= playerData->autonomy && playerData->autonomy <= pointFilter.autonomyRange.y
	&& pointFilter.physicalRange.x <= playerData->physical && playerData->physical <= pointFilter.physicalRange.y
	&& pointFilter.analyticalRange.x <= playerData->analytical && playerData->analytical <= pointFilter.analyticalRange.y
	&& pointFilter.socioemotionalRange.x <= playerData->socioemotional && playerData->socioemotional <= pointFilter.socioemotionalRange.y
	&& pointFilter.insightRange.x <= playerData->insight && playerData->insight <= pointFilter.insightRange.y;
}

// return SSBO id
struct st_positionsSSBO st_loadPointBuffer( struct st_playersData players, struct st_pointDataFilter pointFilter )
{
	struct st_positionsSSBO res = { 0 };

	for ( unsigned int player_idx = 0; player_idx < players.count; ++player_idx )
	{
		const struct st_playerData* const playerData = &players.items[player_idx];
		const bool hasMotivationAndChallenge = st_testPlayerMotivationAndChallenge( playerData, pointFilter );
		assert( hasMotivationAndChallenge ); // TOOD: remove, only for debug

		if ( hasMotivationAndChallenge )
		{
			const struct st_levelsData* const levelsData = &playerData->levelsData;

			for ( unsigned int lvl_idx = 0; lvl_idx < levelsData->count; ++lvl_idx )
			{
				const struct st_levelData* const levelData = &levelsData->items[lvl_idx];
				if ( strcmp((const char*) levelData->name, pointFilter.levelName) == 0 )
				{
					for ( unsigned int pos_idx = 0; pos_idx < levelData->positions.count; ++pos_idx )
						st_daAppend( res.positions, levelData->positions.items[ pos_idx ] );
					break;
				}
			}
		}
	}
	fprintf( stdout, "loaded %u positions \n", res.positions.count );
	if ( res.positions.count == 0 )
		return res;

	st_daShrinkToFit( res.positions );
	res.ssboHandle = rlLoadShaderBuffer(res.positions.count * sizeof( *res.positions.items ), res.positions.items, RL_DYNAMIC_COPY);
	return res;
}

int main(void)
{
	const unsigned int screenWidth = 800u;
	const unsigned int screenHeight = 640u;

	InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window" );

	SetTargetFPS( 60u );

	struct st_playersData playersData = st_loadCsv();
	struct st_pointDataFilter pointFilter = {
		.relatednessRange = {0.0, 1.0},
		.competenceRange = {0.0, 1.0},
		.immersionRange = {0.0, 1.0},
		.funRange = {0.0, 1.0},
		.autonomyRange = {0.0, 1.0},
		.physicalRange = {0.0, 1.0},
		.analyticalRange = {0.0, 1.0},
		.socioemotionalRange = {0.0, 1.0},
		.insightRange = {0.0, 1.0},
		.levelName = "aries_test.tscn"
	};

	struct st_positionsSSBO positionsSSBO = st_loadPointBuffer( playersData, pointFilter );

	// Load compute shader and process points to write to render buffer
	char* heatmapLogicCode = LoadFileText( "resources/shaders/glsl430/heatmap.glsl" );
	unsigned int heatmapLogicShader = rlCompileShader( heatmapLogicCode, RL_COMPUTE_SHADER );
	unsigned int heatmapLogicProgram = rlLoadComputeShaderProgram( heatmapLogicShader );
	UnloadFileText( heatmapLogicCode );

	// Load fragment shader for rendering the points
	Shader heatmapRenderShader = LoadShader( NULL, "resources/shaders/glsl430/heatmap_render.glsl" );

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
