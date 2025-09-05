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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#pragma clang diagnostic pop

#define MAX_POINT_COUNT 262144

struct hp_pointsSSBO;

// aabb
struct hp_aabb;
static void hp_printAABB  ( struct hp_aabb bounds );
static void hp_findBounds ( const Vector2 *const points, unsigned int pointsCount, struct hp_aabb *const bounds );

// point buffer related
void                  hp_updatePointsBuffer ( struct hp_pointsSSBO *ssbo );
struct hp_pointsSSBO  hp_loadPointsBuffer   ();

// external dependencies
extern void hp_initPlugin     ();
extern void hp_cleanupPlugin  ();

extern void hp_fetchPoints          ( Vector2 **points, unsigned int *pointsCount );
extern bool hp_heatmapShouldReload  ( );

struct hp_pointsSSBO{
  unsigned int handle;
  unsigned int count;
  Vector2 *points;
};

struct hp_aabb{
  Vector2 topLeft;
  Vector2 bottomRight;
};

static void hp_printAABBDebug( const struct hp_aabb bounds )
{
  fprintf(stdout, "top-left: (%f, %f)\nbottom-right: (%f, %f)\nwidth-height: (%f, %f)\n", 
  bounds.topLeft.x, bounds.topLeft.y,
  bounds.bottomRight.x, bounds.bottomRight.y,
  fabsf(bounds.topLeft.x - bounds.bottomRight.x), fabsf(bounds.topLeft.y  - bounds.bottomRight.y));
}

static void hp_findBounds( const Vector2 *const points, const unsigned int pointsCount, struct hp_aabb *const bounds )
{
  if ( pointsCount == 0u )
  {
    bounds->topLeft = Vector2Zero(); 
    bounds->bottomRight = Vector2Zero();
    return;
  }

  bounds->topLeft =  points[0];
  bounds->bottomRight =  points[0];
  for ( int point_idx = 1; point_idx < pointsCount; ++point_idx )
  {
    const Vector2 point = points[ point_idx ];
    if ( bounds->topLeft.x > point.x ) bounds->topLeft.x = point.x;
    if ( bounds->topLeft.y > point.y ) bounds->topLeft.y = point.y; // y points down
    if ( bounds->bottomRight.x < point.x ) bounds->bottomRight.x = point.x;
    if ( bounds->bottomRight.y < point.y ) bounds->bottomRight.y = point.y;
  }

  Vector2 size = {.x = fabsf(bounds->topLeft.x - bounds->bottomRight.x), .y = fabsf(bounds->topLeft.y  - bounds->bottomRight.y) };
  if( size.x > size.y )
    bounds->bottomRight.y = bounds->topLeft.y + size.x;
  else if( size.y > size.x )
    bounds->bottomRight.x = bounds->topLeft.x + size.y;
}

struct hp_pointsSSBO hp_loadPointsBuffer()
{
  struct hp_pointsSSBO ssbo = {0};
  ssbo.handle = rlLoadShaderBuffer( sizeof( Vector2 ) * MAX_POINT_COUNT, NULL, RL_DYNAMIC_COPY );

  return ssbo;
}

void hp_updatePointsBuffer( struct hp_pointsSSBO *ssbo )
{
  if ( ssbo->points != NULL )
    free( ssbo->points );

  hp_fetchPoints( &ssbo->points, &ssbo->count );
  assert( ssbo->count < MAX_POINT_COUNT ); 
  rlUpdateShaderBuffer( ssbo->handle, ssbo->points, ssbo->count * sizeof( Vector2 ), 0 );
}

// Load shader storage buffer object (SSBO)
unsigned int rlLoadUniformBuffer(unsigned int size, const void *data, int usageHint)
{
    unsigned int ubo = 0;

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, data, usageHint? usageHint : RL_STREAM_COPY);
    if (data == NULL) glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);    // Clear buffer data to 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return ubo;
}

// Update UBO buffer data
void rlUpdateUniformBuffer(unsigned int id, const void *data, unsigned int dataSize, unsigned int offset)
{
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, dataSize, data);
}

// Unload shader storage buffer object (SSBO)
void rlUnloadUniformBuffer(unsigned int uboId)
{
    glDeleteBuffers(1, &uboId);
}

#define HEATMAP_WIDTH 2048 
#define HEATMAP_HEIGHT 2048

int main(void)
{
  hp_initPlugin();

  const unsigned int screenWidth = 800u;
  const unsigned int screenHeight = 640u;

  SetConfigFlags( FLAG_WINDOW_RESIZABLE );
  InitWindow(screenWidth, screenHeight, "HeatPie" );

  SetTargetFPS( 60u );

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

  for (int idx = 0; idx < 3; idx++)
  {
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
  int err;
  err = glGetError();
  if (err)
    printf("GL ERR BEFORE %x\n", err);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, HEATMAP_WIDTH, HEATMAP_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
  err = glGetError();
  if (err)
    printf("GL ERR AFTER %x\n", err);

  Texture rlHeatmapTex = {
    .id = heatmapTex,
    .width = HEATMAP_WIDTH, .height = HEATMAP_HEIGHT,
    .mipmaps = 0,
    .format = PIXELFORMAT_UNCOMPRESSED_R32
  };

  // Create the uniform buffer for AABB bounds
  struct hp_aabb dataBounds = {0};
  unsigned int boundsUBO = rlLoadUniformBuffer(sizeof( dataBounds ), &dataBounds, RL_DYNAMIC_COPY);

  // Create points buffer
  struct hp_pointsSSBO pointsBuffer = hp_loadPointsBuffer();

  Camera2D camera = { 0 };
  camera.zoom = 1.0f;

  RenderTexture2D saveTargetTex = LoadRenderTexture( HEATMAP_WIDTH, HEATMAP_HEIGHT );

  // Main game loop
  while ( !WindowShouldClose() )
  {
    Rectangle scrollBounds = { GetScreenWidth() - 120, 10, 110, 200 };

    // Translate based on mouse right click
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec( GetMousePosition(), scrollBounds))
    {
      Vector2 delta = GetMouseDelta();
      delta = Vector2Scale(delta, -1.0f/camera.zoom);
      camera.target = Vector2Add(camera.target, delta);
    }

    // Zoom based on mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && !CheckCollisionPointRec( GetMousePosition(), scrollBounds) )
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

    // Render the heatmap on reload 
   if ( hp_heatmapShouldReload() )
   {
      hp_updatePointsBuffer( &pointsBuffer );
      //puts("Reload heatmap call");

      dataBounds.topLeft = Vector2Zero();
      dataBounds.bottomRight = Vector2Zero();
      //printf("Vec2(%.03f,%.03f)\n", pointsBuffer.points[0].x, pointsBuffer.points[0].y );
      //printf("Vec2(%.03f,%.03f)\n", pointsBuffer.points[1].x, pointsBuffer.points[1].y );
      //printf("Vec2(%.03f,%.03f)\n", pointsBuffer.points[2].x, pointsBuffer.points[2].y );
      hp_findBounds( pointsBuffer.points, pointsBuffer.count, &dataBounds );
      //printf("AABB(%.03f,%.03f), (%.03f,%.03f)\n", dataBounds.topLeft.x, dataBounds.topLeft.y, dataBounds.bottomRight.x, dataBounds.bottomRight.y );
      rlUpdateUniformBuffer( boundsUBO, &dataBounds, sizeof( dataBounds ), 0 );

      const unsigned int black = 0.0f;
      glClearTexImage( heatmapTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &black);

      rlEnableShader( heatmapLogicProgram );

      glBindBufferBase( GL_UNIFORM_BUFFER, 0, boundsUBO );
      rlBindShaderBuffer( pointsBuffer.handle, 1 );
      glBindImageTexture( 2, heatmapTex, 0, 0, 0, GL_READ_WRITE, GL_R32UI );
      rlComputeShaderDispatch( ( pointsBuffer.count + 128 - 1 ) / 128, 1, 1 );
      glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );

      rlDisableShader();
    }

    BeginDrawing();
      ClearBackground( RAYWHITE );

      //------------------------------------------------------------------------------

      BeginMode2D( camera );
        DrawFPS(0, 0);                                                     // Draw current FPS
        BeginShaderMode( heatmapRenderShader );
        DrawTexture( rlHeatmapTex, 0, 0, WHITE );
        EndShaderMode();

        // TODO: Render misc from gui
      EndMode2D();

      // TODO: Render gui controls

    EndDrawing();
    glFinish();
  }

  // Unload dlls
  hp_cleanupPlugin();

  // Unload resources
  UnloadRenderTexture( saveTargetTex );

  rlUnloadShaderBuffer( pointsBuffer.handle );
  pointsBuffer.handle = 0u;
  free( pointsBuffer.points );
  pointsBuffer.points = NULL;
  pointsBuffer.count = 0u;

  rlUnloadUniformBuffer( boundsUBO );

  rlUnloadShaderProgram( heatmapLogicProgram );

  UnloadTexture( rlHeatmapTex );
  heatmapTex = 0u;
  UnloadShader( heatmapRenderShader );

  CloseWindow();

  return 0;
}
