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

namespace Ui {
class CalculixDockWidget;
}

class CalculixDockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalculixDockWidget(QWidget *parent = nullptr);
    ~CalculixDockWidget();


private slots:
    void on_openCalpre_clicked();

    void on_calPath_clicked();

    void on_calSolver_clicked();

    void on_calRes_clicked();

    void on_frd2vtu_clicked();

    void onTick();

    void on_playVtu_clicked();

    void ensureCgxAllowSys();

    bool runCommandLine(const QString &command, QString *stdOut = nullptr, QString *stdErr = nullptr);

private:
    Ui::CalculixDockWidget *ui;
    QString calPath;
    QString workPath;
    QString frd2vtu;
    QProcess process;
    QTimer timer;

    // 状态
    bool   playing{false};
    double baseScale{0.1};  // 滑条 0~0.5 的工程单位
    double freqHz{1.0};
    double timeSec{0.0};


signals:
    void showInpFile(const QString &filePath);
    void showVtuFile(const QString &filePath);

    void vtuAnimation(double s);
};

#endif // CALCULIXDOCKWIDGET_H
