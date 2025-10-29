#ifndef CALCULIXDOCKWIDGET_H
#define CALCULIXDOCKWIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QtMath>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProgressBar>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>

namespace Ui {
class CalculixDockWidget;
}

class CalculixDockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalculixDockWidget(QWidget *parent = nullptr);
    ~CalculixDockWidget();

public slots:
    void receiveVtuSclName(QStringList &vtuSclName);

private slots:
    void on_openCalpre_clicked();

    void on_calPath_clicked();

    void on_calSolver_clicked();

    void on_calRes_clicked();

    void on_frd2vtu_clicked();

    void on_playVtu_clicked();

    void ensureCgxAllowSys();

    bool runCommandLine(const QString &command, QString *stdOut = nullptr, QString *stdErr = nullptr);

    void runCommandWithProgress(QWidget *parent, const QString &command);

    void on_colorSelect_currentIndexChanged(const QString &arg1);


private:
    Ui::CalculixDockWidget *ui;
    QString calPath;
    QString workPath;
    QString frd2vtu;
    QProcess process;
    //QTimer timer;

    // 状态
    bool   playing{false};
    double baseScale{0.3};  // 滑条 0~0.5 的工程单位
    double freqHz{1.0};
    double timeSec{0.0};


signals:
    void showInpFile(const QString &filePath);
    void showVtuFile(const QString &filePath);
    void signalPlayPause(bool playing);
    void changeColors(const QString &name);

};

#endif // CALCULIXDOCKWIDGET_H
