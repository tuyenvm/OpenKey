#include <QApplication>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

class OpenKeyControlCenter : public QMainWindow {
public:
  OpenKeyControlCenter() {
    setWindowTitle("OpenKey - Cài đặt");
    resize(600, 400);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    QLabel *label =
        new QLabel("OpenKey Linux Control Center (Qt5 Native)", this);
    label->setAlignment(Qt::AlignCenter);

    QPushButton *reloadBtn = new QPushButton("Reload Fcitx5", this);
    connect(reloadBtn, &QPushButton::clicked, this, []() {
      // Replace with actual Fcitx5 IPC / system command in the future
      system("fcitx5-remote -r");
    });

    layout->addWidget(label);
    layout->addWidget(reloadBtn);

    setCentralWidget(centralWidget);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  OpenKeyControlCenter window;
  window.show();

  return app.exec();
}
