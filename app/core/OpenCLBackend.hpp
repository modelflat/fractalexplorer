#ifndef FRACTALEXPLORER_OPENCLBACKEND_HPP
#define FRACTALEXPLORER_OPENCLBACKEND_HPP


#include "OpenCL.hpp"

#include "OpenCLKernelUtils.hpp"

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

struct KernelWithMetainfo {
    cl::Kernel kernel;
    cl_uint imageArgIdx;
    std::vector<cl_uint> dimensionalArgs;
    ArgNameMap param2name;
};

/**
 * OpenCL Backend. Keeps everything related to OpenCL computations together.
 */
class OpenCLBackend {

    cl::Context ctx;

    cl::CommandQueue queue;

    std::unordered_map<KernelId, KernelBase> registry_;

    std::unordered_map<CompilationContext, KernelWithMetainfo> compileCache_;

public:

    OpenCLBackend();

    inline cl::Context currentContext() noexcept { return ctx; }

    inline cl::CommandQueue currentQueue() noexcept { return queue; }

    KernelWithMetainfo compileKernel(KernelId);

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

#endif //FRACTALEXPLORER_OPENCLBACKEND_HPP
