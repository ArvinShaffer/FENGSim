#pragma once
#include <string>
#include <vtkSmartPointer.h>

class vtkUnstructuredGrid;

/**
 * @brief 读取 Abaqus/CalculiX .ing/.inp（解析 *NODE 和 *ELEMENT）
 *        支持常见线性体单元：C3D4(四面体)、C3D8(六面体)、C3D6(楔形/棱柱)、C3D5/CPENTA(同为楔形)、C3D5H、C3D4H、C3D8I（按线性处理）
 *        其他或高阶单元会被忽略（不报错，跳过）
 *
 * @param file 输入文件路径
 * @param err  可选：错误信息输出（失败时写入）
 * @return vtkUnstructuredGrid（即使失败也返回非空 smart pointer；失败时 points/cells 可能为空）
 */
class IngReader
{
public:
    static vtkSmartPointer<vtkUnstructuredGrid>
    Load(const std::string& file, std::string* err = nullptr);
};

