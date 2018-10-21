#include "OpenCLKernelUtils.hpp"

#include "Utility.hpp"

LOGGER()

std::string to_string(const cl::Kernel &kernel) {
    return fmt::format("Kernel[{}]", kernel.getInfo<CL_KERNEL_FUNCTION_NAME>());
}

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

static std::unordered_map<std::string, KernelArgType> kernelArgTypenameMap {
    {
        { "int", KernelArgType::Int32 },
        { "uint", KernelArgType::Int32 },
        { "long", KernelArgType::Int64 },
        { "ulong", KernelArgType::Int64 },
        { "float", KernelArgType::Float32 },
        { "double", KernelArgType::Float64 },
        { "float2", KernelArgType::Vector2Float32 },
        { "double2", KernelArgType::Vector2Float64 },
        { "float3", KernelArgType::Vector3Float32 },
        { "double3", KernelArgType::Vector3Float64 },
        { "image1d_t", KernelArgType::Image },
        { "image2d_t", KernelArgType::Image },
        { "image3d_t", KernelArgType::Image }

    }
};

std::pair<KernelArgType, std::string> detectKernelArgType(const cl::Kernel& kernel, cl_uint idx) {
    auto argName = kernel.getArgInfo<CL_KERNEL_ARG_TYPE_NAME>(idx);
    auto found = kernelArgTypenameMap.find(argName);
    if (found != kernelArgTypenameMap.end()) {
        return { found->second, argName };
    }
    auto argQualifier = kernel.getArgInfo<CL_KERNEL_ARG_ADDRESS_QUALIFIER>(idx);
    if (argQualifier != CL_KERNEL_ARG_ADDRESS_PRIVATE) {
        return { KernelArgType ::Buffer, argName };
    }
    return { KernelArgType::Unknown, argName };
}

std::vector<KernelArgType> detectArgumentTypes(const cl::Kernel &kernel) {
    logger->info(fmt::format("Detecting types for {}", to_string(kernel)));

    auto argc = kernel.getInfo<CL_KERNEL_NUM_ARGS>();
    std::vector<KernelArgType> types;
    types.reserve(argc);
    for (cl_uint i = 0; i < argc; ++i) {
        auto [type, typeStr] = detectKernelArgType(kernel, i);
        types.push_back(type);

        logger->info(
            fmt::format("\tArg #{} ({} {}) : {}", i, typeStr, kernel.getArgInfo<CL_KERNEL_ARG_NAME>(i), (int)type)
        );
    }
    return types;
}
