#include "OpenCLBackend.hpp"
#include "Utility.hpp"

cl_uint detectImageArgIdx(const ArgNameMap& map) {
    auto it = map.find("image");
    if (it != map.end()) {
        return it->second;
    }
    return 0;
}

std::vector<cl_uint> detectImageDimensionalArgIdxs(const ArgNameMap& map) {
    cl_uint width, height, depth;
    {
        auto it = map.find("width");
        width = it != map.end() ? it->second : 1;
    }
    {
        auto it = map.find("height");
        height = it != map.end() ? it->second : 2;
    }
    {
        auto it = map.find("width");
        depth = it != map.end() ? it -> second : 3;
    }
    return { width, height, depth };
}

ArgNameMap mapNamesToArgIndices(const cl::Kernel &kernel) {
    auto numArgs = kernel.getInfo<CL_KERNEL_NUM_ARGS>();
    ArgNameMap res;
    for (cl_uint i = 0; i < numArgs; ++i) {
        auto argName = kernel.getArgInfo<CL_KERNEL_ARG_NAME>(i);
        res[argName] = i;
    }
    return res;
}

void applyArgsToKernel(KernelWithMetainfo& kernel, const Args &args) {
    for (const auto& [argId, argValue] : args) {
        auto [name, idx] = argId;
        size_t val;
        if (name) {
            auto it = kernel.param2name.find(name.value());
            if (it == kernel.param2name.end()) {
                throw std::runtime_error("No argument named ...");
            }
            val = it->second;
        } else {
            if (!idx) {
                throw std::runtime_error("Invalid argument specification, neither name, nor index was provided");
            }
            val = idx.value();
        }
        argValue.apply(kernel.kernel, static_cast<cl_uint>(val));
    }
}

OpenCLBackend::OpenCLBackend() {
    ctx = cl::Context::getDefault();
    queue = cl::CommandQueue::getDefault();
}

KernelWithMetainfo OpenCLBackend::compileKernel(KernelId id) {
    auto foundInCache = this->compileCache_.find(CompilationContext { id, ctx });
    if (foundInCache != compileCache_.end()) {
        return foundInCache->second;
    }

    // not found in cache, need to compile
    const KernelSettings& kernelSettings = findKernelSettings(id);

    cl::Program prg (ctx, kernelSettings.sourceCode);

    // TODO validate && handle and rethrow exceptions here
    auto err = prg.build(join(kernelSettings.compileOptions.begin(), kernelSettings.compileOptions.end()).c_str());
    auto kernel = cl::Kernel { prg, id.src.c_str() };

    auto nameMap = mapNamesToArgIndices(kernel);

    KernelWithMetainfo result {
        kernel,
        detectImageArgIdx(nameMap),
        detectImageDimensionalArgIdxs(nameMap),
        nameMap
    };

    return result;
}

const KernelSettings &OpenCLBackend::findKernelSettings(KernelId id) {
    auto kernelSettingsIt = this->registry_.find(id);
    if (kernelSettingsIt == registry_.end()) {
        throw std::runtime_error(""); // TODO Meaningful exception;
    }
    return kernelSettingsIt->second;
}

void OpenCLBackend::registerKernel(KernelId id, KernelSettings settings) {
    // todo implement
}

void OpenCLBackend::clearCache() {
    compileCache_.clear();
}
