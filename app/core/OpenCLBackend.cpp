#include "OpenCLBackend.hpp"
#include "Utility.hpp"
#include "OpenCLKernelUtils.hpp"

#include <algorithm>

#include "Utility.hpp"

LOGGER()

OpenCLBackend::OpenCLBackend() {
    ctx = cl::Context::getDefault();
    queue = cl::CommandQueue::getDefault();
}

cl::Kernel OpenCLBackend::compileCLKernel(KernelId id) {
    auto foundInCache = this->compileCache_.find(CompilationContext { id, ctx });
    if (foundInCache != compileCache_.end()) {
        logger->info(
            fmt::format("Kernel ({}, {}) was found in cache!", id.src, id.settings)
                    );
        return foundInCache->second;
    }

    logger->info(fmt::format("Kernel ({}, {}) not in cache, compiling...", id.src, id.settings));

    // build a kernel from base
    return this->compileCache_.try_emplace(
        CompilationContext {id, ctx}, findKernelBase(id).build(ctx), id.src.c_str()
    ).first->second;
}

const KernelBase &OpenCLBackend::findKernelBase(KernelId id) const {
    const auto kernelSettingsIt = this->registry_.find(id);
    if (kernelSettingsIt == registry_.end()) {
        auto err = fmt::format("No kernel with id {{{}, {}}} was found in registry!", id.src, id.settings);
        logger->error(err);
        throw std::runtime_error(err);
    }
    return kernelSettingsIt->second;
}

void OpenCLBackend::registerKernel(KernelId id, KernelBase&& base) {
    auto res = registry_.try_emplace(id, std::forward<KernelBase>(base));
    logger->info(
        fmt::format("Kernel ({}, {}) was {}", id.src, id.settings, res.second ? "REGISTERED" : "UPDATED")
    );
}

void OpenCLBackend::clearCache() {
    logger->info("Clearing CL backend cache...");
    compileCache_.clear();
}
