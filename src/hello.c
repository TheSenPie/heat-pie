#include "raylib.h"


int main(void)
{
	const unsigned int screenWidth = 800u;
	const unsigned int screenHeight = 450u;

	InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window" );

	SetTargetFPS( 60u );

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
