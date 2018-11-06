#include "KernelArgWidget.hpp"
#include "app/core/Utility.hpp"

#include <QVBoxLayout>

LOGGER()

template <typename T>
T getFromStream(std::istream& ss, bool& err)  {
    T val; ss >> val;
    err |= ss.fail();
    return val;
}

template <typename ...Ts>
std::optional<KernelArgValue> parseValue(const std::string& string) {
    std::istringstream iss { string };
    bool error = false;
    KernelArgValue value { getFromStream<Ts>(iss, error)... };
    return value;
}

std::optional<KernelArgValue> parseNumericValue(KernelArgType valueType, const std::string& string) {
    switch (valueType) {
        case KernelArgType::Float32:
            return parseValue<float>(string);
        case KernelArgType::Float64:
            return parseValue<double>(string);
        case KernelArgType::Int32:
            return parseValue<int32_t>(string);
        case KernelArgType::Int64:
            return parseValue<int64_t>(string);
        case KernelArgType::Vector2Float32:
            return parseValue<float, float>(string);
        case KernelArgType::Vector2Float64:
            return parseValue<double, double>(string);
        case KernelArgType::Vector3Float32:
            return parseValue<float, float, float>(string);
        case KernelArgType::Vector3Float64:
            return parseValue<double, double, double>(string);
        default:
            logger->warn(fmt::format("KernelArgType::<{}> is not a numeric value, can't parse", (int)valueType));
            return {};
    }
}

NumberWidget::NumberWidget(const QString& name, KernelArgType valueType, QWidget *parent)
: ArgProviderWidget(parent), valueType_(valueType) {
    groupBox = new QGroupBox(name, this);
    auto* lines = new QVBoxLayout;
    textBox = new QLineEdit;
    lines->addWidget(textBox);
    lines->addStretch(1);

    groupBox->setLayout(lines);

    connect(textBox, & QLineEdit::textEdited, [this](const QString& text){
        auto value = this->value();
        if (value) { emit valueChanged(*value); }
    });
}

std::optional<KernelArgValue> NumberWidget::value() {
    auto text = this->textBox->text().trimmed();
    if (text.isEmpty()) { return {}; }
    return parseNumericValue(valueType_, text.toStdString());
}

KernelArgWidget::KernelArgWidget(
    ArgsTypesWithNames argTypes, KernelArgProperties<UIProperties> conf,
    QWidget *parent) : QWidget(parent) {

    std::transform(argTypes.begin(), argTypes.end(), std::back_inserter(argProviders), [this](auto& argTypeAndName) {
        auto [type, name] = argTypeAndName;
        return new NumberWidget( QString::fromStdString(name), type, this );
    });

    auto* layout = new QVBoxLayout;
    std::for_each(argProviders.begin(), argProviders.end(), [layout](auto* pr) {
        layout->addWidget(pr);
    });

    this->setLayout(layout);
}

KernelArgWidget* makeParameterWidgetForKernel(
    KernelId id, cl::Kernel kernel, KernelArgConfigurationStoragePtr<UIProperties> confStorage
) {
    auto honey = detectArgumentTypesAndNames(kernel);
    auto conf = confStorage->findOrParseConfiguration(id, honey);
    return new KernelArgWidget(honey, conf);
}
