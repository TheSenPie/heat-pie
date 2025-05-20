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

// Use r32f format for the output texture to support atomic operations with floats
layout (r32f, binding = 2) coherent restrict uniform image2D plotOutput;

// Constants
const float bandwidth = 5.0;  // Gaussian sigma
const float PI = 3.14159265359;
const float CUTOFF_MULTIPLIER = 20.0;  // Points beyond 3*sigma have negligible contribution

void main() {
    // Each thread processes one point
    uint pointIndex = gl_GlobalInvocationID.x;
    
    // Get this thread's point
    vec2 point = points[pointIndex];
    
    // Calculate pixel-space cutoff distance
    vec2 boundsSize = bounds.bottomRight - bounds.topLeft;
    float cutoffDistance = bandwidth * CUTOFF_MULTIPLIER;
    float pixelWorldSize = min(boundsSize.x / HEATMAP_WIDTH, boundsSize.y / HEATMAP_HEIGHT);
    int pixelRadius = int(ceil(cutoffDistance / pixelWorldSize));
    
    // Calculate the center pixel for this point
    vec2 normalizedPos = (point - bounds.topLeft) / boundsSize;
    ivec2 centerPixel = ivec2(normalizedPos * vec2(HEATMAP_WIDTH, HEATMAP_HEIGHT));
    
    // Calculate contribution value (pre-compute the constant part of the Gaussian)
    float gaussianNormalization = 1.0 / (2.0 * PI * bandwidth * bandwidth);
    
    // Determine affected pixel range (clamp to texture boundaries)
    ivec2 minPixel = max(ivec2(0), centerPixel - ivec2(pixelRadius));
    ivec2 maxPixel = min(ivec2(HEATMAP_WIDTH-1, HEATMAP_HEIGHT-1), centerPixel + ivec2(pixelRadius));
    
    // Loop through affected pixels and add contribution
    for (int y = minPixel.y; y <= maxPixel.y; y++) {
        for (int x = minPixel.x; x <= maxPixel.x; x++) {
            // Calculate pixel position in world space
            vec2 pixelPos = bounds.topLeft + (vec2(x, y) + 0.5) / vec2(HEATMAP_WIDTH, HEATMAP_HEIGHT) * boundsSize;
            
            // Calculate squared distance
            vec2 diff = point - pixelPos;
            float distSquared = dot(diff, diff);
            
            // Skip pixels beyond cutoff distance
            if (distSquared > cutoffDistance * cutoffDistance) {
                continue;
            }
            
            // Calculate Gaussian contribution
            float sigma2 = bandwidth * bandwidth;
            float contribution = gaussianNormalization * exp(-distSquared / (2.0 * sigma2));
            
            // Convert to a float for atomic addition
            // Note: imageAtomicAdd only works with int/uint, so we use a scaled integer representation
            // We'll use a scaling factor to preserve precision (multiply by 10000)
            int scaledContribution = int(contribution * 10000.0);
            
            // For r32f formats, we need to use GLSL 4.3+ atomic exchange pattern
            // since there's no direct imageAtomicAdd for floats
            float oldValue, newValue;
            do {
                oldValue = imageLoad(plotOutput, ivec2(x, y)).r;
                newValue = oldValue + contribution;
            } while (imageAtomicExchange(plotOutput, ivec2(x, y), newValue) != oldValue);
            
            // Alternative if using r32ui format:
            // imageAtomicAdd(plotOutput, ivec2(x, y), scaledContribution);
        }
    }
    
    // Ensure visibility of our atomic operations
    memoryBarrier();
}

