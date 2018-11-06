#ifndef FRACTALEXPLORER_OPENCLBACKEND_HPP
#define FRACTALEXPLORER_OPENCLBACKEND_HPP

#include "OpenCL.hpp"

#include "OpenCLKernelUtils.hpp"
#include "Utility.hpp"

#include <unordered_map>
#include <optional>

struct KernelId {
    std::string src;
    std::string settings;

    bool operator==(const KernelId& other) const {
        return other.src == src && other.settings == settings;
    }
};

struct CompilationContext {
    KernelId id;
    cl::Context ctx;

    bool operator==(const CompilationContext& other) const {
        return other.id == id && other.ctx() == ctx();
    }
};

namespace std {
    template<> struct hash<KernelId> {
        size_t operator()(const KernelId& id) const noexcept {
            auto h1 = std::hash <std::string> {} (id.src);
            auto h2 = std::hash <std::string> {} (id.settings);
            return 31 * h1 + h2; // classic hash combine
        }
    };

    template<> struct hash<CompilationContext> {
        size_t operator()(const CompilationContext& ctx) const noexcept {
            auto h1 = std::hash <KernelId> {} (ctx.id);
            auto h2 = std::hash <cl_context> {} (ctx.ctx());
            return 31 * h1 + h2;
        }
    };
}

/**
 * OpenCL Backend. Keeps everything related to OpenCL computations together.
 */
class OpenCLBackend {

    cl::Context ctx;

    cl::CommandQueue queue;

    std::unordered_map<KernelId, KernelBase> registry_;

    std::unordered_map<CompilationContext, cl::Kernel> compileCache_;

    cl::Kernel compileCLKernel(KernelId);

public:

    OpenCLBackend();

    inline cl::Context currentContext() noexcept { return ctx; }

    inline cl::CommandQueue currentQueue() noexcept { return queue; }

    template<typename T>
    KernelInstance<T> compileKernel(KernelId id) {
        return { compileCLKernel(id) };
    }

    const KernelBase& findKernelBase(KernelId) const;

    /**
     * Make mapping id -> base
     */
    void registerKernel(KernelId, KernelBase&&);

    void clearCache();

};

/**
 * Keystroke saviour
 */
using OpenCLBackendPtr = std::shared_ptr<OpenCLBackend>;

inline static bool memoryBelongsToContext(const cl::Memory& memory, const cl::Context& context) {
    return memory.getInfo<CL_MEM_CONTEXT>()() == context();
}

template <typename UserArgProperties>
class KernelArgConfigurationStorage {

//    inline LOGGER();
    std::unordered_map<KernelId, std::pair< std::string, KernelArgProperties<UserArgProperties>>> storage_ {};

public:

    using UserArgPropertiesType = UserArgProperties;

    void registerConfiguration(KernelId id, const ArgsTypesWithNames& argDict, std::string conf) {
        absl::StripAsciiWhitespace(&conf);
        storage_[id] = std::make_pair(conf, propertiesFromStream(argDict, conf));
    }

    inline void registerConfiguration(KernelId id, const KernelArgProperties<UserArgProperties> &conf) {
        storage_[id] = std::make_pair("", conf); // TODO compose string from configuration or is it an overkill?
    }

    inline void registerConfiguration(KernelId id, KernelArgProperties<UserArgProperties> &&conf) {
        storage_[id] = std::make_pair("", std::move(conf));
    }

    void registerConfiguration(KernelId id, std::string conf) {
        absl::StripAsciiWhitespace(&conf);
        storage_[id] = std::make_pair(conf, KernelArgProperties<UserArgProperties>{});
    }

    const std::pair<std::string, KernelArgProperties<UserArgProperties>>&
    findConfiguration(KernelId id) const {
        auto it = storage_.find(id);
        if (it == storage_.end()) {
            auto err = fmt::format("Cannot find configuration for id {}.{}", id.src, id.settings);
            logger->error(err);
            throw std::runtime_error(err);
        }
        return it->second;
    }

    KernelArgProperties<UserArgProperties>
    findOrParseConfiguration(KernelId id, const ArgsTypesWithNames& argDict) {
        auto [strConf, conf] = findConfiguration(id);
        if (conf.empty()) {
            // TODO maybe set up a flag to know exactly if configuration was parsed?
            logger->info(fmt::format("Configuration is empty, seems like recompilation is needed"));
            return storage_.try_emplace(id, strConf, propertiesFromConfig<UserArgProperties>(argDict, strConf)).first->second.second;
        } else {
            logger->info(fmt::format("Configuration already not empty, too lazy to compile again"));
        }
        return findConfiguration(id).second; // TODO optimize ?
    }

};

template <typename UserArgProperties>
using KernelArgConfigurationStoragePtr = std::shared_ptr<KernelArgConfigurationStorage<UserArgProperties>>;

#endif //FRACTALEXPLORER_OPENCLBACKEND_HPP
