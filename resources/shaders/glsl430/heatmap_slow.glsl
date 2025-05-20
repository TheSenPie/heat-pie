#version 430

#define HEATMAP_WIDTH 128
#define HEATMAP_HEIGHT 128

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct aabb
{
	vec2 topLeft;
	vec2 bottomRight;
};

layout(std140, binding = 0) uniform heatmapMetadata {
	aabb bounds;
};

layout(std430, binding = 1) readonly restrict buffer pointsBuffer {
	vec2 points[];
};

layout (rgba32f, binding = 2) writeonly restrict uniform image2D plotOutput;

void main() {
    ivec2 currentTexelCoord = ivec2(gl_GlobalInvocationID.xy);

    // Early bounds check
    if (currentTexelCoord.x >= HEATMAP_WIDTH || currentTexelCoord.y >= HEATMAP_HEIGHT) {
        return;
    }

    // Calculate current pixel position in world space
    vec2 boundsSize = bounds.bottomRight - bounds.topLeft;
    vec2 texelPos = bounds.topLeft + (vec2(currentTexelCoord) + 0.5) / vec2(HEATMAP_WIDTH, HEATMAP_HEIGHT) * boundsSize;
    float bandwidth = 5.0;

    // KDE calculation
    float density = 0.0;
    for (int i = 0; i < points.length(); ++i) {
        vec2 point = points[i];

        // Calculate squared distance
        vec2 diff = point - texelPos;
        float distSquared = dot(diff, diff);

        // Apply Gaussian kernel: (1/(2πσ²)) * exp(-d²/(2σ²))
        float kernelValue = exp(-distSquared / (2.0 * bandwidth * bandwidth));
        // Normalize by bandwidth (note: we skip the 2π factor as it's just a scaling constant)
        kernelValue /= (bandwidth * bandwidth);

        // Accumulate density
        density += kernelValue;
    }

    // Normalize by number of points (optional depending on your needs)
    // density /= float(points.length());

    // Output density to texture with a reasonable scaling
    // You may need to adjust the scaling factor based on your data
    vec4 color = vec4(density, density, density, 1.0);
    imageStore(plotOutput, currentTexelCoord, color);
}
