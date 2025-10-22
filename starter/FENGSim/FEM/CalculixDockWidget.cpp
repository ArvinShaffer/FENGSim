#include "CalculixDockWidget.h"
#include "ui_CalculixDockWidget.h"

CalculixDockWidget::CalculixDockWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalculixDockWidget)
{
    ui->setupUi(this);

    timer.setInterval(16);
    connect(&timer, &QTimer::timeout, this, &CalculixDockWidget::onTick);
    ensureCgxAllowSys();

}

CalculixDockWidget::~CalculixDockWidget()
{
    delete ui;
}

/**
 * @brief 手动拆分命令行字符串为 程序 + 参数
 *        支持带引号、空格。
 */
static QStringList splitCommandManual(const QString &cmd)
{
    QStringList result;
    QRegularExpression re(R"((?:[^\s"]+|"[^"]*")+)");
    QRegularExpressionMatchIterator i = re.globalMatch(cmd);
    while (i.hasNext()) {
        QRegularExpressionMatch m = i.next();
        QString token = m.captured(0);
        if (token.startsWith('"') && token.endsWith('"'))
            token = token.mid(1, token.length() - 2);
        result << token;
    }
    return result;
}

/**
 * @brief 运行一条命令行（同步阻塞版）
 * @param command 整行命令，如 "python3 /path/to/script.py arg1 arg2"
 * @param stdOut  可选：标准输出
 * @param stdErr  可选：标准错误
 * @return 是否执行成功（退出码==0）
 */
bool CalculixDockWidget::runCommandLine(const QString &command, QString *stdOut, QString *stdErr)
{
    // 自动拆分命令为程序+参数

    QStringList parts = splitCommandManual(command);
    if (parts.isEmpty()) {
        qWarning() << "runCommandLine(): empty command!";
        return false;
    }

    QString program = parts.takeFirst();
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setWorkingDirectory(workPath);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("TERM", "dumb");
    env.insert("NO_COLOR", "1");
    env.insert("PYTHONWARNINGS", "ignore::SyntaxWarning");
    process.setProcessEnvironment(env);

    // 启动
    process.start(program, parts);
    if (!process.waitForStarted(3000)) {
        qWarning() << "Failed to start:" << program;
        return false;
    }

    // 等待执行完成
    process.waitForFinished(-1);
    QByteArray out = process.readAllStandardOutput();
    QByteArray err = process.readAllStandardError();
    QString stdoutText = QString::fromLocal8Bit(out);
    QString stderrText = QString::fromLocal8Bit(err);

    if (stdOut) *stdOut = stdoutText;
    if (stdErr) *stdErr = stderrText;

    qDebug().noquote() << "[Output]\n" << stdoutText;
    if (!stderrText.isEmpty())
        qWarning().noquote() << "[Error]\n" << stderrText;

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
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
        //qDebug() << "frd to vtu" << frd2vtu;
    }
}

void CalculixDockWidget::ensureCgxAllowSys()
{
    QString path = QDir::homePath() + "/.cgx";
    QFile f(path);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << "ALLOW_SYS\n";
    }
}

void CalculixDockWidget::on_openCalpre_clicked()
{
    QString prePath = QFileDialog::getOpenFileName(
                this,
                tr("选择fbl,fbd文件"),
                "..",
                tr("FBL(*.fbl *.fbd)"));

    if (!prePath.isEmpty()) {
        qDebug() << "选中的fbl文件" << prePath;
        //QString cmd = calPath + "/bin/cgx -b " + prePath;
        QString cmd = calPath + "/bin/cgx -bg " + prePath;
        qDebug() << cmd ;
        //workPath = prePath.left(prePath.lastIndexOf('.')) + '/';
        workPath = prePath.left(prePath.lastIndexOf('/') + 1);
        //qDebug() << workPath ;

        QString out, err;
        bool ok = runCommandLine(cmd, &out, &err);
        if (ok) {
            qDebug() << "Command executed successfully!";
        } else {
            qWarning() << "fbl file open failed";
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
        QString out, err;
        bool ok = runCommandLine(ccxCmd, &out, &err);
        if (ok) {
            qDebug() << "Command executed successfully!";
        } else {
            qWarning() << " solve failed";
        }
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
        qDebug() << "python3 " + frd2vtu + " " + frdFilePath + " vtu";
        QProcess p;
        p.setProcessEnvironment(env);

        p.start("python3", {
            frd2vtu,
            frdFilePath, "vtu"
        });
        p.waitForFinished();
        QString output = p.readAllStandardOutput();
        QString error = p.readAllStandardError();
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
