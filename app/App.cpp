#include <iostream>

#include <QOpenGLWidget>
#include <QApplication>
#include <QPushButton>

#include "clc/CLC_Sources.hpp"
#include "ComputableImage.hpp"

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
        id, KernelSettings { newton, { "" } }
    );
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    std::cout << "CL: " << cl::Platform::getDefault().getInfo<CL_PLATFORM_NAME>() << std::endl;

    QPushButton hello("Hello world!", nullptr);
    hello.resize(640, 480);
    hello.show();

    OpenCLBackendPtr backend = std::make_shared<OpenCLBackend>();

    registerDefaultAlgorithms(backend);

    OpenCLComputableImage<Dim_2D> img(backend, Range<2>{512, 512});

    try {
        backend->compileKernel(id);
    } catch (const std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    img.clear(backend);

    backend->currentQueue().finish();

    return QApplication::exec();
}