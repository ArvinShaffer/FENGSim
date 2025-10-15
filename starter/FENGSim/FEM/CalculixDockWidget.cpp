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
                "../../",
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
                "..",
                tr("FBL(*.fbl)"));

    if (!prePath.isEmpty()) {
        qDebug() << "选中的fbl文件" << prePath;
        //QString cmd = calPath + "/bin/cgx -b " + prePath;
        QString cmd = calPath + "/bin/cgx -bg " + prePath;
        qDebug() << cmd ;    //auto grid = IngReader::Load();
        process.start(cmd);
        process.waitForFinished();
        int lastSlash = prePath.lastIndexOf('/');
        if (lastSlash != -1) {
            QString cmd2 = "mv all.msh " + prePath.left(lastSlash + 1);
            qDebug() << cmd2;
            process.start(cmd2);
            process.waitForFinished();
        }

        QString mshPath = QFileInfo(prePath).absolutePath() + "/all.msh";
        qDebug() << mshPath;
        emit showInpFile(mshPath);
    }
    else {
        QMessageBox::warning(this, "警告", "请选择fbl文件");
    }
}


void CalculixDockWidget::on_calSolver_clicked()
{
    if(calPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请配置求解器路径");
    } else {
        QString inpFilePath = QFileDialog::getOpenFileName(this, tr("选择inp文件"), "..", tr("inp文件(*.inp)"));
        QString inpName = inpFilePath.left(inpFilePath.lastIndexOf('.'));
        QString ccxCmd = calPath + "/bin/ccx_2.21 " + inpName;
        qDebug() << ccxCmd;
        process.start(ccxCmd);
        process.waitForFinished();
        QMessageBox::information(this, "提示", "求解完成");
    }
}
