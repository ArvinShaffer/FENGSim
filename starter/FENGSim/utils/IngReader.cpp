#include "IngReader.h"

#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkTetra.h>
#include <vtkHexahedron.h>
#include <vtkWedge.h>
#include <vtkPyramid.h>
#include <vtkQuadraticHexahedron.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

using std::string;

namespace {
// -------- 小工具 ----------
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
// 可能是六面体近亲（8 节点）
inline bool isHex8(const string& t){
    return t == "C3D8" || t == "C3D8I";
}
// 20 节点六面体（含 C3D20 / C3D20R）
inline bool isHex20(const string& t){
    return t == "C3D20" || t == "C3D20R";
}

// 当前类型所需结点数（仅对已支持类型返回正数）
inline int requiredNodes(const string& t){
    if (isTetra(t)) return 4;
    if (isHex8(t)) return 8;
    if (isWedge(t)) return 6;         // 这里将 C3D5 当作 6 节点楔处理（见上）
    if (isHex20(t)) return 20;
    // 金字塔：某些 deck 会写成 5 结点（非标准），这里按 5 处理（无 TYPE 时靠 conn.size()==5 兜底）
    if (t == "C3D5") return 5;
    return -1;
}

// 把一条已 remap 的连接（conn，大小与 need 一致）插入 grid
inline void insertCellToGrid(const string& t,
                             const std::vector<vtkIdType>& conn,
                             vtkUnstructuredGrid* grid)
{
    // 四面体（线性）
    if (isTetra(t) && conn.size() >= 4) {
        auto cell = vtkSmartPointer<vtkTetra>::New();
        for (int i=0;i<4;++i) cell->GetPointIds()->SetId(i, conn[i]);
        grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
        return;
    }
    // 六面体（线性，8 节点）
    if (isHex8(t) && conn.size() >= 8) {
        auto cell = vtkSmartPointer<vtkHexahedron>::New();
        for (int i=0;i<8;++i) cell->GetPointIds()->SetId(i, conn[i]);
        grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
        return;
    }
    // 楔形（线性，6 节点）
    if (isWedge(t) && conn.size() >= 6) {
        auto cell = vtkSmartPointer<vtkWedge>::New();
        for (int i=0;i<6;++i) cell->GetPointIds()->SetId(i, conn[i]);
        grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
        return;
    }
    // 金字塔（线性，5 节点）——如果类型未知但恰好 5 个点，也可能在主循环里兜底处理
    if (!t.empty() && conn.size() == 5) {
        auto cell = vtkSmartPointer<vtkPyramid>::New();
        for (int i=0;i<5;++i) cell->GetPointIds()->SetId(i, conn[i]);
        grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
        return;
    }
    // 六面体（二次，20 节点）
    if (isHex20(t) && conn.size() >= 20) {
        auto cell = vtkSmartPointer<vtkQuadraticHexahedron>::New();

        // 角点 0..7（Abaqus 与 VTK 一致）
        for (int i = 0; i < 8; ++i)
            cell->GetPointIds()->SetId(i, conn[i]);

        // 你的示例 deck：中点顺序为 —— 底边(9–12) → 立边(17–20) → 顶边(13–16)
        // VTK 需要：           底边(8..11)  → 顶边(12..15)  → 立边(16..19)

        // 底边：直接复制 9..12 → VTK 8..11
        for (int k = 0; k < 4; ++k)
            cell->GetPointIds()->SetId(8 + k, conn[8 + k]);       // (0-1),(1-2),(2-3),(3-0)

        // 顶边：来自你文件中的 13..16（在索引 16..19）
        for (int k = 0; k < 4; ++k)
            cell->GetPointIds()->SetId(12 + k, conn[16 + k]);     // (4-5),(5-6),(6-7),(7-4)

        // 立边：来自你文件中的 17..20（在索引 12..15）
        for (int k = 0; k < 4; ++k)
            cell->GetPointIds()->SetId(16 + k, conn[12 + k]);     // (0-4),(1-5),(2-6),(3-7)

        /* 如果你的另一份 deck 使用“底→顶→立”（Abaqus 手册常见）,
           则把上面两段来源互换为：
             cell->GetPointIds()->SetId(12 + k, conn[12 + k]); // 顶边
             cell->GetPointIds()->SetId(16 + k, conn[16 + k]); // 立边
        */

        grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
        return;
    }

    // 若类型未知但恰好给了 5 点，当作金字塔兜底
    if (t.empty() && conn.size() == 5) {
        auto cell = vtkSmartPointer<vtkPyramid>::New();
        for (int i=0;i<5;++i) cell->GetPointIds()->SetId(i, conn[i]);
        grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
        return;
    }
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

    // 为 ELEMENT 续行缓存
    long long pendingEid = -1;
    std::vector<long long> pendingConn;

    auto flush_pending = [&](){
        if (!(readingElems && pendingEid >= 0)) return;
        int need = requiredNodes(currentElemTypeUpper);
        if (need <= 0 || static_cast<int>(pendingConn.size()) < need) {
            // 类型不支持或点数不够，丢弃
            pendingEid = -1;
            pendingConn.clear();
            return;
        }
        // remap
        std::vector<vtkIdType> conn;
        conn.reserve(need);
        bool missing = false;
        for (int i = 0; i < need; ++i) {
            auto it = nodeIdRemap.find(pendingConn[i]);
            if (it == nodeIdRemap.end()){ missing = true; break; }
            conn.push_back(it->second);
        }
        if (!missing) {
            insertCellToGrid(currentElemTypeUpper, conn, grid);
        }
        pendingEid = -1;
        pendingConn.clear();
    };

    string raw;
    while (std::getline(fin, raw)) {
        string line = raw;
        trim(line);
        if (line.empty()) continue;
        // Abaqus 注释
        if (line.size() >= 2 && line[0] == '*' && line[1] == '*') continue;

        if (!line.empty() && line[0] == '*') {
            // 新关键字到来前，先冲刷上一个 pending
            flush_pending();

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
            // 把整行先 token 化
            std::vector<long long> nums;
            for (long long v; ss >> v; ) nums.push_back(v);
            if (nums.empty()) continue;

            int need = requiredNodes(currentElemTypeUpper);
            if (need <= 0) {
                // 未支持类型：尝试 5 节点金字塔兜底（TYPE 缺失时）
                if (pendingEid < 0 && nums.size() >= 6) {
                    long long eid = nums[0];
                    std::vector<long long> connIds(nums.begin()+1, nums.end());
                    if (connIds.size() == 5) {
                        // remap -> 直接插入金字塔
                        std::vector<vtkIdType> conn(5);
                        bool missing = false;
                        for (int i=0;i<5;++i){
                            auto it = nodeIdRemap.find(connIds[i]);
                            if (it == nodeIdRemap.end()){ missing = true; break; }
                            conn[i] = it->second;
                        }
                        if (!missing) {
                            auto cell = vtkSmartPointer<vtkPyramid>::New();
                            for (int i=0;i<5;++i) cell->GetPointIds()->SetId(i, conn[i]);
                            grid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
                        }
                    }
                }
                continue;
            }

            if (pendingEid < 0) {
                // 新元素开始：必须至少包含 eid
                pendingEid = nums[0];
                pendingConn.assign(nums.begin()+1, nums.end());
            } else {
                // 续行：直接追加所有数字
                pendingConn.insert(pendingConn.end(), nums.begin(), nums.end());
            }

            // 凑齐就插入
            if (static_cast<int>(pendingConn.size()) >= need) {
                // remap & insert
                std::vector<vtkIdType> conn;
                conn.reserve(need);
                bool missing = false;
                for (int i = 0; i < need; ++i) {
                    auto it = nodeIdRemap.find(pendingConn[i]);
                    if (it == nodeIdRemap.end()){ missing = true; break; }
                    conn.push_back(it->second);
                }
                if (!missing) {
                    insertCellToGrid(currentElemTypeUpper, conn, grid);
                }
                // 本元素结束
                pendingEid = -1;
                pendingConn.clear();
            }
        }
    }

    // 文件结束后的尾部冲刷
    flush_pending();

    return grid;
}
