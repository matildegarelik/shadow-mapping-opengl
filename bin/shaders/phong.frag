# version 330 core

in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoords;
in vec4 lightVSPosition;
in vec4 fragPosLightSpace;

//uniform vec3 lightPosition;
//uniform vec3 viewPos;

// propiedades del material
uniform sampler2D depthTexture; // ambient and diffuse components
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;
uniform float opacity;

uniform float bias;
uniform bool pcf;
uniform bool shadow_active;

// propiedades de la luz
uniform float ambientStrength;
uniform vec3 lightColor;

out vec4 fragColor;

#include "funcs/calcPhong.frag"

float ShadowCalculation(vec4 fragPosLightSpace)
{
	// perform perspective divide
	vec3 projCoords = vec3(fragPosLightSpace) / fragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(depthTexture, projCoords.xy).r; 
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// calculate bias (based on depth map resolution and slope)
	vec3 normal = normalize(fragNormal);
	vec3 lightDir = normalize(vec3(lightVSPosition) - fragPosition);
	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	// check whether current frag pos is in shadow
	// float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	// PCF
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	
	if(pcf){
		shadow =0;
		vec2 texelSize = 1.0 / textureSize(depthTexture, 0);
		for(int x = -1; x <= 1; ++x)
		{
			for(int y = -1; y <= 1; ++y)
			{
				vec2 syt = projCoords.xy + vec2(x, y) * texelSize;
				if(syt.x>1 || syt.x<0 ||syt.y<0 ||syt.y>1) return -1.f;
				float pcfDepth = texture(depthTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
				shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
			}    
		}
		shadow /= 9.0;
	}
	
	// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if(projCoords.z > 1.0)
		shadow = 0.0;
	
	return shadow;
}

void main() {
	
	vec3 phong = calcPhong(lightVSPosition, lightColor, ambientStrength,
		ambientColor,diffuseColor,
		specularColor, shininess);
	if(shadow_active){
		float shadow = ShadowCalculation(fragPosLightSpace);
		if(shadow == -1){
			fragColor=vec4(1.f,0.f,0.f,1.f);
		}else{
			vec3 lighting= (vec3(ambientColor*ambientStrength) * shadow+ phong*(1.f-shadow));
			//fragColor = vec4(vec3(1.0-shadow),opacity);
			fragColor = vec4(lighting,opacity);
		}
	}else{
		fragColor = vec4(phong,opacity);
	}
	
}

