#ifndef FRACTALEXPLORER_OPENCLBACKEND_HPP
#define FRACTALEXPLORER_OPENCLBACKEND_HPP

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#ifdef _WIN32
#pragma warning(pop)
#endif

#include <unordered_map>
#include <optional>

using ArgNameMap = std::unordered_map<std::string, cl_uint>;

ArgNameMap mapNamesToArgIndices(const cl::Kernel& kernel);

using KernelSourceId = std::string;

using SettingsId = std::string;

struct KernelId {
    KernelSourceId src;
    SettingsId settings;

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
            auto h1 = std::hash <KernelSourceId> {} (id.src);
            auto h2 = std::hash <SettingsId> {} (id.settings);
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

struct KernelSettings {
//    std::optional<std::string> filesystemPath;
    cl::Program::Sources sourceCode;
    std::vector<std::string> compileOptions;
    cl::NDRange localRange = cl::NullRange;
};

struct KernelWithMetainfo {
    cl::Kernel kernel;
    cl_uint imageArgIdx;
    std::vector<cl_uint> dimensionalArgs;
    ArgNameMap param2name;
};

/**
 * Either Index or Name of kernel argument. Name is higher priority and checked first
 */
using ArgIdentifier = std::pair<std::optional<std::string>, std::optional<size_t>>;

enum class KernelArgType {
    I32, I64, F32, F64,
    V2F32, V2F64,
    V3F32, V3F64
};

/**
 * Kernel parameter which could be passed around by value.
 */
class KernelArg {
    const KernelArgType typeTag;

    union {
        cl_int i32;
        cl_long i64;
        cl_float f32;
        cl_double f64;
        cl_float2 v2f32;
        cl_double2 v2f64;
        cl_float3 v3f32;
        cl_double3 v3f64;
    };

public:

    KernelArg(int32_t v) : typeTag(KernelArgType::I32), i32(v) {}
    KernelArg(int64_t v) : typeTag(KernelArgType::I64), i64(v) {}
    KernelArg(float v) : typeTag(KernelArgType::F32), f32(v) {}
    KernelArg(double v) : typeTag(KernelArgType::F64), f64(v) {}
    KernelArg(float vx, float vy) : typeTag(KernelArgType::V2F32), v2f32( {vx, vy} ) {}
    KernelArg(double vx, double vy) : typeTag(KernelArgType::V2F64), v2f64( {vx, vy} ) {}
    KernelArg(float vx, float vy, float vz) : typeTag(KernelArgType::V3F32), v3f32( {vx, vy, vz} ) {}
    KernelArg(double vx, double vy, double vz) : typeTag(KernelArgType::V3F64), v3f64( {vx, vy, vz} ) {}

    void apply(cl::Kernel& kernel, cl_uint idx) const {
        switch (typeTag) {
            case KernelArgType::I32: kernel.setArg(idx, this->i32); break;
            case KernelArgType::I64: kernel.setArg(idx, this->i64); break;
            case KernelArgType::F32: kernel.setArg(idx, this->f32); break;
            case KernelArgType::F64: kernel.setArg(idx, this->f64); break;
            case KernelArgType::V2F32: kernel.setArg(idx, this->v2f32); break;
            case KernelArgType::V2F64: kernel.setArg(idx, this->v2f64); break;
            case KernelArgType::V3F32: kernel.setArg(idx, this->v3f32); break;
            case KernelArgType::V3F64: kernel.setArg(idx, this->v3f64); break;
            default:
                throw std::runtime_error("Type is not known");
        }
    }
};

/**
 * Map of kernel argument identifiers to actual kernel argument values
 */
using Args = std::unordered_map<ArgIdentifier, KernelArg>;

/**
 * Apply argument map to kernel.
 */
void applyArgsToKernel(KernelWithMetainfo& kernel, const Args& args);

/**
 * OpenCL Backend. Keeps everything related to OpenCL computations together.
 */
class OpenCLBackend {

    cl::Context ctx;

    cl::CommandQueue queue;

    std::unordered_map<KernelId, KernelSettings> registry_;

    std::unordered_map<CompilationContext, KernelWithMetainfo> compileCache_;

public:

    OpenCLBackend();

    inline cl::Context currentContext() noexcept { return ctx; }

    inline cl::CommandQueue currentQueue() noexcept { return queue; }

    KernelWithMetainfo compileKernel(KernelId id);

    const KernelSettings& findKernelSettings(KernelId id);

    void registerKernel(KernelId id, KernelSettings settings);

    void clearCache();

};

/**
 * Keystroke saviour
 */
using OpenCLBackendPtr = std::shared_ptr<OpenCLBackend>;

#endif //FRACTALEXPLORER_OPENCLBACKEND_HPP
