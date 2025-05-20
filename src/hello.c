#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "raylib.h"

#if defined(PLATFORM_DESKTOP) || defined(PLATFORM_DESKTOP_SDL)
    #if defined(GRAPHICS_API_OPENGL_ES2)
        #include "glad_gles2.h"       // Required for: OpenGL functionality 
        #define glGenVertexArrays glGenVertexArraysOES
        #define glBindVertexArray glBindVertexArrayOES
        #define glDeleteVertexArrays glDeleteVertexArraysOES
        #define GLSL_VERSION            100
    #else
        #if defined(__APPLE__)
            #define GL_SILENCE_DEPRECATION // Silence Opengl API deprecation warnings 
            #include <OpenGL/gl3.h>     // OpenGL 3 library for OSX
            #include <OpenGL/gl3ext.h>  // OpenGL 3 extensions library for OSX
        #else
            #include "glad.h"       // Required for: OpenGL functionality 
        #endif
        #define GLSL_VERSION            330
    #endif
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include "rlgl.h"
#include "raymath.h"

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

const struct st_vector2d st_VECTOR2D_ZERO = { .x = .0f, .y = .0f };

struct st_aabb{
	struct st_vector2d topLeft;
	struct st_vector2d bottomRight;
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

static void st_printAABBDebug(const struct st_aabb bounds)
{
	fprintf(stdout, "top-left: (%f, %f)\nbottom-right: (%f, %f)\nwidth-height: (%f, %f)\n", 
		bounds.topLeft.x, bounds.topLeft.y,
		bounds.bottomRight.x, bounds.bottomRight.y,
		fabsf(bounds.topLeft.x - bounds.bottomRight.x), fabsf(bounds.topLeft.y  - bounds.bottomRight.y));
}

static struct st_playersData st_loadCsv()
{
	// csv folder
	static const char *csvDir = "csv";
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

void st_findBounds( const struct st_packedPositionArray points, struct st_aabb* bounds )
{
	if ( points.count == 0u )
	{
		bounds->topLeft = st_VECTOR2D_ZERO; 
		bounds->bottomRight = st_VECTOR2D_ZERO;
		return;
	}

	bounds->topLeft =  points.items[0];
	bounds->bottomRight =  points.items[0];

	for ( int point_idx = 1; point_idx < points.count; ++point_idx )
	{
		const struct st_vector2d point = points.items[ point_idx ];
		if ( bounds->topLeft.x > point.x ) bounds->topLeft.x = point.x;
		if ( bounds->topLeft.y > point.y ) bounds->topLeft.y = point.y; // y points down
		if ( bounds->bottomRight.x < point.x ) bounds->bottomRight.x = point.x;
		if ( bounds->bottomRight.y < point.y ) bounds->bottomRight.y = point.y;
	}
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


// Unload shader storage buffer object (SSBO)
void rlUnloadUniformBuffer(unsigned int uboId)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glDeleteBuffers(1, &ssboId);
#else
    TRACELOG(RL_LOG_WARNING, "UBO: UBO not enabled. Define GRAPHICS_API_OPENGL_43");
#endif

}

#define HEATMAP_WIDTH 2048 
#define HEATMAP_HEIGHT 2048

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
		.levelName = "cancer_test.tscn"
	};

	struct st_positionsSSBO positionsSSBO = st_loadPointBuffer( playersData, pointFilter );
	struct st_aabb data_bounds = {0};
	st_findBounds( positionsSSBO.positions, &data_bounds );
	st_printAABBDebug( data_bounds );

	// Load compute shader and process points to write to render buffer
	char* heatmapLogicCode = LoadFileText( "resources/shaders/glsl430/heatmap.glsl" );
	unsigned int heatmapLogicShader = rlCompileShader( heatmapLogicCode, RL_COMPUTE_SHADER );
	unsigned int heatmapLogicProgram = rlLoadComputeShaderProgram( heatmapLogicShader );
	UnloadFileText( heatmapLogicCode );

	// query limitations
	// -----------------
	int max_compute_work_group_count[3];
	int max_compute_work_group_size[3];
	int max_compute_work_group_invocations;

	for (int idx = 0; idx < 3; idx++) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
	}
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);

	puts( "OpenGL Limitations: ");
	fprintf( stdout, "maximum number of work groups in X dimension %u\n", max_compute_work_group_count[0] );
	fprintf( stdout, "maximum number of work groups in Y dimension %u\n", max_compute_work_group_count[1] );
	fprintf( stdout, "maximum number of work groups in Z dimension %u\n", max_compute_work_group_count[2] );

	fprintf( stdout, "maximum size of a work group in X dimension %u\n", max_compute_work_group_size[0] );
	fprintf( stdout, "maximum size of a work group in Y dimension %u\n", max_compute_work_group_size[1] );
	fprintf( stdout, "maximum size of a work group in Z dimension %u\n", max_compute_work_group_size[2] );

	fprintf( stdout, "Number of invocations in a single local work group that may be dispatched to a compute shader %u", max_compute_work_group_invocations);

	// Load fragment shader for rendering the points
	Shader heatmapRenderShader = LoadShader( NULL, "resources/shaders/glsl430/heatmap_render.glsl" );
	
	unsigned int heatmapTex;
	glGenTextures(1, &heatmapTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, heatmapTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// specify two-dimensional heatmapTex image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, HEATMAP_WIDTH, HEATMAP_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
	/*void glTexImage2D(GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void * data);*/

	Texture rlHeatmapTex = {
		.id = heatmapTex,
		.width = HEATMAP_WIDTH, .height = HEATMAP_HEIGHT,
		.mipmaps = 0,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 
	};

	// Create the uniform buffer for AABB bounds
	unsigned int boundsUBO = rlLoadShaderBuffer(sizeof(struct st_aabb), &data_bounds, RL_DYNAMIC_COPY);

	rlEnableShader( heatmapLogicProgram );
	glBindBufferBase( GL_UNIFORM_BUFFER, 0, boundsUBO );
	rlBindShaderBuffer( positionsSSBO.ssboHandle, 1 );
	rlBindImageTexture( heatmapTex, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32, false );
	rlComputeShaderDispatch( ( positionsSSBO.positions.count + 128 - 1 ) / 128, 1, 1 );
	glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
	rlDisableShader();

	Camera2D camera = { 0 };
	camera.zoom = 1.0f;

	// Main game loop
	while ( !WindowShouldClose() )
	{
		// Translate based on mouse right click
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			Vector2 delta = GetMouseDelta();
			delta = Vector2Scale(delta, -1.0f/camera.zoom);
			camera.target = Vector2Add(camera.target, delta);
		}

		// Zoom based on mouse wheel
		float wheel = GetMouseWheelMove();
		if (wheel != 0)
		{
			// Get the world point that is under the mouse
			Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

			// Set the offset to where the mouse is
			camera.offset = GetMousePosition();

			// Set the target to match, so that the camera maps the world space point
			// under the cursor to the screen space point under the cursor at any zoom
			camera.target = mouseWorldPos;

			// Zoom increment
			// Uses log scaling to provide consistent zoom speed
			float scale = 0.2f*wheel;
			camera.zoom = Clamp(expf(logf(camera.zoom)+scale), 0.125f, 64.0f);
		}


		BeginDrawing();
			
			ClearBackground( RAYWHITE );
			BeginMode2D( camera );
				BeginShaderMode( heatmapRenderShader );
				DrawTexture( rlHeatmapTex, 0, 0, WHITE );
				EndShaderMode();

				DrawText( "Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY );
			EndMode2D();

		EndDrawing();
	}

	// Unload resources
	rlUnloadShaderBuffer( positionsSSBO.ssboHandle );
	positionsSSBO.ssboHandle = 0u;
	st_daUnloadPackedPositionArray( &positionsSSBO.positions );

	rlUnloadUniformBuffer( boundsUBO );

	rlUnloadShaderProgram( heatmapLogicProgram );

	UnloadTexture( rlHeatmapTex );
	heatmapTex = 0u;
	UnloadShader( heatmapRenderShader );

	CloseWindow();

	return 0;
}
