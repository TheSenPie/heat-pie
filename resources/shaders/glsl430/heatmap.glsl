#version 430

#define HEATMAP_WIDTH 1080
#define HEATMAP_HEIGHT 1080

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

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
	
	// Calculate bounds dimensions
	vec2 boundsSize = bounds.bottomRight - bounds.topLeft;
	vec4 pointColor = vec4(0.0, 0.0, 0.0, 1.0);

	for (int i = 0; i < points.length(); ++i) {
		vec2 point = points[i];
		vec2 normalized = (point - bounds.topLeft) / boundsSize;

		ivec2 texCoord = ivec2(
			int( normalized.x * float( HEATMAP_WIDTH - 1) ),
			int( normalized.y * float( HEATMAP_HEIGHT - 1) )
		);

		imageStore( plotOutput, texCoord, pointColor );
	}
}
