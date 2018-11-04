#include <iostream>

#include <QOpenGLWidget>
#include <QApplication>
#include <QPushButton>
#include <app/ui/KernelArgWidget.hpp>

#include "clc/CLC_Sources.hpp"
#include "ComputableImage.hpp"
#include "Utility.hpp"

LOGGER()

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
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    logger->info(fmt::format("CL: {}", cl::Platform::getDefault().getInfo<CL_PLATFORM_NAME>()));

    OpenCLBackendPtr backend = std::make_shared<OpenCLBackend>();
    registerDefaultAlgorithms(backend);

    auto kernel = backend->compileKernel({ "newton_fractal", "default" });

    KernelArgWidget hello(detectArgumentTypesAndNames(kernel.kernel), nullptr);
    hello.show();

    return QApplication::exec();
}