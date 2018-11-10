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

//class NumberWidget : public ArgProviderWidget {
//
//    Q_OBJECT
//
//    KernelArgType valueType_;
//    QGroupBox* groupBox;
//    QLineEdit* textBox;
//
//public:
//
//    NumberWidget(
//        const QString& name,
//        KernelArgType type,
//        ArgProperties<UIProperties> conf,
//        QWidget* parent = nullptr);
//
//    std::optional<KernelArgValue> value() override;
//
//signals:
//
//    void valueChanged(KernelArgValue);
//
//};

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

//class VectorWidget : public ArgProviderWidget {
//
//    Q_OBJECT
//
//    size_t size_;
//    KernelArgType valueType_;
//    QString header_;
//    QVector<QPair<KernelArgValue, KernelArgValue>> minMaxPairs_;
//
//    QGroupBox* enclosingGroupBox_;
//    QVector<QLabel*> componentLabels_;
//    QVector<QSlider*> sliders_;
//    QPushButton* extrasButton_;
//
//public:
//
//    VectorWidget(
//        QString header, KernelArgType, ArgProperties<UIProperties>,
//        QVector<QPair<KernelArgValue, KernelArgValue>> constraints,
//        QWidget* parent = nullptr
//    );
//
//    std::optional<KernelArgValue> value() override;
//
//    void setConstraints(size_t componentIdx, QPair<KernelArgValue, KernelArgValue> newConstraints);
//
//    void setConstraints(QPair<KernelArgValue, KernelArgValue> newConstraints);
//
//signals:
//
//    void valueChanged(KernelArgValue);
//
//};
//

class KernelArgWidget : public QWidget {

    QVector<Slider*> argProviders;

public:

    KernelArgWidget(ArgsTypesWithNames argTypes, KernelArgProperties<UIProperties> conf, QWidget* parent = nullptr);

};

KernelArgWidget* makeParameterWidgetForKernel(KernelId, cl::Kernel, KernelArgConfigurationStoragePtr<UIProperties>);

#endif //FRACTALEXPLORER_KERNELARGWIDGET_HPP
