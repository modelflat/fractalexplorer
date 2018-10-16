#include "GLSL_Sources.hpp"

#include <map>
#include <string>

#include "GLSL_Render.hpp"
#include "GLSL_RenderStatisticalClear.hpp"


static const std::map<std::string, std::string_view> SHADER_REGISTRY {
        {
            { "render.vert", RENDER_VERT },
            { "render.frag", RENDER_FRAG },
            { "render_stat_clear.frag", RENDER_STAT_FRAG }
        }
};


std::string_view getShaderSource(std::string name) {
    return SHADER_REGISTRY.at(name);
}
