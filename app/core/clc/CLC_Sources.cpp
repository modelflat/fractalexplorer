#include "CLC_Sources.hpp"

#include "app/core/clc/CLC_Definitions.hpp"
#include "app/core/clc/CLC_Random.hpp"
#include "app/core/clc/CLC_NewtonFractal.hpp"

void SourcesRegistry::registerSources(std::string id, cl::Program::Sources&& src) {
    auto res = registry_.try_emplace(id, std::forward<cl::Program::Sources>(src));
    if (!res.second) {
        throw std::runtime_error("Sources already present");
    }
}

cl::Program::Sources SourcesRegistry::findById(const std::string &id) {
    auto src = registry_.find(id);
    if (src == registry_.end()) {
        throw std::runtime_error("No such sources");
    }
    return src->second;
}

SourcesRegistry::SourcesRegistry() {
    // register all the default sources
    registerSources(STDLIB, {
        { // definitions
            DEFINITIONS_SOURCE.data(), DEFINITIONS_SOURCE.size()
        },
        { // PRNG
            RANDOM_SOURCE.data(), RANDOM_SOURCE.size()
        }
    });

    // register newton-fractal sources
    registerSources("newton-fractal", {
        {
            NEWTON_FRACTAL_3EQ_SOURCE.data(), NEWTON_FRACTAL_3EQ_SOURCE.size()
        },
        {
            NEWTON_FRACTAL_SOURCE.data(), NEWTON_FRACTAL_SOURCE.size()
        }
    });
}
