#version 430
#define HEATMAP_WIDTH 2048
#define HEATMAP_HEIGHT 2048
layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

struct aabb {
  vec2 topLeft;
  vec2 bottomRight;
};

layout(std140, binding = 0) uniform heatmapMetadata {
  aabb bounds;
};

layout(std430, binding = 1) readonly restrict buffer pointsBuffer {
  vec2 points[];
};

layout(r32ui, binding = 2) coherent restrict uniform uimage2D plotOutput;

// Constants
const float sigma = 100.0;  // Gaussian sigma
const float PI = 3.14159265359;
const float CUTOFF_MULTIPLIER = 3.0;  // Points beyond 3*sigma have negligible contribution

void main() {
  // Each thread processes one point
  uint pointIndex = gl_GlobalInvocationID.x;
  
  // Get this thread's point
  vec2 point = points[pointIndex];
  
  // Calculate pixel-space cutoff distance
  vec2 boundsSize = bounds.bottomRight - bounds.topLeft;
  float cutoffDistance = sigma * CUTOFF_MULTIPLIER;
  float worldToPixelFixedAspect = min( HEATMAP_WIDTH / boundsSize.x, HEATMAP_HEIGHT / boundsSize.y );
  float pixelToWorldFixedAspect = 1.0 / worldToPixelFixedAspect;
  int cutoffDistancePixels = int(ceil( cutoffDistance * worldToPixelFixedAspect ));
  
  // Calculate the center pixel for this point
  ivec2 centerPixel = ivec2( ( point - bounds.topLeft ) * worldToPixelFixedAspect );
  
  // Calculate contribution value (pre-compute the constant part of the Gaussian)
  float sigma2 = sigma * sigma;
  float gaussianNormalization = 1.0 / sqrt( 2.0 * PI * sigma2 );
  float reciprocal2sigma2 = 1 / ( 2.0 * sigma2 );
  
  // Determine affected pixel range (clamp to texture boundaries)
  ivec2 minPixel = max(ivec2(0), centerPixel - ivec2(cutoffDistancePixels));
  ivec2 maxPixel = min(ivec2(HEATMAP_WIDTH-1, HEATMAP_HEIGHT-1), centerPixel + ivec2(cutoffDistancePixels));
  
  // Loop through affected pixels and add contribution
  for ( int y = minPixel.y; y <= maxPixel.y; y++ ) {
    for ( int x = minPixel.x; x <= maxPixel.x; x++ ) {
      // Calculate pixel position in world space
      vec2 worldPos = bounds.topLeft + ( ( vec2(x, y) + 0.5 ) * pixelToWorldFixedAspect );

      // Calculate squared distance
      vec2 diff = point - worldPos;
      float distSquared = dot(diff, diff);

      //// Calculate Gaussian contribution
      float contribution = gaussianNormalization * exp(-distSquared * reciprocal2sigma2 );

      //// Convert to a float for atomic addition
      //// Note: imageAtomicAdd only works with int/uint, so we use a scaled integer representation
      //// We'll use a scaling factor to preserve precision (multiply by 10000)
      int scaledContribution = int(contribution * 10000.0);

      // Alternative if using r32ui format:
      //imageAtomicAdd(plotOutput, ivec2(x, y), scaledContribution);
      imageAtomicAdd( plotOutput, ivec2(x, y), scaledContribution );
    }
  }
  //imageAtomicExchange( plotOutput, centerPixel, 1 );

  memoryBarrier();
}

