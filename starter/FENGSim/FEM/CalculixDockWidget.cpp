#include "CalculixDockWidget.h"
#include "ui_CalculixDockWidget.h"

CalculixDockWidget::CalculixDockWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalculixDockWidget)
{
    ui->setupUi(this);

    ui->chkXY->setStyleSheet(R"(
                             QCheckBox::indicator:unchecked {
                                 border: 2px solid #888;
                                 background-color: #fff;
                                 border-radius: 4px;
                             }
                             QCheckBox::indicator:checked {
                                 border: 2px solid #0078d7;
                                 background-color: #0078d7;
                             }
                         )");
    ui->chkXZ->setStyleSheet(R"(
                             QCheckBox::indicator:unchecked {
                                 border: 2px solid #888;
                                 background-color: #fff;
                                 border-radius: 4px;
                             }
                             QCheckBox::indicator:checked {
                                 border: 2px solid #0078d7;
                                 background-color: #0078d7;
                             }
                         )");
    ui->chkYZ->setStyleSheet(R"(
                             QCheckBox::indicator:unchecked {
                                 border: 2px solid #888;
                                 background-color: #fff;
                                 border-radius: 4px;
                             }
                             QCheckBox::indicator:checked {
                                 border: 2px solid #0078d7;
                                 background-color: #0078d7;
                             }
                         )");
    ui->chkLoop->setStyleSheet(R"(
                               QCheckBox::indicator:unchecked {
                                   border: 2px solid #888;
                                   background-color: #fff;
                                   border-radius: 4px;
                               }
                               QCheckBox::indicator:checked {
                                   border: 2px solid #0078d7;
                                   background-color: #0078d7;
                               }
                           )");
    auto updateMirrorMask = [=] {
        int mask = 0;
        if (ui->chkXY->isChecked()) mask |= 1;
        if (ui->chkXZ->isChecked()) mask |= 2;
        if (ui->chkYZ->isChecked()) mask |= 4;
        //emit chkxyz(mask);
        pending.mirrorMask = mask;
    };
    connect(ui->chkXY, &QCheckBox::toggled, this, [=](bool) { updateMirrorMask(); });
    connect(ui->chkXZ, &QCheckBox::toggled, this, [=](bool) { updateMirrorMask(); });
    connect(ui->chkYZ, &QCheckBox::toggled, this, [=](bool) { updateMirrorMask(); });
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

    qDebug().noquote() << "[Output]CalculixDockwidget \n" << stdoutText;
    if (!stderrText.isEmpty())
        qWarning().noquote() << "[Error]CalculixDockwidget \n" << stderrText;

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void CalculixDockWidget::runCommandWithProgress(QWidget *parent, const QString &command)
{
    auto *dialog = new QDialog(parent);
    dialog->setWindowTitle("运行中...");
    dialog->resize(600, 400);

    auto *layout = new QVBoxLayout(dialog);
    auto *label = new QLabel(QString("正在执行命令：\n%1").arg(command));
    layout->addWidget(label);

    auto *progress = new QProgressBar(dialog);
    progress->setRange(0, 0); // 无限循环模式
    layout->addWidget(progress);

    auto *text = new QTextEdit(dialog);
    text->setReadOnly(true);
    layout->addWidget(text, 1);

    auto *btnCancel = new QPushButton("取消", dialog);
    layout->addWidget(btnCancel);

    // 拆分命令行
    QStringList parts = splitCommandManual(command);
    for (QString &p : parts)
        if (p.startsWith('"') && p.endsWith('"')) p = p.mid(1, p.length() - 2);

    QString program = parts.takeFirst();
    auto *process = new QProcess(dialog);
    process->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(process, &QProcess::readyReadStandardOutput, [=]() {
        text->append(QString::fromLocal8Bit(process->readAllStandardOutput()));
        text->moveCursor(QTextCursor::End);
    });

    QObject::connect(process, &QProcess::readyReadStandardError, [=]() {
        text->append(QString::fromLocal8Bit(process->readAllStandardError()));
        text->moveCursor(QTextCursor::End);
    });
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [=](int code, QProcess::ExitStatus status) {
        progress->setRange(0, 1);
        progress->setValue(1);
        QString msg = (status == QProcess::NormalExit && code == 0)
                ? "✅ 任务完成"
                : QString("❌ 任务失败，退出码 %1").arg(code);
        label->setText(msg);
        btnCancel->setText("关闭");
    });
    QObject::connect(btnCancel, &QPushButton::clicked, [=]() {
        if (process->state() == QProcess::Running) {
            process->kill();
            process->waitForFinished(1000);
        }
        dialog->close();
    });

    process->setWorkingDirectory(workPath);
    process->start(program, parts);
    dialog->exec();
}

void CalculixDockWidget::on_calPath_clicked()
{
    calPath = QFileDialog::getExistingDirectory(
                this,
                tr("选择求解器路径"),
                "../../toolkit/MultiX/extern/Calculix",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!calPath.isEmpty()) {
        qDebug() << "CalculixDockwidget 求解器路径" << calPath;
        frd2vtu = calPath + "/ccx2paraview/ccx2paraview.py";
        //qDebug() << "frd to vtu" << frd2vtu;
    }
}

void CalculixDockWidget::ensureCgxAllowSys()
{
    QString path = QDir::homePath() + "/.cgx";
    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "ALLOW_SYS\n";
        }
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
        qDebug() << "CalculixDockwidget 选中的fbl文件" << prePath;
        //QString cmd = calPath + "/bin/cgx -b " + prePath;
        QString cmd = calPath + "/bin/cgx -bg " + prePath;
        qDebug() << "CalculixDockwidget " << cmd ;
        //workPath = prePath.left(prePath.lastIndexOf('.')) + '/';
        workPath = prePath.left(prePath.lastIndexOf('/') + 1);
        //qDebug() << workPath ;

        QString out, err;
        bool ok = runCommandLine(cmd, &out, &err);
        if (ok) {
            qDebug() << "CalculixDockwidget Command executed successfully!";
        } else {
            qWarning() << "CalculixDockwidget fbl file open failed";
        }

        QString mshPath = QFileInfo(prePath).absolutePath() + "/all.msh";
        qDebug() << "CalculixDockwidget " << mshPath;
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
        qDebug() << "CalculixDockwidget  " << ccxCmd;
        runCommandWithProgress(this, ccxCmd);
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
        qDebug() << "CalculixDockwidget python3 " + frd2vtu + " " + frdFilePath + " vtu";
        QProcess p;
        p.setProcessEnvironment(env);

        p.start("python3", {
            frd2vtu,
            frdFilePath, "vtu"
        });
        p.waitForFinished();
        QString output = p.readAllStandardOutput();
        QString error = p.readAllStandardError();
        qDebug() << "CalculixDockwidget 输出:" << output.trimmed();
        if (!error.isEmpty())
            qDebug() << "CalculixDockwidget 错误:" << error.trimmed();
        QMessageBox::information(this, "提示", "数据转换完成");
    }
}

void CalculixDockWidget::on_calRes_clicked()
{
    QStringList vtuPath = QFileDialog::getOpenFileNames(this, "选择多个 .vtu", "..", "VTU (*.vtu)");
    if (!vtuPath.isEmpty()) {
        emit showVtuFile(vtuPath);
    } else {
        QMessageBox::warning(this, "警告", "请选择vtu文件");
    }
}


void CalculixDockWidget::receiveVtuSclName(QStringList &vtuSclName)
{
    ui->colorSelect->clear();
    ui->colorSelect->addItems(vtuSclName);
}

void CalculixDockWidget::on_colorSelect_currentIndexChanged(const QString &arg1)
{
    QVariant data = ui->colorSelect->currentData();
    pending.scalar =  data.toList();
    pending.scalarContent = arg1;
}

void CalculixDockWidget::on_scale_valueChanged(double arg1)
{
    pending.warpScale = arg1;
}


void CalculixDockWidget::on_playVtu_clicked()
{
    ui->playVtu->setDisabled(true);
    ui->pauseVtu->setDisabled(false);
    emit signalPlayPause(true);
}

void CalculixDockWidget::on_pauseVtu_clicked()
{
    ui->playVtu->setDisabled(false);
    ui->pauseVtu->setDisabled(true);
    emit signalPlayPause(false);
}


void CalculixDockWidget::on_vtuSpeed_valueChanged(double arg1)
{
    pending.playSpeed = arg1;
}



void CalculixDockWidget::on_chkLoop_stateChanged(int arg1)
{
    if (arg1) {
        pending.loop = true;
    } else {
        pending.loop = false;
    }

}


void CalculixDockWidget::receiveFilesChange(const QStringList& files)
{
    QSignalBlocker block(ui->frame);
    ui->frame->clear();
    for (const auto& path : files) {
        ui->frame->addItem(QFileInfo(path).fileName());
    }
    ui->frame->setEnabled(!files.isEmpty());
    if (!files.isEmpty()) {
        ui->frame->setCurrentIndex(0);
    }
}

void CalculixDockWidget::receiveArray(const VtuData& vtuData)
{
    {
        QSignalBlocker block(ui->vectors);
        ui->vectors->clear();
        ui->vectors->addItem("(无)", QString());
        for (const auto& name: vtuData.vecName) {
            ui->vectors->addItem(name, name);
        }
        int idx = ui->vectors->findData(vtuData.currVec);
        if (idx < 0) idx = 0;
        ui->vectors->setCurrentIndex(idx);
        pending.vectorArray = ui->vectors->currentData().toString();
        ui->vectors->setEnabled(ui->vectors->count() > 1);
    }
    {
        QSignalBlocker block(ui->colorSelect);
        ui->colorSelect->clear();
        ui->colorSelect->addItem(QString("(无)"), QVariantList({QString(""), QString("")}));
        for (auto it = vtuData.sclName.begin(); it != vtuData.sclName.cend(); ++it) {
            const QString& key = it.key();
            const QStringList& subList = it.value();

            for (const QString& sub : subList) {
                QString display = key + "_" + sub;
                QVariantList payload;
                payload << key << sub;
                ui->colorSelect->addItem(display, payload);
            }
        }

        int idx = ui->colorSelect->findData(vtuData.currScl);
        if (idx < 0) idx = 0;
        ui->colorSelect->setCurrentIndex(idx);
        auto data = ui->colorSelect->currentData();
        pending.scalar = data.toList();
        ui->colorSelect->setEnabled(ui->colorSelect->count() > 1);
    }
}

void CalculixDockWidget::receiveFrame(const VtuData& vtuData)
{
    int currStep = vtuData.currStep;
    if (currStep >= 0 && currStep < ui->frame->count()) {
        QSignalBlocker block(ui->frame);
        ui->frame->setCurrentIndex(currStep);
    }
}

void CalculixDockWidget::on_vectors_currentIndexChanged(const QString &arg1)
{
    pending.vectorArray = ui->vectors->currentData().toString();
}

void CalculixDockWidget::on_frame_currentIndexChanged(int index)
{
    pending.currFrame = index;
}

void CalculixDockWidget::on_apply_clicked()
{
    emit signalApply(pending);
}


