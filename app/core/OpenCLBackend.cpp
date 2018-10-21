#include "OpenCLBackend.hpp"
#include "Utility.hpp"
#include "OpenCLKernelUtils.hpp"

#include <algorithm>

#include "Utility.hpp"

LOGGER()

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

std::string getBuildLog(const cl::Context &ctx, const cl::Program& prg) {
    std::stringstream ss;
    auto devs = ctx.getInfo<CL_CONTEXT_DEVICES>();
    std::for_each(devs.begin(), devs.end(), [&ss, &prg] (cl::Device device) {
        ss << "=== Options: " << prg.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(device) << '\n';
        auto log = prg.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        if (!std::all_of(log.begin(), log.end(), [](auto&& arg) {return std::isspace(arg);})) {
            ss << "=== Build log for device " << device.getInfo<CL_DEVICE_NAME>() << ":\n";
            ss << log << '\n';
        } else {
            ss << "=== Empty build log for device " << device.getInfo<CL_DEVICE_NAME>() << "\n";
        }
    });
    return ss.str();
}

OpenCLBackend::OpenCLBackend() {
    ctx = cl::Context::getDefault();
    queue = cl::CommandQueue::getDefault();
}

KernelWithMetainfo OpenCLBackend::compileKernel(KernelId id) {
    logger->info(
        fmt::format("Kernel compilation requested ({}, {}). Looking in cache...", id.src, id.settings)
    );
    auto foundInCache = this->compileCache_.find(CompilationContext { id, ctx });
    if (foundInCache != compileCache_.end()) {
        logger->info(
            fmt::format("Kernel ({}, {}) was found in cache!", id.src, id.settings)
        );
        return foundInCache->second;
    }

    logger->info("Not in cache. Compiling kernel...");
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

        ss << getBuildLog(ctx, prg);

        auto err = ss.str();
        logger->error(err);
        throw std::runtime_error(err.c_str());
    }

    logger->info("Program successfully built");
    auto buildLog = getBuildLog(ctx, prg);
    if (!buildLog.empty()) {
        logger->info(fmt::format("Printing build log:\n{}", buildLog));
    }

    auto kernel = cl::Kernel { prg, id.src.c_str() };

    auto nameMap = mapNamesToArgIndices(kernel);

    detectArgumentTypes(kernel);

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
        auto err = fmt::format("No kernel with id {{{}, {}}} was found in registry!", id.src, id.settings);
        logger->error(err);
        throw std::runtime_error(err);
    }
    return kernelSettingsIt->second;
}

void OpenCLBackend::registerKernel(KernelId id, KernelSettings&& settings) {
    auto res = registry_.try_emplace(id, std::forward<KernelSettings>(settings));
    logger->info(
        fmt::format("Kernel ({}, {}) was {}", id.src, id.settings, res.second ? "REGISTERED" : "UPDATED")
    );
}

void OpenCLBackend::clearCache() {
    logger->info("Clearing CL backend cache...");
    compileCache_.clear();
}
