#include "string_theory.h"

void st_daUnloadPackedStringArray( struct st_packedStringArray *arr )
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

void st_daUnloadPackedPositionArray( struct st_packedPositionArray *arr )
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

void st_daUnloadNumbers( struct st_numbers *arr )
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

void hp_initPlugin()
{
  st_initDataLoader();
  st_initGUI();
}

void hp_cleanupPlugin()
{
  st_cleanupGUI();
  st_cleanupDataLoader();
}
