#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

class OpenKeyControlCenter : public QMainWindow {
public:
  OpenKeyControlCenter() {
    setWindowTitle("OpenKey - Cài đặt");
    resize(400, 350);

    settings = new QSettings("OpenKeyLinux", "Config", this);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QFormLayout *formLayout = new QFormLayout();

    // Input Method (vInputType)
    inputTypeCombo = new QComboBox(this);
    inputTypeCombo->addItem("Telex", 0);
    inputTypeCombo->addItem("VNI", 1);
    inputTypeCombo->addItem("Simple Telex 1", 2);
    inputTypeCombo->addItem("Simple Telex 2", 3);
    inputTypeCombo->setCurrentIndex(settings->value("inputType", 0).toInt());
    formLayout->addRow("Kiểu gõ:", inputTypeCombo);

    // Code Table (vCodeTable)
    codeTableCombo = new QComboBox(this);
    codeTableCombo->addItem("Unicode", 0);
    codeTableCombo->addItem("TCVN3 (ABC)", 1);
    codeTableCombo->addItem("VNI Windows", 2);
    codeTableCombo->addItem("Unicode Compound", 3);
    codeTableCombo->setCurrentIndex(settings->value("codeTable", 0).toInt());
    formLayout->addRow("Bảng mã:", codeTableCombo);

    // Options
    freeMarkCheck = new QCheckBox("Cho phép đặt dấu tự do", this);
    freeMarkCheck->setChecked(settings->value("freeMark", true).toBool());
    formLayout->addRow("", freeMarkCheck);

    modernOrthCheck = new QCheckBox("Sử dụng kiểu gõ hiện đại (oà, uý)", this);
    modernOrthCheck->setChecked(settings->value("modernOrth", true).toBool());
    formLayout->addRow("", modernOrthCheck);

    mainLayout->addLayout(formLayout);

    QPushButton *saveBtn = new QPushButton("Lưu & Áp dụng", this);
    saveBtn->setStyleSheet("background-color: #2ecc71; color: white; padding: "
                           "10px; font-weight: bold; border-radius: 5px;");
    connect(saveBtn, &QPushButton::clicked, this,
            &OpenKeyControlCenter::saveSettings);
    mainLayout->addWidget(saveBtn);

    setCentralWidget(centralWidget);
  }

private Q_SLOTS:
  void saveSettings() {
    settings->setValue("inputType", inputTypeCombo->currentIndex());
    settings->setValue("codeTable", codeTableCombo->currentIndex());
    settings->setValue("freeMark", freeMarkCheck->isChecked());
    settings->setValue("modernOrth", modernOrthCheck->isChecked());
    settings->sync();

    // Reload Fcitx5 to apply changes
    system("fcitx5-remote -r");
  }

private:
  QSettings *settings;
  QComboBox *inputTypeCombo;
  QComboBox *codeTableCombo;
  QCheckBox *freeMarkCheck;
  QCheckBox *modernOrthCheck;
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  OpenKeyControlCenter window;
  window.show();
  return app.exec();
}
