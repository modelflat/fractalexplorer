#ifndef FRACTALEXPLORER_GLSL_RENDER_HPP
#define FRACTALEXPLORER_GLSL_RENDER_HPP

#include <string_view>

static constexpr std::string_view RENDER_FRAG { R"GLSL(
#version 400

in vec2 fragmentUV;
uniform sampler2D tex;

void main() {
    gl_FragColor = texture(tex, fragmentUV);
}
)GLSL" };

static constexpr std::string_view RENDER_VERT { R"GLSL(
#version 400

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 vertexUV;

out vec2 fragmentUV;
out vec2 texCoords;

void main() {
    gl_Position = vec4(position, 1.0, 1.0);
    fragmentUV = vertexUV;
    texCoords = vec2(gl_MultiTexCoord0);
}
)GLSL"};

#endif //FRACTALEXPLORER_GLSL_RENDER_HPP
