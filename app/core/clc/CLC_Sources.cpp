#include "CLC_Sources.hpp"

#include <map>
#include <string>
#include <sstream>

#include "app/core/clc/CLC_Definitions.hpp"
#include "app/core/clc/CLC_Random.hpp"

#include "app/core/clc/CLC_NewtonFractal.hpp"

static const std::map<std::string, cl::Program::Sources> ALGORITHM_REGISTRY {
    {
        "newton_fractal", {
            { NEWTON_FRACTAL_3EQ_SOURCE.data(), NEWTON_FRACTAL_3EQ_SOURCE.size() },
            { NEWTON_FRACTAL_SOURCE.data(), NEWTON_FRACTAL_SOURCE.size() }
        }
    }
};

static const cl::Program::Sources COMMON_SOURCES {
        { RANDOM_SOURCE.data(), RANDOM_SOURCE.size() },
        { DEFINITIONS_SOURCE.data(), DEFINITIONS_SOURCE.size() }
};


cl::Program compileProgramWithCommonSources(cl::Context ctx, cl::Program::Sources sources, std::string options) {
    sources.insert(sources.begin(), COMMON_SOURCES.begin(), COMMON_SOURCES.end());
    cl::Program program {ctx, sources};
    program.build(options.data());
    return program;
}

cl::Program compileBuiltInAlgorithm(cl::Context ctx, std::string name, std::string options) {
    auto sources = ALGORITHM_REGISTRY.find(name);
    if (sources == ALGORITHM_REGISTRY.end()) {
        std::ostringstream ss;
        ss << "No such algorithm: \"" << name << "\"";
        throw std::exception(ss.str().c_str());
    }
    return compileProgramWithCommonSources(ctx, sources->second, options);
}