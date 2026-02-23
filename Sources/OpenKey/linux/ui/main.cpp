#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

class OpenKeyControlCenter : public QMainWindow {
public:
  OpenKeyControlCenter() {
    setWindowTitle("OpenKey - Cài đặt");
    setMinimumSize(450, 550);

    settings = new QSettings("OpenKeyLinux", "Config", this);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Header Section ---
    QWidget *header = new QWidget(this);
    header->setObjectName("header");
    header->setStyleSheet("QWidget#header { background-color: #f8f9fa; "
                          "border-bottom: 1px solid #dee2e6; }");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);

    QLabel *logoLabel = new QLabel(this);
    QPixmap logo(
        ":/openkey.png"); // Try resource first, fallback to path if needed
    if (logo.isNull()) {
      logo.load("/usr/share/icons/hicolor/256x256/apps/openkey.png");
    }
    logoLabel->setPixmap(
        logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    headerLayout->addWidget(logoLabel);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    QLabel *titleLabel = new QLabel("OpenKey Linux", this);
    titleLabel->setStyleSheet(
        "font-size: 20px; font-weight: bold; color: #2c3e50;");
    QLabel *versionLabel = new QLabel("Phiên bản 2.0.10", this);
    versionLabel->setStyleSheet("color: #7f8c8d;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(versionLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    // --- Tabs Section ---
    tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet("QTabWidget::pane { border: none; padding: 10px; }"
                             "QTabBar::tab { padding: 10px 20px; background: "
                             "#e9ecef; margin: 2px; border-radius: 4px; }"
                             "QTabBar::tab:selected { background: #2ecc71; "
                             "color: white; font-weight: bold; }");

    createTabEngine();
    createTabMacro();
    createTabSystem();
    createTabAbout();

    mainLayout->addWidget(tabWidget);

    // --- Bottom Buttons ---
    QWidget *bottomArea = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomArea);

    QPushButton *btnReset = new QPushButton("Mặc định", this);
    btnReset->setStyleSheet("padding: 8px 15px; background: #95a5a6; color: "
                            "white; border-radius: 4px;");
    connect(btnReset, &QPushButton::clicked, this,
            &OpenKeyControlCenter::resetDefaults);

    QPushButton *btnSave = new QPushButton("Đóng", this);
    btnSave->setStyleSheet("padding: 8px 30px; background: #2ecc71; color: "
                           "white; font-weight: bold; border-radius: 4px;");
    connect(btnSave, &QPushButton::clicked, this,
            &OpenKeyControlCenter::saveAndClose);

    bottomLayout->addWidget(btnReset);
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnSave);
    mainLayout->addWidget(bottomArea);

    setCentralWidget(centralWidget);
    loadSettings();
  }

private:
  void createTabEngine() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *groupMain = new QGroupBox("Cài đặt chính");
    QFormLayout *form = new QFormLayout(groupMain);

    inputTypeCombo = new QComboBox();
    inputTypeCombo->addItems(
        {"Telex", "VNI", "Simple Telex 1", "Simple Telex 2"});
    form->addRow("Kiểu gõ:", inputTypeCombo);

    codeTableCombo = new QComboBox();
    codeTableCombo->addItems({"Unicode", "TCVN3 (ABC)", "VNI Windows",
                              "Unicode Compound", "Vietware-F", "Vietware-X",
                              "BKIHCM"});
    form->addRow("Bảng mã:", codeTableCombo);
    layout->addWidget(groupMain);

    QGroupBox *groupOptions = new QGroupBox("Tùy chọn");
    QVBoxLayout *optLayout = new QVBoxLayout(groupOptions);

    checkFreeMark = new QCheckBox("Cho phép đặt dấu tự do (uý, oà)");
    checkModernOrth = new QCheckBox("Sử dụng kiểu gõ hiện đại (oà, uý)");
    checkCheckSpelling = new QCheckBox("Kiểm tra chính tả");
    checkUpperCaseFirst = new QCheckBox("Tự động viết hoa chữ cái đầu tiên");

    optLayout->addWidget(checkFreeMark);
    optLayout->addWidget(checkModernOrth);
    optLayout->addWidget(checkCheckSpelling);
    optLayout->addWidget(checkUpperCaseFirst);
    layout->addWidget(groupOptions);

    layout->addStretch();
    tabWidget->addTab(page, "Bộ gõ");
  }

  void createTabMacro() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *groupMacro = new QGroupBox("Gõ tắt");
    QVBoxLayout *mLayout = new QVBoxLayout(groupMacro);

    checkUseMacro = new QCheckBox("Cho phép gõ tắt");
    checkUseMacroEnglish =
        new QCheckBox("Cho phép gõ tắt khi ở chế độ Tiếng Anh");
    checkAutoCapsMacro = new QCheckBox("Tự động viết hoa từ gõ tắt");

    mLayout->addWidget(checkUseMacro);
    mLayout->addWidget(checkUseMacroEnglish);
    mLayout->addWidget(checkAutoCapsMacro);

    QPushButton *btnMacroTable = new QPushButton("Bảng gõ tắt...");
    btnMacroTable->setFixedWidth(150);
    mLayout->addWidget(btnMacroTable);

    layout->addWidget(groupMacro);

    QGroupBox *groupAdvance = new QGroupBox("Cải tiến");
    QVBoxLayout *aLayout = new QVBoxLayout(groupAdvance);
    checkQuickTelex = new QCheckBox("Gõ nhanh Telex (tự động thêm w, [)");
    checkQuickStartConsonant =
        new QCheckBox("Gõ nhanh phụ âm đầu (f->ph, j->gi, w->ng)");
    checkQuickEndConsonant =
        new QCheckBox("Gõ nhanh phụ âm cuối (g->ng, h->nh, k->ch)");

    aLayout->addWidget(checkQuickTelex);
    aLayout->addWidget(checkQuickStartConsonant);
    aLayout->addWidget(checkQuickEndConsonant);
    aLayout->addWidget(checkQuickEndConsonant);
    layout->addWidget(groupAdvance);

    layout->addStretch();
    tabWidget->addTab(page, "Gõ tắt");
  }

  void createTabSystem() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *groupSystem = new QGroupBox("Hệ thống");
    QVBoxLayout *sLayout = new QVBoxLayout(groupSystem);

    checkRunAtStartup = new QCheckBox("Chạy cùng hệ thống");
    checkShowTrayIcon = new QCheckBox("Hiện icon trên thanh hệ thống");
    checkUseGrayIcon = new QCheckBox("Sử dụng icon màu xám (Modern Icon)");
    checkUseClipboard =
        new QCheckBox("Sử dụng clipboard để gửi phím (Nhanh hơn)");

    sLayout->addWidget(checkRunAtStartup);
    sLayout->addWidget(checkShowTrayIcon);
    sLayout->addWidget(checkUseGrayIcon);
    sLayout->addWidget(checkUseClipboard);
    layout->addWidget(groupSystem);

    QGroupBox *groupCompatibility = new QGroupBox("Tương thích");
    QVBoxLayout *cLayout = new QVBoxLayout(groupCompatibility);
    checkFixBrowser =
        new QCheckBox("Sửa lỗi gõ tiếng Việt trên Browser (Chrome, Edge...)");
    checkFixChromium = new QCheckBox("Tối ưu cho các trình duyệt Chromium");

    cLayout->addWidget(checkFixBrowser);
    cLayout->addWidget(checkFixChromium);
    layout->addWidget(groupCompatibility);

    layout->addStretch();
    tabWidget->addTab(page, "Hệ thống");
  }

  void createTabAbout() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *logoLarge = new QLabel(this);
    QPixmap logo(":/openkey.png");
    if (logo.isNull()) {
      logo.load("/usr/share/icons/hicolor/256x256/apps/openkey.png");
    }
    logoLarge->setPixmap(
        logo.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logoLarge, 0, Qt::AlignCenter);

    QLabel *name = new QLabel("OpenKey Linux", this);
    name->setStyleSheet(
        "font-size: 18px; font-weight: bold; margin-top: 10px;");
    layout->addWidget(name, 0, Qt::AlignCenter);

    QLabel *author = new QLabel("Tác giả: Mai Vũ Tuyên", this);
    layout->addWidget(author, 0, Qt::AlignCenter);

    QPushButton *btnCheckUpdate =
        new QPushButton("Kiểm tra phiên bản mới...", this);
    btnCheckUpdate->setStyleSheet("margin: 20px; padding: 10px; background: "
                                  "#3498db; color: white; border-radius: 4px;");
    layout->addWidget(btnCheckUpdate, 0, Qt::AlignCenter);

    QHBoxLayout *links = new QHBoxLayout();
    auto addLink = [&](QString name, QString url) {
      QPushButton *btn = new QPushButton(name);
      btn->setFlat(true);
      btn->setStyleSheet("color: #3498db; text-decoration: underline;");
      connect(btn, &QPushButton::clicked,
              [url]() { QDesktopServices::openUrl(QUrl(url)); });
      links->addWidget(btn);
    };
    addLink("Trang chủ", "https://open-key.org");
    addLink("Fanpage", "https://facebook.com/OpenKeyVN");
    addLink("Source Code", "https://github.com/tuyenvm/OpenKey");
    layout->addLayout(links);

    layout->addStretch();
    tabWidget->addTab(page, "Thông tin");
  }

  void loadSettings() {
    inputTypeCombo->setCurrentIndex(settings->value("inputType", 0).toInt());
    codeTableCombo->setCurrentIndex(settings->value("codeTable", 0).toInt());
    checkFreeMark->setChecked(settings->value("freeMark", true).toBool());
    checkModernOrth->setChecked(settings->value("modernOrth", true).toBool());
    checkCheckSpelling->setChecked(
        settings->value("checkSpelling", true).toBool());
    checkUpperCaseFirst->setChecked(
        settings->value("upperCaseFirst", false).toBool());

    checkUseMacro->setChecked(settings->value("useMacro", false).toBool());
    checkUseMacroEnglish->setChecked(
        settings->value("useMacroEnglish", false).toBool());
    checkAutoCapsMacro->setChecked(
        settings->value("autoCapsMacro", true).toBool());
    checkQuickTelex->setChecked(settings->value("quickTelex", false).toBool());
    checkQuickStartConsonant->setChecked(
        settings->value("quickStart", false).toBool());
    checkQuickEndConsonant->setChecked(
        settings->value("quickEnd", false).toBool());

    checkRunAtStartup->setChecked(
        settings->value("runAtStartup", true).toBool());
    checkShowTrayIcon->setChecked(
        settings->value("showTrayIcon", true).toBool());
    checkUseGrayIcon->setChecked(
        settings->value("useGrayIcon", false).toBool());
    checkUseClipboard->setChecked(
        settings->value("useClipboard", true).toBool());
    checkFixBrowser->setChecked(settings->value("fixBrowser", true).toBool());
    checkFixChromium->setChecked(settings->value("fixChromium", true).toBool());
  }

  void saveAndClose() {
    settings->setValue("inputType", inputTypeCombo->currentIndex());
    settings->setValue("codeTable", codeTableCombo->currentIndex());
    settings->setValue("freeMark", checkFreeMark->isChecked());
    settings->setValue("modernOrth", checkModernOrth->isChecked());
    settings->setValue("checkSpelling", checkCheckSpelling->isChecked());
    settings->setValue("upperCaseFirst", checkUpperCaseFirst->isChecked());

    settings->setValue("useMacro", checkUseMacro->isChecked());
    settings->setValue("useMacroEnglish", checkUseMacroEnglish->isChecked());
    settings->setValue("autoCapsMacro", checkAutoCapsMacro->isChecked());
    settings->setValue("quickTelex", checkQuickTelex->isChecked());
    settings->setValue("quickStart", checkQuickStartConsonant->isChecked());
    settings->setValue("quickEnd", checkQuickEndConsonant->isChecked());

    settings->setValue("runAtStartup", checkRunAtStartup->isChecked());
    settings->setValue("showTrayIcon", checkShowTrayIcon->isChecked());
    settings->setValue("useGrayIcon", checkUseGrayIcon->isChecked());
    settings->setValue("useClipboard", checkUseClipboard->isChecked());
    settings->setValue("fixBrowser", checkFixBrowser->isChecked());
    settings->setValue("fixChromium", checkFixChromium->isChecked());

    settings->sync();
    system("fcitx5-remote -r");
    close();
  }

  void resetDefaults() {
    settings->clear();
    loadSettings();
  }

  QSettings *settings;
  QTabWidget *tabWidget;

  // Tab 1
  QComboBox *inputTypeCombo, *codeTableCombo;
  QCheckBox *checkFreeMark, *checkModernOrth, *checkCheckSpelling,
      *checkUpperCaseFirst;

  // Tab 2
  QCheckBox *checkUseMacro, *checkUseMacroEnglish, *checkAutoCapsMacro;
  QCheckBox *checkQuickTelex, *checkQuickStartConsonant,
      *checkQuickEndConsonant;

  // Tab 3
  QCheckBox *checkRunAtStartup, *checkShowTrayIcon, *checkUseGrayIcon,
      *checkUseClipboard;
  QCheckBox *checkFixBrowser, *checkFixChromium;
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setStyle("Fusion"); // Ensure a clean, modern look on Linux
  OpenKeyControlCenter window;
  window.show();
  return app.exec();
}
