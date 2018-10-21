#include "OpenCLKernelUtils.hpp"

#include <fmt/format.h>

void setArg(cl::Kernel &kernel, cl_uint idx, const AnyType &value) {
    std::visit(
        [&kernel, idx](auto &&value) {
            kernel.setArg(idx, value);
        }, value
              );
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

void applyArgsToKernel(cl::Kernel &kernel, const KernelArgs &args) {
    auto nameMap = mapNamesToArgIndices(kernel);
    std::for_each(
        args.begin(), args.end(), [&](const auto &value) {
            std::visit(
                [&](auto &&arg) {
                    using Type = decltype(arg);
                    if constexpr (std::is_same_v<Type, std::string>) {
                        auto val = nameMap.find(arg);
                        if (val == nameMap.end()) {
                            throw std::invalid_argument(
                                fmt::format("Kernel has no argument named \"{}\"", arg)
                                                       );
                        }
                        value.second.apply(kernel, val->second);
                    } else if constexpr (std::is_same_v<Type, size_t>) {
                        value.second.apply(kernel, static_cast<cl_uint>(arg));
                    }
                }, value.first
                      );
        }
                 );
}
