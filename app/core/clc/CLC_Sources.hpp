#ifndef FRACTALEXPLORER_CLSOURCES_HPP
#define FRACTALEXPLORER_CLSOURCES_HPP

#include <string>

#include <CL/cl.hpp>

cl::Program compileProgramWithCommonSources(cl::Context ctx, cl::Program::Sources sources, std::string options);

cl::Program compileBuiltInAlgorithm(cl::Context ctx, std::string name, std::string options);

#endif //FRACTALEXPLORER_CLSOURCES_HPP

