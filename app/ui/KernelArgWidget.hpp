#ifndef FRACTALEXPLORER_KERNELARGWIDGET_HPP
#define FRACTALEXPLORER_KERNELARGWIDGET_HPP

#include <app/core/OpenCLKernelUtils.hpp>

#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QtWidgets/QPushButton>

struct IArgProvider {
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
        QWidget* parent = nullptr);

    std::optional<KernelArgValue> value() override;

signals:

    void valueChanged(KernelArgValue);

};

class VectorWidget : public ArgProviderWidget {

    Q_OBJECT

    size_t size_;
    KernelArgType valueType_;
    QString header_;
    QVector<QPair<KernelArgValue, KernelArgValue>> minMaxPairs_;

    QGroupBox* enclosingGroupBox_;
    QVector<QLabel*> componentLabels_;
    QVector<QSlider*> sliders_;
    QPushButton* extrasButton_;

public:

    VectorWidget(
        size_t size, KernelArgType valueType, QString header,
        QVector<QPair<KernelArgValue, KernelArgValue>> constraints
    );

    VectorWidget(
        KernelArgType valueType, QString header, QPair<KernelArgValue, KernelArgValue> constraints
    );

    std::optional<KernelArgValue> value() override;

    void setConstraints(size_t componentIdx, QPair<KernelArgValue, KernelArgValue> newConstraints);

    void setConstraints(QPair<KernelArgValue, KernelArgValue> newConstraints);

signals:

    void valueChanged(KernelArgValue);

};

struct UIProperties {

    bool hidden;

    static UIProperties fromStream(std::istream& str) {
        UIProperties res;
        int t; str >> t;
        res.hidden = t != 0;
        return res;
    }

};

class KernelArgWidget : public QWidget {

    QVector<ArgProviderWidget*> argProviders;

public:

    KernelArgWidget(ArgsTypesWithNames&& argTypes, KernelArgProperties<UIProperties> conf, QWidget* parent = nullptr);

};

#endif //FRACTALEXPLORER_KERNELARGWIDGET_HPP
