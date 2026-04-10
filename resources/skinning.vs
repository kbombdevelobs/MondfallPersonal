#version 330

// Vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
in vec4 vertexBoneIds;
in vec4 vertexBoneWeights;

// Uniforms
uniform mat4 mvp;
uniform mat4 boneMatrices[128];

// Outputs to fragment shader
out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    // Skinning: blend vertex position by up to 4 bone influences
    int bi0 = int(vertexBoneIds.x);
    int bi1 = int(vertexBoneIds.y);
    int bi2 = int(vertexBoneIds.z);
    int bi3 = int(vertexBoneIds.w);

    vec4 pos = vec4(vertexPosition, 1.0);
    vec4 skinnedPos =
        boneMatrices[bi0] * pos * vertexBoneWeights.x +
        boneMatrices[bi1] * pos * vertexBoneWeights.y +
        boneMatrices[bi2] * pos * vertexBoneWeights.z +
        boneMatrices[bi3] * pos * vertexBoneWeights.w;

    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * skinnedPos;
}
