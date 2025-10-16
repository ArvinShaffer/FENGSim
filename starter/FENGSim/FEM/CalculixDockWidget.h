#ifndef CALCULIXDOCKWIDGET_H
#define CALCULIXDOCKWIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

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

private:
    Ui::CalculixDockWidget *ui;
    QString calPath;
    QString workPath;
    QString frd2vtu;
    QProcess process;

signals:
    void showInpFile(const QString &filePath);
};

#endif // CALCULIXDOCKWIDGET_H
