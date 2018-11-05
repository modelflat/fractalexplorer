#include <iostream>
#include <sstream>

#include <QOpenGLWidget>
#include <QApplication>
#include <QPushButton>
#include <app/ui/KernelArgWidget.hpp>
#include <app/ui/ComputableImageWidget.hpp>

#include "clc/CLC_Sources.hpp"
#include "ComputableImage.hpp"
#include "Utility.hpp"

LOGGER()

static OpenCLBackendPtr backend =
    std::make_shared<OpenCLBackend>();
static KernelArgConfigurationStoragePtr<NoUserProperties> confStorage =
    std::make_shared<KernelArgConfigurationStorage<NoUserProperties>>();

auto id = KernelId { "newton_fractal", "default" };

void registerDefaultAlgorithms(OpenCLBackendPtr backend) {
    cl::Program::Sources newton;
    newton.reserve(4);
    // TODO abstract?
    auto stdlib = sourcesRegistry.findById(STDLIB);
    newton.insert(std::end(newton), stdlib.begin(), stdlib.end());
    auto newtonSpecific = sourcesRegistry.findById("newton-fractal");
    newton.insert(std::end(newton), newtonSpecific.begin(), newtonSpecific.end());

    backend->registerKernel(
        id, KernelBase { newton, { "-DUSE_DOUBLE_PRECISION" } }
    );

    std::string_view str = R"(
        0    0 -1 1
        1    0 -1 1
        2    0 -1 1
        3    0 -1 1
        C    0 -1 1     0 -1 1
        5    0  0 1
        6    1  0 1
        7    0.4 -10 10
        8    100 1 1e16
        9    100 1 1e16
        10   0 0 1e16
        seed   1 0 1
    )";

    confStorage->registerConfiguration(id, str.data());
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    logger->info(fmt::format("CL: {}", cl::Platform::getDefault().getInfo<CL_PLATFORM_NAME>()));


    registerDefaultAlgorithms(backend);

    KernelInstance<> kernel = backend->compileKernel<NoUserProperties>({ "newton_fractal", "default" });

    auto honey = detectArgumentTypesAndNames(kernel.kernel());

    auto [strConf, conf] = confStorage->findOrParseConfiguration(id, honey);
    (void)strConf;

    ComputableImageWidget2D img (backend, {512, 512}, id);
    KernelArgWidget params (honey, conf);

    QSlider hello(nullptr);
    hello.show();

    return QApplication::exec();
}