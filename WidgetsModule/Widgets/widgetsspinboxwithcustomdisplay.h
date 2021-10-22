#ifndef WIDGETSSPINBOXWITHCUSTOMDISPLAY_H
#define WIDGETSSPINBOXWITHCUSTOMDISPLAY_H

#include <QSpinBox>

#include <PropertiesModule/internal.hpp>

class WidgetsSpinBoxWithCustomDisplay : public QSpinBox
{
    using Super = QSpinBox;
public:
    using ValueFromTextHandler = std::function<qint32 (const WidgetsSpinBoxWithCustomDisplay* spinBox, const QString&)>;
    using TextFromValueHandler = std::function<QString (const WidgetsSpinBoxWithCustomDisplay* spinBox, qint32)>;
    WidgetsSpinBoxWithCustomDisplay(QWidget* parent = nullptr);

    static const ValueFromTextHandler& GetDefaultValueFromTextHandler();
    static const TextFromValueHandler& GetDefaultTextFromValueHandler();

    void SetHandlers(const TextFromValueHandler& textFromValueHandler, const ValueFromTextHandler& valueFromTextHandler)
    {
        m_textFromValueHandler = textFromValueHandler;
        m_valueFromTextHandler = valueFromTextHandler;
    }

    void SetTextFromValueHandler(const TextFromValueHandler& textFromValueHandler)
    {
        m_textFromValueHandler = textFromValueHandler;
    }

    void SetValueFromTextHandler(const ValueFromTextHandler& valueFromTextHandler)
    {
        m_valueFromTextHandler = valueFromTextHandler;
    }

    DispatcherConnection MakeOptional(LocalPropertyBool* valid);

private:
    QString textFromValue(int val) const override;
    qint32 valueFromText(const QString& text) const override;
    QValidator::State validate(QString& input, int& pos) const override;

private:
    TextFromValueHandler m_textFromValueHandler;
    ValueFromTextHandler m_valueFromTextHandler;
    bool m_emptyInputIsValid;
};

class WidgetsDoubleSpinBoxWithCustomDisplay : public QDoubleSpinBox
{
    using Super = QDoubleSpinBox;
public:
    using ValueFromTextHandler = std::function<double (const WidgetsDoubleSpinBoxWithCustomDisplay* spinBox, const QString&)>;
    using TextFromValueHandler = std::function<QString (const WidgetsDoubleSpinBoxWithCustomDisplay* spinBox, double)>;
    WidgetsDoubleSpinBoxWithCustomDisplay(QWidget* parent = nullptr);

    static const ValueFromTextHandler& GetDefaultValueFromTextHandler();
    static const TextFromValueHandler& GetDefaultTextFromValueHandler();

    void SetHandlers(const TextFromValueHandler& textFromValueHandler, const ValueFromTextHandler& valueFromTextHandler)
    {
        m_textFromValueHandler = textFromValueHandler;
        m_valueFromTextHandler = valueFromTextHandler;
    }

    void SetTextFromValueHandler(const TextFromValueHandler& textFromValueHandler)
    {
        m_textFromValueHandler = textFromValueHandler;
    }

    void SetValueFromTextHandler(const ValueFromTextHandler& valueFromTextHandler)
    {
        m_valueFromTextHandler = valueFromTextHandler;
    }

    DispatcherConnection MakeOptional(LocalPropertyBool* valid);
    void MakeOptional();

    bool IsValid() const;

private:
    QString textFromValue(double val) const override;
    double valueFromText(const QString& text) const override;
    QValidator::State validate(QString& input, int& pos) const override;

private:
    TextFromValueHandler m_textFromValueHandler;
    ValueFromTextHandler m_valueFromTextHandler;
    bool m_emptyInputIsValid;
};

#endif // WIDGETSSPINBOXWITHCUSTOMDISPLAY_H
