#include "OpenCLBackend.hpp"
#include "Utility.hpp"
#include "OpenCLKernelUtils.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

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
    try {
        prg.build(join(
            kernelSettings.compileOptions.begin(),
            kernelSettings.compileOptions.end()).c_str());
    } catch (const cl::Error& cle) {
        std::stringstream ss;

        ss << "OpenCL error has occurred while building program. ";
        ss << "Error code is " << cle.err()
        << ", and the error says '" << cle.what() << "'\n";

        auto devs = ctx.getInfo<CL_CONTEXT_DEVICES>();
        std::for_each(devs.begin(), devs.end(), [&ss, &prg] (cl::Device device) {
            ss << "Build options: " << prg.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(device) << '\n';
            ss << "Build log for device " << device.getInfo<CL_DEVICE_NAME>() << ":\n\n";
            ss << prg.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << '\n';
        });

        throw std::runtime_error(ss.str().c_str());
    }

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
        throw std::runtime_error("No kernel with such id"); // TODO Meaningful exception;
    }
    return kernelSettingsIt->second;
}

void OpenCLBackend::registerKernel(KernelId id, KernelSettings&& settings) {
    registry_.try_emplace(id, std::forward<KernelSettings>(settings));
}

void OpenCLBackend::clearCache() {
    compileCache_.clear();
}
