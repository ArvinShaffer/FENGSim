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

    void ensureCgxAllowSys();

    bool runCommandLine(const QString &command, QString *stdOut = nullptr, QString *stdErr = nullptr);

    void runCommandWithProgress(QWidget *parent, const QString &command);

    void on_colorSelect_currentIndexChanged(const QString &arg1);

    void on_scale_valueChanged(double arg1);

private:
    Ui::CalculixDockWidget *ui;
    QString calPath;
    QString workPath;
    QString frd2vtu;
    QProcess process;


signals:
    void showInpFile(const QString &filePath);
    void showVtuFile(const QStringList &filesList);
    void signalPlayPause(bool playing);
    void changeColors(const QString &name);
    void changeScale(double scale);

};

#endif // CALCULIXDOCKWIDGET_H
