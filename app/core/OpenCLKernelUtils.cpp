#include "OpenCLKernelUtils.hpp"

#include "Utility.hpp"

LOGGER()

static const std::unordered_map<std::string, KernelArgType> kernelArgTypenameMap {
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

static const std::unordered_map<KernelArgType, KernelArgTypeTraits> typeTraitsMap {
    {
        { KernelArgType::Int32,          { 1, [](const std::vector<Primitive>& vec){
            return static_cast<int32_t>(vec[0]);
        }, KernelArgTypeClass::Integer }},
        { KernelArgType::Int64,          { 1, [](const std::vector<Primitive>& vec){
            return static_cast<int64_t>(vec[0]);
        }, KernelArgTypeClass::Integer }},
        { KernelArgType::Float32,        { 1, [](const std::vector<Primitive>& vec){
            return static_cast<float>(vec[0]);
        }, KernelArgTypeClass::FloatingPoint }},
        { KernelArgType::Float64,        { 1, [](const std::vector<Primitive>& vec) {
            return static_cast<double>(vec[0]);
        }, KernelArgTypeClass::FloatingPoint }},
        { KernelArgType::Vector2Float32, { 2, [](const std::vector<Primitive>& vec){
            return cl_float2 { static_cast<float>(vec[0]), static_cast<float>(vec[1]) };
        }, KernelArgTypeClass::FloatingPoint }},
        { KernelArgType::Vector2Float64, { 2, [](const std::vector<Primitive>& vec){
            return cl_double2 { static_cast<double>(vec[0]), static_cast<double>(vec[1]) };
        }, KernelArgTypeClass::FloatingPoint }},
        { KernelArgType::Vector3Float32, { 3, [](const std::vector<Primitive>& vec){
            return cl_float3 { static_cast<float>(vec[0]), static_cast<float>(vec[1]), static_cast<float>(vec[2]) };
        }, KernelArgTypeClass::FloatingPoint }},
        { KernelArgType::Vector3Float64, { 3, [](const std::vector<Primitive>& vec){
            return cl_double3 { static_cast<double>(vec[0]), static_cast<double>(vec[1]), static_cast<double>(vec[2]) };
        }, KernelArgTypeClass::FloatingPoint }},
        { KernelArgType::Image,          { 0, [](const std::vector<Primitive>& vec){
            return vec.empty();
        }, KernelArgTypeClass::Memory }},
        { KernelArgType::Buffer,         { 0, [](const std::vector<Primitive>& vec){
            return vec.empty();
        }, KernelArgTypeClass::Memory }}
    }
};

KernelArgTypeTraits findTypeTraits(KernelArgType type) {
    auto it = typeTraitsMap.find(type);
    if (it == typeTraitsMap.end()) {
        auto err = fmt::format("Unable to find type traits for typeid {}", static_cast<int>(type) );
        logger->error(err);
        throw std::runtime_error(err);
    }
    return it->second;
}

AnyScalarType getVectorComponent(size_t idx, AnyType val) {
    return std::visit([idx](auto val) {
        using T = decltype(val);
        bool error = false;
        if constexpr (
            std::is_same_v<T, cl_int>
            || std::is_same_v<T, cl_long>
            || std::is_same_v<T, cl_float>
            || std::is_same_v<T, cl_double>
        ) {
            if (idx == 0) {
                return AnyScalarType { val };
            } else {
                error = true;
            }
        }
        if constexpr (
            std::is_same_v<T, cl_float2>
            || std::is_same_v<T, cl_double2>
        ) {
            if (idx <= 1) {
                return AnyScalarType { val.s[idx] };
            } else {
                error = true;
            }
        }
        if constexpr (
            std::is_same_v<T, cl_float3>
            || std::is_same_v<T, cl_double3>
            ) {
            if (idx <= 2) {
                return AnyScalarType { val.s[idx] };
            } else {
                error = true;
            }
        }
        if (error) {
            auto err = fmt::format("Index out of range: {}", idx);
            logger->error(err);
            throw std::runtime_error(err);
        }

        logger->error("std::visit visitor is not exhaustive");
        return AnyScalarType { 0 };
    }, val);
}

std::string to_string(const cl::Kernel &kernel) {
    return fmt::format("Kernel[{}]", kernel.getInfo<CL_KERNEL_FUNCTION_NAME>());
}

std::string to_string(KernelArgValue val) {
    std::ostringstream str {};
    str << "KernelArgValue { " ;
    std::visit([&str](auto val) mutable {
        using T = decltype(val);
        if constexpr (
            std::is_same_v<T, cl_int>
            || std::is_same_v<T, cl_long>
            || std::is_same_v<T, cl_double>
            || std::is_same_v<T, cl_float>
        ) {
            str << val;
            return;
        }
        if constexpr (
            std::is_same_v<T, cl_float2>
            || std::is_same_v<T, cl_double2>
        ) {
            str << val.x << ',' << val.y;
            return;
        }
        if constexpr (
            std::is_same_v<T, cl_float3>
            || std::is_same_v<T, cl_double3>
        ) {
            str << val.x << ',' << val.y << ',' << val.z;
            return;
        }
        str << "<something>";
    }, val.value);
    str << " }";
    return str.str();
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

std::pair<KernelArgType, std::string> detectKernelArgType(const cl::Kernel& kernel, cl_uint idx) {
    auto argName = kernel.getArgInfo<CL_KERNEL_ARG_TYPE_NAME>(idx);
    auto found = kernelArgTypenameMap.find(argName);
    if (found != kernelArgTypenameMap.end()) {
        return { found->second, argName };
    }
    auto argQualifier = kernel.getArgInfo<CL_KERNEL_ARG_ADDRESS_QUALIFIER>(idx);
    if (argQualifier != CL_KERNEL_ARG_ADDRESS_PRIVATE) {
        return { KernelArgType::Buffer, argName };
    }
    return { KernelArgType::Unknown, argName };
}

ArgsTypesWithNames detectArgumentTypesAndNames(const cl::Kernel &kernel) {
    logger->info(fmt::format("Detecting types for {}", to_string(kernel)));

    auto argc = kernel.getInfo<CL_KERNEL_NUM_ARGS>();
    std::vector<std::pair<KernelArgType, std::string>> types;
    types.reserve(argc);
    for (cl_uint i = 0; i < argc; ++i) {
        auto [type, typeStr] = detectKernelArgType(kernel, i);
        types.emplace_back( type, kernel.getArgInfo<CL_KERNEL_ARG_NAME>(i) );

        logger->info(
            fmt::format("\tArg #{} ({} {}) : {}", i, typeStr, kernel.getArgInfo<CL_KERNEL_ARG_NAME>(i), (int)type)
        );
    }

    return types;
}

/**
 * Helper function to fetch build log for a program.
 */
std::string getBuildLog(const cl::Context &ctx, const cl::Program& prg) {
    std::stringstream ss;
    auto devs = ctx.getInfo<CL_CONTEXT_DEVICES>();
    std::for_each(devs.begin(), devs.end(), [&ss, &prg] (cl::Device device) {
        auto log = prg.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        if (!std::all_of(log.begin(), log.end(), [](auto&& arg) {return std::isspace(arg);})) {
            ss << "=== Build log for device " << device.getInfo<CL_DEVICE_NAME>() << ":\n";
            ss << log << '\n';
        }
    });
    return ss.str();
}

KernelBase::KernelBase(
    cl::Program::Sources sourceCode,
    std::vector<std::string> compileOptions)
    :   sourceCode_(std::move(sourceCode)),
        compileOptions_(std::move(compileOptions))
    {}

cl::Program KernelBase::build(const cl::Context& ctx) const {
    cl::Program prg (ctx, sourceCode_);

    auto options = absl::StrJoin(compileOptions_.begin(), compileOptions_.end(), " ");

    logger->info(fmt::format("Building program with options: {}", options));

    try {
        prg.build(options.c_str());
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
    logger->info(
        fmt::format("Build logs:{}{}",
            buildLog.empty() ? " " : "\n",
            buildLog.empty() ? "all clear!" : buildLog)
                );

    return prg;
}

cl_uint detectImageArgIdx(const ArgNameMap& map) {
    auto it = map.find("image");
    if (it != map.end()) {
        return it->second;
    }
    return 0;
}

std::vector<cl_uint> detectImageDimensionalArgIdxs(const ArgNameMap& map) {
    std::vector<cl_uint> res;
    res.reserve(4);
    {
        auto it = map.find("width");
        if (it != map.end()) res.push_back(it->second);
    }
    {
        auto it = map.find("height");
        if (it != map.end()) res.push_back(it->second);
    }
    {
        auto it = map.find("depth");
        if (it != map.end()) res.push_back(it->second);
    }
    return res;
}

/**
 * Retrieve a value of given type from input stream, or empty optional if parsing failed.
 */
template <typename T>
std::optional<T> getFromStreamSingle(std::istream& ss)  {
    T val; ss >> val;
    if (ss.fail()) return {};
    return val;
}

/**
 * Consume the first token of stream as name or index.
 */
std::variant<cl_uint, std::string> nameOrIndex(std::istream& str) {
    auto v1 = getFromStreamSingle<cl_uint>(str);
    if (v1) {
        return *v1;
    }
    str.clear();
    auto v2 = getFromStreamSingle<std::string>(str);
    return v2.value();
}

template <typename ...Ts>
std::tuple<std::optional<Ts>...> getFromStream(std::istream& ss) {
    return std::make_tuple<std::optional<Ts>...>( getFromStreamSingle<Ts>(ss)... );
}

template <typename ...Ts, typename ...Vectors>
std::tuple<Ts...> constructFromVectors(KernelArgType type, Vectors&& ... vectors) {
    auto convert = findTypeTraits(type).fromVector;
    return std::make_tuple( convert(vectors)... );
}

std::tuple<
    cl_uint, std::tuple<KernelArgValue, KernelArgValue, KernelArgValue>
> parseBasicProperties(const ArgsTypesWithNames &args, std::istream &str) {
    cl_uint index;
    std::visit([&](auto val) {
        if constexpr (std::is_same_v<decltype(val), cl_uint>) {
            index = val;
        }
        if constexpr (std::is_same_v<decltype(val), std::string>) {
            auto it = std::find_if(args.begin(), args.end(), [&val](auto arg) {
                return arg.second == val;
            });
            if (it == args.end()) {
                auto err = fmt::format("Cannot find argument named '{}'", val);
                logger->error(err);
                throw PropertyParsingError(err);
            }
            index = static_cast<cl_uint>(std::distance(args.begin(), it));
        }
    }, nameOrIndex(str));

    if (index >= args.size()) {
        auto err = fmt::format("Argument index is out of range ({}/{})", index, args.size());
        logger->error(err);
        throw PropertyParsingError(err);
    }

    auto [type, name] = args[index];
    logger->info(fmt::format("Parsing def/min/max values for argument {} (#{})", name, index));

    auto traits = findTypeTraits(type);

    std::vector<Primitive> defValues; defValues.reserve(traits.numComponents);
    std::vector<Primitive> minValues; minValues.reserve(traits.numComponents);
    std::vector<Primitive> maxValues; maxValues.reserve(traits.numComponents);
    for (size_t i = 0; i < traits.numComponents; ++i) {
        auto [max, min, def] = getFromStream<Primitive, Primitive, Primitive>(str);
        if (!def || !min || !max) {
            auto err = fmt::format("Cannot parse def/min/max for component #{} of argument {} (#{})", i, name, index);
            logger->error(err);
            throw PropertyParsingError(err);
        }
        logger->info(fmt::format("\tComponent #{}: def/min/max = {}/{}/{}", i, *def, *min, *max));
        defValues.push_back(*def);
        minValues.push_back(*min);
        maxValues.push_back(*max);
    }

    return { index, constructFromVectors
        <KernelArgValue, KernelArgValue, KernelArgValue>
        (type, std::move(defValues), std::move(minValues), std::move(maxValues))
    };
}

size_t findArgIndex(std::string_view argName, const ArgNameMap& map) {
    auto it = map.find(argName.data());
    if (it == map.end()) {
        auto err = fmt::format("No such argument: '{}'", argName);
        logger->error(err);
        throw std::runtime_error(err);
    }
    return it->second;
}

