#ifndef STRING_THEORY_H
#define STRING_THEORY_H

#include "stringtheoryplugin_export.h"

#include "raymath.h"

#include <assert.h>

#include <stdbool.h>
#include <stdlib.h>

// Array types
struct st_numbers;
struct st_packedStringArray;
struct st_packedPositionArray;
struct st_levelsData;
struct st_playersData;

// Gameplay specific types
struct st_levelData;
struct st_playerData;
struct st_pointDataFilter;

// Array destructors
void st_daUnloadNumbers             ( struct st_numbers *arr );
void st_daUnloadPackedStringArray   ( struct st_packedStringArray *arr );
void st_daUnloadPackedPositionArray ( struct st_packedPositionArray *arr );
/* missing st_daUnloadPositionsArray */
void st_daUnloadLevelsData          ( struct st_levelsData* data );
/* missing st_daUnloadPlayersData */

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

struct st_packedPositionArray
{
  struct Vector2* items;
  unsigned int count;
  unsigned int capacity;
};

struct st_playersData{
  struct st_playerData* items;
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

//#define st_daShrinkToFit(xs)\
//  do {\
//    assert( xs.count > 0 );\
//    xs.capacity = xs.count;\
//    xs.items = realloc( xs.items, xs.capacity * sizeof( *xs.items ) );\
//  } while ( 0 )

struct st_levelsData
{
  struct st_levelData* items;
  unsigned int count;
  unsigned int capacity;
};

struct st_levelData{
  unsigned char* name;
  struct st_packedPositionArray positions;
  struct st_numbers successfulJumps; // from detach to successful attach
  int successfulRun;
};

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

struct st_pointDataFilter {
  // ranges are inclusive
  Vector2 relatednessRange;
  Vector2 competenceRange;
  Vector2 immersionRange;
  Vector2 funRange;
  Vector2 autonomyRange;
  Vector2 physicalRange;
  Vector2 analyticalRange;
  Vector2 socioemotionalRange;
  Vector2 insightRange;

  const char* levelName;

  bool successfulJumpsOnly;
  bool successfulRunOnly;

  int player; // -1 - none, 0 - all, n - specific user
};

STRINGTHEORYPLUGIN_EXPORT void hp_fetchPoints         ( struct Vector2 **points, unsigned int *pointsCount );
STRINGTHEORYPLUGIN_EXPORT bool hp_heatmapShouldReload ();

STRINGTHEORYPLUGIN_EXPORT void hp_initPlugin    ();
STRINGTHEORYPLUGIN_EXPORT void hp_cleanupPlugin ();

void                      st_initGUI        ();
void                      st_cleanupGUI     ();
struct st_pointDataFilter st_getPointsFilter();

void                        st_initDataLoader();
void                        st_cleanupDataLoader();

//struct st_positionsSSBO{
//  struct st_packedPositionArray positions;
//  unsigned int ssboHandle;
//};
//
//static struct st_packedStringArray st_splitLine( const unsigned char* line, const char* delimiter ) 
//{
//  assert( line && delimiter );
//  const size_t delimiterSize = strlen( delimiter );
//  assert( delimiterSize > 0 );
//
//  struct st_packedStringArray chunks = {0};
//
//  const unsigned char* str = line;
//  const unsigned char* end = (unsigned char*) strstr( (const char*)str, delimiter );
//  while ( end )
//  {
//    size_t chunkSize = end - str + 1u; 
//    unsigned char* chunk = malloc( chunkSize );
//    strncpy_s( (char*)chunk, chunkSize, (const char*)str, chunkSize - 1);
//    chunk[ chunkSize - 1 ] = '\0';
//
//    st_daAppend( chunks, chunk );
//
//    str = end + delimiterSize;
//    end = (unsigned char*) strstr( (const char*)str, delimiter );
//  }
//
//  // Read last chunk
//  if ( *str != '\0' )
//  {
//    size_t chunkSize = strlen( (const char*)line ) - (str - line) + 1u; 
//    unsigned char* chunk = malloc( chunkSize );
//    strncpy_s( (char*)chunk, chunkSize, (const char*)str, chunkSize - 1);
//    chunk[ chunkSize - 1 ] = '\0';
//
//    st_daAppend( chunks, chunk );
//  }
//
//  return chunks;
//}
//
//static unsigned char* st_readLine( struct st_fileAccess *file )
//{
//  static char buffer[500] = {0};
//
//  unsigned int idx = file->offset;
//  unsigned int lineSize = 0u;
//
//  while( idx < file->count )
//  {
//    unsigned char c = file->data[ idx ];
//    if ( '\n' == c || '\0' == c )
//    {
//      file->offset = idx + 1u;
//
//      buffer[ lineSize ] = 0u; 
//      lineSize++;
//      unsigned char* line = malloc( sizeof(unsigned char) * lineSize );
//      strcpy_s( (char*)line, lineSize, buffer );
//
//      return line;
//    }
//    else if ( '\r' != c )
//    {
//      buffer[ lineSize ] = c; 
//      lineSize++;
//    }
//    idx++;
//  }
//  file->offset = 0;
//  return NULL;
//}
//
//static void st_loadUser( const char* name, const char* filePath, struct st_playerData* playerData )
//{
//  fprintf( stdout, "Load user %s with file %s\n", name, filePath );
//  unsigned int dataSize = 0u;
//  unsigned char* data = LoadFileData( filePath, (int*) &dataSize );
//
//  assert( data && dataSize );
//  struct st_fileAccess userGameplayFile = {
//    .data = data,
//    .count = dataSize,
//    .offset = 0u
//  };
//
//  struct st_levelData currentLevel = {0};
//  bool levelStart = false;
//  int detached = -1; // no detach
//  bool player_death = false;
//  int successfulRun = 0;
//
//  unsigned char* line = NULL;
//  unsigned int rows = 0;
//  while ( ( line = st_readLine( &userGameplayFile ) ) )
//  {
//    rows++;
//
//    struct st_packedStringArray cols = st_splitLine( line, ", " );
//    assert( cols.count >= 2 && "User csv data has at least two columns" );
//
//    const bool isLevelStatusEvent = strcmp( (const char*)cols.items[0], "[elsc]" ) == 0;
//    if (  isLevelStatusEvent && 
//      strcmp( (const char*)cols.items[3], "start" ) == 0  &&
//      !levelStart )
//    {
//      levelStart =  true;
//      size_t nameSize = strlen( (const char*) cols.items[2] );
//      currentLevel.name = malloc( nameSize + 1 ); // name
//      strcpy_s( (char*)currentLevel.name, nameSize + 1, (const char*) cols.items[2] );
//      currentLevel.successfulRun = -1;
//
//      detached = 0;
//      player_death = false;
//    }
//    else if( levelStart )
//    {
//      if( isLevelStatusEvent && strcmp( (const char*)cols.items[3], "start" ) == 0 )
//      {
//        fprintf(stdout, "ERR Received level:start in the middle of processing level data for %s \n", currentLevel.name );
//
//        free( currentLevel.name );
//        size_t nameSize = strlen( (const char*) cols.items[2] );
//        currentLevel.name = malloc( nameSize + 1 ); // name
//        strcpy_s( (char*)currentLevel.name, nameSize + 1, (const char*) cols.items[2] );
//        currentLevel.successfulRun = -1;
//
//        st_daUnloadPackedPositionArray( &currentLevel.positions );
//
//        detached = 0;
//        player_death = false;
//      }
//      if( isLevelStatusEvent && strcmp( (const char*)cols.items[3], "complete" ) == 0 )
//      {
//        assert(successfulRun >= 0);
//        currentLevel.successfulRun = successfulRun;
//
//        levelStart = false;
//        st_daAppend( (playerData->levelsData), currentLevel );
//        currentLevel.name = NULL;
//        currentLevel.positions.items = NULL;
//        currentLevel.positions.capacity = 0;
//        currentLevel.positions.count = 0;
//
//        currentLevel.successfulJumps.items = NULL;
//        currentLevel.successfulJumps.capacity = 0;
//        currentLevel.successfulJumps.count = 0;
//      }
//      if( strcmp( (const char*)cols.items[0], "[pos]" ) == 0 )
//      {
//        struct st_vec2 pos = {
//          .x = atof( (const char*)cols.items[2] ),
//          .y = atof( (const char*)cols.items[3] )
//        };
//
//        st_daAppend( currentLevel.positions, pos );
//
//
//        if( player_death )
//        {
//          // hack because spawn location is different after death
//          Vector2 origin = { 0 };
//          switch( currentLevel.name[0] )
//          {
//            case 'a':
//              origin.x = 256.0f; origin.y = 512.0f;
//              break;
//            case 't':
//              origin.x = 768.0f; origin.y = 384.0f;
//              break;
//            case 'g':
//              origin.x = 2816.0f; origin.y = 1920.0f;
//              break;
//            case 'c':
//              origin.x = 576.0f; origin.y = 384.0f;
//              break;
//            case 'l':
//              origin.x = 539.0f; origin.y = 359.0f;
//              break;
//            case 'v':
//              origin.x = 320.0f; origin.y = 0.0f;
//              break;
//          }
//          if (Vector2Distance((Vector2){pos.x, pos.y}, origin) < 23.0f )
//          { // probably restarted
//            detached = currentLevel.positions.count - 1;
//            successfulRun = currentLevel.positions.count - 1;
//            player_death = false;
//          }
//        }
//      }
//      if( strcmp( (const char*)cols.items[0], "[sad]") == 0 )
//      {
//        if( strcmp( (const char*)cols.items[2], "attach" ) == 0 )
//        {
//          if( detached == -1 )
//          {
//            fprintf( stderr, "ERR Received attached while being attached %s at %s\n",
//                currentLevel.name,
//                cols.items[1]);
//            exit(0);
//          }
//          st_daAppend( currentLevel.successfulJumps, detached );
//          st_daAppend( currentLevel.successfulJumps, currentLevel.positions.count );
//          detached = -1;
//        }
//
//        if( strcmp( (const char*)cols.items[2], "detach" ) == 0 )
//        {
//          if ( detached >= 0 ) {
//            fprintf( stderr, "ERR Received detached while being detached %s at %s\n",
//                currentLevel.name,
//                cols.items[1]);
//            exit(0);
//          }
//          detached = currentLevel.positions.count;
//        }
//      }
//      if( strcmp( (const char*)cols.items[0], "[pd]") == 0 )
//      {
//        player_death = true;
//      }
//    }
//
//    st_daUnloadPackedStringArray( &cols );
//
//    free( line );
//    line = NULL;
//  }
//
//  fprintf( stdout, "%s has %u rows\n", name, rows );
//  UnloadFileData( data );
//
//  if ( levelStart )
//    st_daAppend( (playerData->levelsData), currentLevel );
//}
//
//void st_loadUserMetrics( struct st_fileAccess *userMetricsFile, struct st_playersData* out )
//{
//  // skip first line
//  st_readLine( userMetricsFile );
//
//  unsigned char* line = NULL;
//  while ( ( line = st_readLine( userMetricsFile ) ) )
//  {
//    struct st_packedStringArray cols = st_splitLine( line, "," );
//    assert( cols.count == 10 && "User metrics csv has ten columns" );
//
//    unsigned char* playerName = cols.items[0];
//
//    struct st_playerData* playerData = NULL;
//    for ( unsigned int i = 0; i < out->count; ++i )
//      if ( strcmp( (const char*)out->items[i].name, (const char*)playerName ) == 0 )
//      {
//        playerData = &out->items[i];
//        break;
//      }
//    assert( playerData );
//
//    playerData->relatedness = atof( (const char*) cols.items[1] );
//    playerData->competence = atof( (const char*) cols.items[2] );
//    playerData->immersion = atof( (const char*) cols.items[3] );
//    playerData->fun = atof( (const char*) cols.items[4] );
//    playerData->autonomy = atof( (const char*) cols.items[5] );
//    playerData->physical = atof( (const char*) cols.items[6] );
//    playerData->analytical = atof( (const char*) cols.items[7] );
//    playerData->socioemotional = atof( (const char*) cols.items[8] );
//    playerData->insight = atof( (const char*) cols.items[9] );
//    
//    free( line );
//    line = NULL;
//  }
//}
//
//static void st_printUserDataDebug( struct st_playerData playerData )
//{
//  fprintf(stdout, "----------------------------------------\n");
//  fprintf(stdout, "User: %s\n", playerData.name );
//  fprintf(stdout, "relatedness: %f, competence: %f, immersion: %f, fun: %f, autonomy: %f\n",
//      playerData.relatedness,
//      playerData.competence, 
//      playerData.immersion,
//      playerData.fun,
//      playerData.autonomy);
//  fprintf(stdout, "physical: %f, analytical: %f, socioemotional: %f, insight: %f\n",
//      playerData.physical,
//      playerData.analytical,
//      playerData.socioemotional,
//      playerData.insight);
//  for ( unsigned int i = 0; i < playerData.levelsData.count; ++i )
//  {
//    struct st_levelData levelData = playerData.levelsData.items[i];
//    fprintf(stdout, "level: %s with %u position points\n", levelData.name, levelData.positions.count);
//  }
//  fprintf(stdout, "----------------------------------------\n");
//}
//
//static struct st_playersData st_loadCsv()
//{
//  // csv folder
//  static const char *csvDir = "csv";
//  assert( DirectoryExists(csvDir) );
//
//  // load users gameplay data
//  static const char *usersDir = "csv/users";
//  FilePathList userFiles;
//  userFiles = LoadDirectoryFiles( usersDir );
//
//  struct st_playersData playersData = {0};
//
//  for ( unsigned int fileIdx = 0; fileIdx < userFiles.count; ++fileIdx )
//  {
//    const char* filePath = userFiles.paths[ fileIdx ];
//    const char* playerName = GetFileNameWithoutExt( filePath );
//    const size_t playerNameSize = strlen( playerName );
//    struct st_playerData playerData = {0};
//    playerData.name = malloc( playerNameSize + 1 );
//    strcpy_s( (char*)playerData.name, playerNameSize + 1, playerName );
//    st_loadUser( playerName, filePath, &playerData );
//    st_daAppend( playersData, playerData );
//  }
//
//  // load player metrics
//  static const char *usersMetricsFile = "csv/player_metrics.csv";
//  assert( FileExists(usersMetricsFile) );
//  int usersMetricsDataSize;
//  unsigned char *usersMetricsData;
//  usersMetricsData = LoadFileData( usersMetricsFile, &usersMetricsDataSize );
//
//  assert( usersMetricsData && usersMetricsDataSize );
//  struct st_fileAccess userMetricsFile = {
//    .data = usersMetricsData,
//    .count = usersMetricsDataSize,
//    .offset = 0u
//  };
//  st_loadUserMetrics( &userMetricsFile, &playersData );
//  UnloadFileData( usersMetricsData );
//
//  UnloadDirectoryFiles(userFiles);
//
//  for ( unsigned int i = 0; i < playersData.count; ++i )
//    st_printUserDataDebug( playersData.items[i] );
//
//  return playersData;
//}

//bool st_testPlayerMotivationAndChallenge( const struct st_playerData* const playerData, struct st_pointDataFilter pointFilter )
//{
//  return pointFilter.relatednessRange.x <= playerData->relatedness && playerData->relatedness <= pointFilter.relatednessRange.y
//  && pointFilter.competenceRange.x <= playerData->competence && playerData->competence <= pointFilter.competenceRange.y
//  && pointFilter.immersionRange.x <= playerData->immersion && playerData->immersion <= pointFilter.immersionRange.y
//  && pointFilter.funRange.x <= playerData->fun && playerData->fun <= pointFilter.funRange.y
//  && pointFilter.autonomyRange.x <= playerData->autonomy && playerData->autonomy <= pointFilter.autonomyRange.y
//  && pointFilter.physicalRange.x <= playerData->physical && playerData->physical <= pointFilter.physicalRange.y
//  && pointFilter.analyticalRange.x <= playerData->analytical && playerData->analytical <= pointFilter.analyticalRange.y
//  && pointFilter.socioemotionalRange.x <= playerData->socioemotional && playerData->socioemotional <= pointFilter.socioemotionalRange.y
//  && pointFilter.insightRange.x <= playerData->insight && playerData->insight <= pointFilter.insightRange.y;
//}
//
//void st_findBounds( const struct st_packedPositionArray points, struct st_aabb* bounds )
//{
//  if ( points.count == 0u )
//  {
//    bounds->topLeft = st_vec2_ZERO; 
//    bounds->bottomRight = st_vec2_ZERO;
//    return;
//  }
//
//  bounds->topLeft =  points.items[0];
//  bounds->bottomRight =  points.items[0];
//
//  for ( int point_idx = 1; point_idx < points.count; ++point_idx )
//  {
//    const struct st_vec2 point = points.items[ point_idx ];
//    if ( bounds->topLeft.x > point.x ) bounds->topLeft.x = point.x;
//    if ( bounds->topLeft.y > point.y ) bounds->topLeft.y = point.y; // y points down
//    if ( bounds->bottomRight.x < point.x ) bounds->bottomRight.x = point.x;
//    if ( bounds->bottomRight.y < point.y ) bounds->bottomRight.y = point.y;
//  }
//
//  struct st_vec2 size = {.x = fabsf(bounds->topLeft.x - bounds->bottomRight.x), .y = fabsf(bounds->topLeft.y  - bounds->bottomRight.y) };
//  if( size.x > size.y )
//    bounds->bottomRight.y = bounds->topLeft.y + size.x;
//  else if( size.y > size.x )
//    bounds->bottomRight.x = bounds->topLeft.x + size.y;
//}
//
//// return SSBO id
//struct st_positionsSSBO st_loadPointBuffer( struct st_playersData players, struct st_pointDataFilter pointFilter )
//{
//  struct st_positionsSSBO res = { 0 };
//
//  unsigned int player_idx_start = 0;
//  unsigned int player_idx_end = players.count;
//
//  if ( pointFilter.player > 0 )
//  {
//    player_idx_start = pointFilter.player - 1;
//    player_idx_end = pointFilter.player;
//  }
//
//  for ( unsigned int player_idx = player_idx_start; player_idx < player_idx_end; ++player_idx )
//  {
//    const struct st_playerData* const playerData = &players.items[player_idx];
//    const bool hasMotivationAndChallenge = st_testPlayerMotivationAndChallenge( playerData, pointFilter );
//
//    if ( hasMotivationAndChallenge )
//    {
//      const struct st_levelsData* const levelsData = &playerData->levelsData;
//
//      for ( unsigned int lvl_idx = 0; lvl_idx < levelsData->count; ++lvl_idx )
//      {
//        const struct st_levelData* const levelData = &levelsData->items[lvl_idx];
//        assert( levelData->successfulJumps.count % 2 == 0  );
//        if ( strcmp((const char*) levelData->name, pointFilter.levelName) == 0 )
//        {
//          static unsigned int indexStack[ 1024 ];
//          unsigned int indexStackHead = 0;
//          assert( levelData->successfulJumps.count <= sizeof( indexStack ) / sizeof( unsigned int ) );
//
//          if ( pointFilter.successfulRunOnly )
//          {
//            if ( levelData->successfulRun >= 0 )
//            {
//              indexStack[ indexStackHead++ ] = levelData->successfulRun;
//              indexStack[ indexStackHead++ ] = levelData->positions.count;
//            }
//          }
//          else if ( pointFilter.successfulJumpsOnly )
//          {
//            for ( unsigned int i = 0; i < levelData->successfulJumps.count; i += 2 )
//            {
//              indexStack[ indexStackHead++ ] = levelData->successfulJumps.items[ i ];
//              indexStack[ indexStackHead++ ] = levelData->successfulJumps.items[ i + 1 ];
//            }
//          }
//          else
//          {
//            indexStack[ indexStackHead++ ] = 0;
//            indexStack[ indexStackHead++ ] = levelData->positions.count;
//          }
//
//          unsigned int start = 0;
//          unsigned int end = 0;
//          while ( indexStackHead != 0 )
//          {
//            start = indexStack[indexStackHead - 2];
//            end = indexStack[indexStackHead - 1];
//            indexStackHead -= 2;
//
//            for ( unsigned int pos_idx = start; pos_idx < end; ++pos_idx )
//              st_daAppend( res.positions, levelData->positions.items[ pos_idx ] );
//          }
//
//          break;
//        }
//      }
//    }
//  }
//  fprintf( stdout, "loaded %u positions \n", res.positions.count );
//  if ( res.positions.count == 0 )
//    return res;
//
//  st_daShrinkToFit( res.positions );
//  res.ssboHandle = rlLoadShaderBuffer(res.positions.count * sizeof( *res.positions.items ), res.positions.items, RL_DYNAMIC_COPY);
//  return res;
//}
//
//struct st_positionsSSBO st_loadPointBufferFromLevel( const struct st_levelData* const levelData )
//{
//  struct st_positionsSSBO res = { 0 };
//
//
//  for ( unsigned int pos_idx = 0; pos_idx < levelData->positions.count; ++pos_idx )
//    st_daAppend( res.positions, levelData->positions.items[ pos_idx ] );
//
//  st_daShrinkToFit( res.positions );
//  res.ssboHandle = rlLoadShaderBuffer(res.positions.count * sizeof( *res.positions.items ), res.positions.items, RL_DYNAMIC_COPY);
//  return res;
//}
//
//// Unload shader storage buffer object (SSBO)
//void rlUnloadUniformBuffer(unsigned int uboId)
//{
//#if defined(GRAPHICS_API_OPENGL_43)
//    glDeleteBuffers(1, &ssboId);
//#else
//    TRACELOG(RL_LOG_WARNING, "UBO: UBO not enabled. Define GRAPHICS_API_OPENGL_43");
//#endif
//
//}
//
//#define HEATMAP_WIDTH 2048 
//#define HEATMAP_HEIGHT 2048
//
//#define NUM_LEVELS 6
//
//typedef enum {
//  ARIES = 0,
//  TAURUS,
//  GEMINI,
//  CANCER,
//  LEO,
//  VIRGO
//} LEVEL;
//
//static const char *levelName[] = {
//    "aries_test.tscn",
//    "taurus_test.tscn",
//    "gemini_test.tscn",
//    "cancer_test.tscn",
//    "leo_test.tscn",
//    "virgo_test.tscn"
//};
//
//#define NUM_CATEGORIES 9
//
//typedef enum {
//  // CAH
//  RELATEDNESS = 0,
//  COMPETENCE,
//  IMMERSION,
//  FUN,
//  AUTONOMY,
//
//  // IMG
//  PHYSICAL,
//  ANALYTICAL,
//  SOCIOEMOTIONAL,
//  INSIGHT,
//} CATEGORY;
//
//int compare(const void *a, const void *b) {
//    float diff = *(float*)a - *(float*)b;
//    return (diff > 0) - (diff < 0); // Correct comparison
//}
//
#endif // STRING_THEORY_H

