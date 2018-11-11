#include <iostream>
#include <sstream>

#include <QOpenGLWidget>
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <app/ui/KernelArgWidget.hpp>
#include <app/ui/ComputableImageWidget.hpp>

#include "clc/CLC_Sources.hpp"
#include "ComputableImage.hpp"
#include "Utility.hpp"

LOGGER()

static OpenCLBackendPtr backend =
    std::make_shared<OpenCLBackend>();
static auto confStorage = std::make_shared<KernelArgConfigurationStorage<UIProperties>>();

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
        0    0 -1 1                 0
        1    0 -1 1                 0
        2    0 -1 1                 0
        3    0 -1 1                 0
        C    0 -1 1     0 -1 1      0
        5    0  0 1                 0
        6    1  0 1                 0
        7    0.4 -10 10             0
        runs_count 1 1 1            1
        points_count 256 1 8192     0
        iter_skip   0 0 8192        0
        seed    1 0 1               1
        12      0 0 0 0 0 0 0 0 0   1
        13                          1
    )";

    confStorage->registerConfiguration(id, str.data());
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    logger->info(fmt::format("CL: {}", cl::Platform::getDefault().getInfo<CL_PLATFORM_NAME>()));

    registerDefaultAlgorithms(backend);

    QMainWindow w;

    ParameterizedComputableImageWidget img(backend, confStorage, {512, 512}, id);

    w.setCentralWidget(&img);

    w.show();

    return QApplication::exec();
}