#include "CalculixDockWidget.h"
#include "ui_CalculixDockWidget.h"


CalculixDockWidget::CalculixDockWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalculixDockWidget)
{
    ui->setupUi(this);
}

CalculixDockWidget::~CalculixDockWidget()
{
    delete ui;
}

void CalculixDockWidget::on_calPath_clicked()
{
    calPath = QFileDialog::getExistingDirectory(
                this,
                tr("选择求解器路径"),
                ".",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!calPath.isEmpty()) {
        qDebug() << "求解器路径" << calPath;
    }

}

void CalculixDockWidget::on_openCalpre_clicked()
{
    QString prePath = QFileDialog::getOpenFileName(
                this,
                tr("选择fbl文件"),
                ".",
                tr("FBL文件(*.fbl)"));

    if (!prePath.isEmpty()) {
        qDebug() << "选中的fbl文件" << prePath;
    }
    QString cmd = calPath + "/bin/cgx -b " + prePath;
    qDebug() << cmd ;    //auto grid = IngReader::Load();
    process.start(cmd);
    //process.waitForFinished();

    QString mshPath = QFileInfo(prePath).absolutePath() + "/all.msh";
    qDebug() << mshPath;
}

