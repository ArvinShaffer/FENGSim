#pragma once
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QVariantList>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkReverseSense.h>
#include <vtkTrivialProducer.h>
#include <vtkAlgorithm.h>

//class VTKWidget;
enum MirrorPlane {
    MirrorNone = 0,
    MirrorXY   = 1 << 0,
    MirrorXZ   = 1 << 1,
    MirrorYZ   = 1 << 2
};

// 收集 UI 选项，便于在信号槽之间按引用传递
class PendingOptions {
public:
    QString vectorArray;
    QVariantList scalar;
    QString scalarContent;
    bool wireframe = true;
    bool loop = false;
    double warpScale = 0.0;
    double playSpeed = 1.0;
    int mirrorMask = 0;
    int currFrame;
};

class VtuData {
public:
    QStringList vtuFiles;
    QVariantList currScl;
    QString currVec;

    QStringList vecName;
    QMap<QString, QStringList> sclName;

    bool autoSclRange = true;
    bool showWire = false;
    bool colorEnabled = true;
    bool loop = true;
    int currStep = 0;
    int mirrorMode = MirrorNone;
    double playFps = 3.0;
    //double sclMin = 0.0;
    //double sclMax = 1.0;
    double scalar = 0.0;
    double warpScale = 0.0;

    QMap<QString, double> rangeMin;
    QMap<QString, double> rangeMax;

    std::vector<MirrorPlane> mirrorOrder;
    vtkSmartPointer<vtkUnstructuredGrid> mirroredDataCache;
    vtkSmartPointer<vtkDataSetSurfaceFilter> mirrorSurfaceFilter;
    vtkSmartPointer<vtkReverseSense>        mirrorReverseFilter;
    vtkAlgorithm* mapperFinalAlgorithm = nullptr;
    vtkSmartPointer<vtkTrivialProducer>     passThroughProducer;
};
