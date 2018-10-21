#ifndef FRACTALEXPLORER_CLSOURCES_HPP
#define FRACTALEXPLORER_CLSOURCES_HPP

#include <string>
#include <unordered_map>

#include "../OpenCL.hpp"

static constexpr const char* STDLIB = "stdlib";

static class SourcesRegistry {

    std::unordered_map<std::string, cl::Program::Sources> registry_;

public:

    SourcesRegistry();

    inline bool idTaken(const std::string& id) const noexcept {
        return registry_.find(id) != registry_.end();
    };

    void registerSources(std::string id, cl::Program::Sources&& src);

    cl::Program::Sources findById(const std::string& id);

} sourcesRegistry {};

#endif //FRACTALEXPLORER_CLSOURCES_HPP

