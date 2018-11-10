#ifndef FRACTALEXPLORER_KERNELARGWIDGET_HPP
#define FRACTALEXPLORER_KERNELARGWIDGET_HPP

#include <app/core/OpenCLKernelUtils.hpp>

#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QtWidgets/QPushButton>
#include <app/core/OpenCLBackend.hpp>

struct IArgProvider {
    virtual std::optional<KernelArgValue> value() = 0;
};

class ArgProviderWidget : public QWidget, public IArgProvider {
public:
    explicit ArgProviderWidget(QWidget* parent) : QWidget(parent), IArgProvider() {}
};

class Slider : public ArgProviderWidget {

    Q_OBJECT

    KernelArgType type_;
    std::vector<QSlider*> sliders_;

public:

    Slider(KernelArgType type, KernelArgValue min, KernelArgValue max, KernelArgValue def, QWidget* parent = nullptr);

    std::optional<KernelArgValue> value() override;

signals:

    void valueChanged(KernelArgValue);

};

class KernelArgWidget : public QWidget {

    Q_OBJECT

    QVector<Slider*> argProviders;
    KernelArgs cachedArgValues;

public:

    KernelArgWidget(ArgsTypesWithNames argTypes, KernelArgProperties<UIProperties> conf, QWidget* parent = nullptr);

signals:

    void valuesChanged(KernelArgs);

};

KernelArgWidget* makeParameterWidgetForKernel(KernelId, cl::Kernel, KernelArgConfigurationStoragePtr<UIProperties>);

#endif //FRACTALEXPLORER_KERNELARGWIDGET_HPP
