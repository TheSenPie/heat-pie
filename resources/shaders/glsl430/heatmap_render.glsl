#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform usampler2D texture0;
uniform vec4 colDiffuse;

//void main()
//{
//	const float exposure = 1.0;
//	const float gamma = 2.2;
//	vec3 hdrColor = texture(texture0, fragTexCoord).rrr;
//
//	// reinhard tone mapping
//	//vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
//	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
//	// gamma correction
//	mapped = pow(mapped, vec3(1.0 / gamma));
//
//	finalColor = vec4(mapped,1.0) * colDiffuse*fragColor;
//}

// Function to convert HSV to RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    const float exposure = 2.0;
    const float gamma = 2.2;
    
    // Get the density value from the red channel
    float density = float( texture( texture0, fragTexCoord ).r ) / 10000.0;
  
    // Apply tone mapping to preserve dynamic range
    float mapped = 1.0 - exp(-density * exposure);
    
    // Apply gamma correction
    mapped = pow(mapped, 1.0 / gamma);
    
    // Create a smooth gradient using HSV
    // Hue ranges from 120 (green) through yellow (~60) to red (0)
    // Saturation is kept high for vibrant colors
    // Value (brightness) increases with density for a subtle glow effect
    
    // Map density to hue: 120° (green) → 0° (red)
    float hue = 120.0 * (1.0 - mapped);
    
    // Keep saturation high, but slightly lower for very high values for a subtle "hot white" at maximum
    float saturation = 1.0 - pow(mapped, 4.0) * 0.3;
    
    // Brightness increases slightly with density for a "glow" effect
    float value = 0.8 + mapped * 0.2;
    
    // Convert HSV to RGB
    vec3 color = hsv2rgb(vec3(hue / 360.0, saturation, value));
    
    // Add subtle glow for high-density areas
    color += vec3(0.2, 0.0, 0.0) * pow(mapped, 5.0);
    
    // Ensure low density areas fade to transparent
    float alpha = smoothstep(0.02, 0.15, mapped);
    
    // Combine with input color parameters
    finalColor = vec4(color, alpha) * colDiffuse * fragColor;
}
