#ifndef FRACTALEXPLORER_KERNELARGWIDGET_HPP
#define FRACTALEXPLORER_KERNELARGWIDGET_HPP

#include <app/core/OpenCLKernelUtils.hpp>

#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>

#include <algorithm>
#include <vector>

class IArgProvider {
    virtual std::optional<KernelArgValue> value() = 0;
};

class ArgProviderWidget : public QWidget, public IArgProvider {
public:
    explicit ArgProviderWidget(QWidget* parent) : QWidget(parent), IArgProvider() {}
};

class NumberWidget : public ArgProviderWidget {

    Q_OBJECT

    KernelArgType valueType_;
    QGroupBox* groupBox;
    QLineEdit* textBox;

public:

    NumberWidget(
        const QString& name,
        KernelArgType valueType,
        std::optional<KernelArgValue> minValue,
        std::optional<KernelArgValue> maxValue,
        QWidget* parent = nullptr);

private:
    virtual std::optional<KernelArgValue> value();

signals:

    void valueChanged(KernelArgValue);

};

class KernelArgWidget : public QWidget {

    QVector<std::unique_ptr<ArgProviderWidget>> argProviders;

public:

    KernelArgWidget(std::vector<KernelArgType>&& argTypes, QWidget* parent);

};


#endif //FRACTALEXPLORER_KERNELARGWIDGET_HPP
