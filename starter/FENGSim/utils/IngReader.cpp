#include "IngReader.h"

#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkTetra.h>
#include <vtkHexahedron.h>
#include <vtkWedge.h>
#include <vtkPyramid.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

using std::string;

namespace {
// 小工具
inline void ltrim(string& s){
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char c){ return !std::isspace(c); }));
}
inline void rtrim(string& s){
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
}
inline void trim(string& s){ ltrim(s); rtrim(s); }
inline string upper(string s){
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c){ return std::toupper(c); });
    return s;
}
// 从 "*ELEMENT, TYPE=XXXX, ..." 抓到类型名
inline string parseElemType(const string& keywordLineUpper){
    auto pos = keywordLineUpper.find("TYPE=");
    if (pos == string::npos) return {};
    string t = keywordLineUpper.substr(pos + 5);
    auto cut = t.find_first_of(", \t\r\n");
    if (cut != string::npos) t = t.substr(0, cut);
    return t;
}
// 简单判断是否楔形（多别名）
inline bool isWedge(const string& t){
    // 常见别名：C3D6、C3D5、CPENTA、C3D5H 等按 6 结点楔形处理（线性）
    return t == "C3D6" || t == "C3D5" || t == "CPENTA" || t == "C3D5H";
}
// 可能是四面体近亲
inline bool isTetra(const string& t){
    return t == "C3D4" || t == "C3D4H";
}
// 可能是六面体近亲
inline bool isHex(const string& t){
    return t == "C3D8" || t == "C3D8I";
}
} // namespace

vtkSmartPointer<vtkUnstructuredGrid>
IngReader::Load(const std::string& file, std::string* err)
{
    auto grid   = vtkSmartPointer<vtkUnstructuredGrid>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    grid->SetPoints(points);

    std::ifstream fin(file);
    if (!fin) {
        if (err) *err = "无法打开文件: " + file;
        return grid;
    }

    // 原ID -> 压缩后的 0..N-1
    std::unordered_map<long long, vtkIdType> nodeIdRemap;
    nodeIdRemap.reserve(100000);

    bool readingNodes = false;
    bool readingElems = false;
    string currentElemTypeUpper;

    string raw;
    while (std::getline(fin, raw)) {
        string line = raw;
        trim(line);
        if (line.empty()) continue;
        // Abaqus 注释
        if (line.size() >= 2 && line[0] == '*' && line[1] == '*') continue;

        if (!line.empty() && line[0] == '*') {
            string u = upper(line);
            readingNodes = (u.find("*NODE") != string::npos);
            readingElems = (u.find("*ELEMENT") != string::npos);
            if (readingElems) currentElemTypeUpper = parseElemType(u);
            continue;
        }

        // 数据行：把逗号转空格，便于 >> 解析
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream ss(line);

        if (readingNodes) {
            // id, x, y, z
            long long nid; double x, y, z;
            if (!(ss >> nid >> x >> y >> z)) continue;
            auto it = nodeIdRemap.find(nid);
            if (it == nodeIdRemap.end()) {
                vtkIdType newId = static_cast<vtkIdType>(nodeIdRemap.size());
                nodeIdRemap.emplace(nid, newId);
                points->InsertPoint(newId, x, y, z);
            } else {
                points->SetPoint(it->second, x, y, z); // 再定义时覆盖
            }
        } else if (readingElems) {
            // eid, n1, n2, ...
            long long eid;
            if (!(ss >> eid)) continue;

            std::vector<long long> connIds;
            long long nid;
            while (ss >> nid) connIds.push_back(nid);
            if (connIds.empty()) continue;

            // remap
            std::vector<vtkIdType> conn;
            conn.reserve(connIds.size());
            bool missing = false;
            for (auto src : connIds) {
                auto it = nodeIdRemap.find(src);
                if (it == nodeIdRemap.end()) { missing = true; break; }
                conn.push_back(it->second);
            }
            if (missing) continue;

            const string t = currentElemTypeUpper;

            // 四面体
            if (isTetra(t) && conn.size() >= 4) {
                auto cell = vtkSmartPointer<vtkTetra>::New();
                cell->GetPointIds()->SetId(0, conn[0]);
                cell->GetPointIds()->SetId(1, conn[1]);
                cell->GetPointIds()->SetId(2, conn[2]);
                cell->GetPointIds()->SetId(3, conn[3]);
                grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
                continue;
            }

            // 六面体
            if (isHex(t) && conn.size() >= 8) {
                auto cell = vtkSmartPointer<vtkHexahedron>::New();
                for (int i=0;i<8;++i) cell->GetPointIds()->SetId(i, conn[i]);
                grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
                continue;
            }

            // 楔形 / 棱柱
            if (isWedge(t) && conn.size() >= 6) {
                auto cell = vtkSmartPointer<vtkWedge>::New();
                for (int i=0;i<6;++i) cell->GetPointIds()->SetId(i, conn[i]);
                grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
                continue;
            }

            // 金字塔（有些 deck 用 C3D5 也指金字塔，这里只在恰好 5 点时当作金字塔）
            if (conn.size() == 5) {
                auto cell = vtkSmartPointer<vtkPyramid>::New();
                for (int i=0;i<5;++i) cell->GetPointIds()->SetId(i, conn[i]);
                grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
                continue;
            }

            // 其他类型（例如高阶单元）——当前忽略
        }
    }

    return grid;
}

