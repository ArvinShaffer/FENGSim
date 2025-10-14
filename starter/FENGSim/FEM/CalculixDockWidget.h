#ifndef CALCULIXDOCKWIDGET_H
#define CALCULIXDOCKWIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QProcess>
#include <QFileInfo>
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

private:
    Ui::CalculixDockWidget *ui;
    QString calPath;
    QProcess process;

signals:
    void showInpFile(const QString &filePath);
};

#endif // CALCULIXDOCKWIDGET_H
