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
        default:
            logger->warn(fmt::format("KernelArgType::<{}> is not a numeric value, can't parse", (int)valueType));
            return {};
    }
}

NumberWidget::NumberWidget(const QString& name, KernelArgType valueType, std::optional<KernelArgValue> minValue,
    std::optional<KernelArgValue> maxValue, QWidget *parent) : ArgProviderWidget(parent), valueType_(valueType) {

    groupBox = new QGroupBox(name, this);
    auto* lines = new QVBoxLayout;
    textBox = new QLineEdit;
    lines->addWidget(textBox);
    lines->addStretch(1);

    groupBox->setLayout(lines);

    connect(textBox, &QLineEdit::textEdited, [this](const QString& text){
        if (!text.isEmpty() && !text.trimmed().isEmpty()) {
            auto value = parseNumericValue(valueType_, text.trimmed().toStdString());
            if (value) {
                emit valueChanged(value.value());
            }
        }
    });

    connect(this, &NumberWidget::valueChanged, [](auto val) {
    });
}

std::optional<KernelArgValue> NumberWidget::value() {
    return std::optional<KernelArgValue>();
}

