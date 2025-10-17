#include "CalculixDockWidget.h"
#include "ui_CalculixDockWidget.h"

CalculixDockWidget::CalculixDockWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalculixDockWidget)
{
    ui->setupUi(this);

    timer.setInterval(16);
    connect(&timer, &QTimer::timeout, this, &CalculixDockWidget::onTick);

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
                "../../toolkit/MultiX/extern/Calculix",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!calPath.isEmpty()) {
        qDebug() << "求解器路径" << calPath;
        frd2vtu = calPath + "/ccx2paraview/ccx2paraview.py";
        qDebug() << "frd to vtu" << frd2vtu;
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
        qDebug() << cmd ;
        //workPath = prePath.left(prePath.lastIndexOf('.')) + '/';
        workPath = prePath.left(prePath.lastIndexOf('/') + 1);
        qDebug() << workPath ;
        process.setWorkingDirectory(workPath);
        process.start(cmd);
        process.waitForFinished();
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

void CalculixDockWidget::on_frd2vtu_clicked()
{
    if(calPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请配置求解器路径");
    } else {
        QString frdFilePath = QFileDialog::getOpenFileName(this, tr("选择frd文件"), "..", tr("inp文件(*.frd)"));
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString ld = env.value("LD_LIBRARY_PATH");

        QStringList parts;
        parts = ld.split(QLatin1Char(':'), QString::SkipEmptyParts);
        QString qtPath = QDir::homePath() + "/FENGSim/toolkit/Tools/qt/5.12.12/lib";
        parts.removeAll(qtPath);
        env.insert("LD_LIBRARY_PATH", parts.join(QLatin1Char(':')));
        process.setProcessEnvironment(env);

        process.start("python3", {
            frd2vtu,
            frdFilePath, "vtu"
        });
        process.waitForFinished();
        QString output = process.readAllStandardOutput();
        QString error = process.readAllStandardError();
        qDebug() << "输出:" << output.trimmed();
        if (!error.isEmpty())
            qDebug() << "错误:" << error.trimmed();
        QMessageBox::information(this, "提示", "数据转换完成");
    }
}

void CalculixDockWidget::on_calRes_clicked()
{
    QString vtuPath = QFileDialog::getOpenFileName(
                this,
                tr("选择vtu文件"),
                "..",
                tr("vtu文件(*.vtu)"));
    if (!vtuPath.isEmpty()) {
        qDebug() << "选中的vtu文件" << vtuPath;
        emit showVtuFile(vtuPath);
    } else {
        QMessageBox::warning(this, "警告", "请选择vtu文件");
    }
}

void CalculixDockWidget::onTick()
{
    if (!playing) return;

    timeSec += timer.interval() / 1000.0;
    const double pi = 3.14159265358979323846;
    const double s = baseScale * std::sin(2.0 * pi * freqHz * timeSec);
    emit vtuAnimation(s);
}

void CalculixDockWidget::on_playVtu_clicked()
{
    playing = !playing;
    if (playing)
    {
        ui->playVtu->setText("暂停");
        timeSec = 0.0;
        timer.start();
    } else {
        ui->playVtu->setText("播放");
        timer.stop();
        emit vtuAnimation(baseScale);
    }
}
