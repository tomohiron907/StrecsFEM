#include "step2inp.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <gmsh.h>
#include <cmath>
#include <map>
#include <algorithm>
#include <iomanip>

Step2Inp::Step2Inp() {}

Step2Inp::~Step2Inp() {}

void Step2Inp::calculateNodeForcesByArea(std::ofstream& f, int surface_number, double total_force, const std::vector<double>& force_direction) {
    writeForceBoundaryConditionWithArea(f, surface_number, total_force, force_direction);
}

void Step2Inp::writeForceBoundaryCondition(std::ofstream& f, const std::vector<int>& node_tags, int surface_number) {
    f << "***********************************************************\n";
    f << "** constraints force node loads\n";
    f << "*CLOAD\n";
    f << "** ConstraintForce\n";
    f << "** node loads on shape: Part__Feature:Face" << surface_number << "\n";
    
    double force_value = -1.0;
    for (int tag : node_tags) {
        f << tag << "," << 3 << "," << force_value << "\n";
    }
}

double Step2Inp::calculateElementArea(const std::vector<std::vector<double>>& coords) {
    int n_nodes = coords.size();
    
    if (n_nodes == 3) {
        // 三角形要素: 2つの辺ベクトルの外積の大きさの半分
        std::vector<double> v1 = {coords[1][0] - coords[0][0], coords[1][1] - coords[0][1], coords[1][2] - coords[0][2]};
        std::vector<double> v2 = {coords[2][0] - coords[0][0], coords[2][1] - coords[0][1], coords[2][2] - coords[0][2]};
        
        // 外積計算
        std::vector<double> cross = {
            v1[1] * v2[2] - v1[2] * v2[1],
            v1[2] * v2[0] - v1[0] * v2[2],
            v1[0] * v2[1] - v1[1] * v2[0]
        };
        
        // ベクトルの大きさ
        double magnitude = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
        return 0.5 * magnitude;
        
    } else if (n_nodes == 4) {
        // 四角形要素: 2つの三角形に分割して面積を合計
        std::vector<double> v1 = {coords[1][0] - coords[0][0], coords[1][1] - coords[0][1], coords[1][2] - coords[0][2]};
        std::vector<double> v2 = {coords[2][0] - coords[0][0], coords[2][1] - coords[0][1], coords[2][2] - coords[0][2]};
        
        std::vector<double> cross1 = {
            v1[1] * v2[2] - v1[2] * v2[1],
            v1[2] * v2[0] - v1[0] * v2[2],
            v1[0] * v2[1] - v1[1] * v2[0]
        };
        double area1 = 0.5 * std::sqrt(cross1[0] * cross1[0] + cross1[1] * cross1[1] + cross1[2] * cross1[2]);
        
        std::vector<double> v3 = {coords[2][0] - coords[0][0], coords[2][1] - coords[0][1], coords[2][2] - coords[0][2]};
        std::vector<double> v4 = {coords[3][0] - coords[0][0], coords[3][1] - coords[0][1], coords[3][2] - coords[0][2]};
        
        std::vector<double> cross2 = {
            v3[1] * v4[2] - v3[2] * v4[1],
            v3[2] * v4[0] - v3[0] * v4[2],
            v3[0] * v4[1] - v3[1] * v4[0]
        };
        double area2 = 0.5 * std::sqrt(cross2[0] * cross2[0] + cross2[1] * cross2[1] + cross2[2] * cross2[2]);
        
        return area1 + area2;
        
    } else {
        // その他の多角形要素（簡易的に扇形分割）
        double area = 0.0;
        for (int i = 1; i < n_nodes - 1; ++i) {
            std::vector<double> v1 = {coords[i][0] - coords[0][0], coords[i][1] - coords[0][1], coords[i][2] - coords[0][2]};
            std::vector<double> v2 = {coords[i+1][0] - coords[0][0], coords[i+1][1] - coords[0][1], coords[i+1][2] - coords[0][2]};
            
            std::vector<double> cross = {
                v1[1] * v2[2] - v1[2] * v2[1],
                v1[2] * v2[0] - v1[0] * v2[2],
                v1[0] * v2[1] - v1[1] * v2[0]
            };
            
            double magnitude = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
            area += 0.5 * magnitude;
        }
        return area;
    }
}

void Step2Inp::writeForceBoundaryConditionWithArea(std::ofstream& f, int surface_number, double total_force, const std::vector<double>& force_direction) {
    f << "***********************************************************\n";
    f << "** constraints force node loads\n";
    f << "*CLOAD\n";
    f << "** ConstraintForce\n";
    f << "** node loads on shape: Part__Feature:Face" << surface_number << "\n";
    f << "** Total force: " << total_force << " N, Direction: [" 
      << force_direction[0] << ", " << force_direction[1] << ", " << force_direction[2] << "]\n";
    
    // ステップ1: 初期化
    std::map<std::size_t, double> node_areas;  // 各節点の寄与面積
    double total_surface_area = 0.0;  // 面全体の面積
    
    // 指定された面の要素を取得
    std::vector<int> element_types;
    std::vector<std::vector<std::size_t>> element_node_tags;
    std::vector<std::vector<std::size_t>> node_tags;
    gmsh::model::mesh::getElements(element_types, element_node_tags, node_tags, 2, surface_number);
    
    // ステップ2: 全要素をループして面積を計算・分配
    for (size_t i = 0; i < element_types.size(); ++i) {
        int elem_type = element_types[i];
        
        // 要素タイプごとの節点数を取得
        std::string element_name;
        int dim, order, num_nodes, num_primary_nodes;
        std::vector<double> parametric_coords;
        gmsh::model::mesh::getElementProperties(elem_type, element_name, dim, order, num_nodes, parametric_coords, num_primary_nodes);
        
        // この要素タイプの全要素を処理
        const auto& elem_nodes = element_node_tags[i];
        int num_elements = elem_nodes.size() / num_nodes;
        
        for (int j = 0; j < num_elements; ++j) {
            // 要素を構成する節点のタグを取得
            int start_idx = j * num_nodes;
            std::vector<std::size_t> element_node_list(elem_nodes.begin() + start_idx, 
                                                      elem_nodes.begin() + start_idx + num_nodes);
            
            // 節点の座標を取得
            std::vector<std::vector<double>> coords;
            for (std::size_t node_tag : element_node_list) {
                std::vector<double> coord;
                std::vector<double> parametric_coord;
                int entity_dim, entity_tag;
                gmsh::model::mesh::getNode(node_tag, coord, parametric_coord, entity_dim, entity_tag);
                
                if (coord.size() >= 3) {
                    coords.push_back({coord[0], coord[1], coord[2]});
                } else {
                    coords.push_back(coord);
                }
            }
            
            // 要素の面積を計算
            double element_area = calculateElementArea(coords);
            total_surface_area += element_area;
            
            // 要素面積を節点に分配
            double area_portion = element_area / num_nodes;
            for (std::size_t node_tag : element_node_list) {
                node_areas[node_tag] += area_portion;
            }
        }
    }
    
    // ステップ3: 各節点にかかる力を計算
    if (total_surface_area > 0) {
        double pressure = total_force / total_surface_area;  // 面全体の圧力
        
        // 力の方向ベクトルを正規化
        std::vector<double> normalized_direction = force_direction;
        double magnitude = std::sqrt(normalized_direction[0] * normalized_direction[0] + 
                                   normalized_direction[1] * normalized_direction[1] + 
                                   normalized_direction[2] * normalized_direction[2]);
        if (magnitude > 0) {
            for (double& component : normalized_direction) {
                component /= magnitude;
            }
        }
        
        f << "** Total surface area: " << std::fixed << std::setprecision(6) << total_surface_area << "\n";
        f << "** Pressure: " << std::fixed << std::setprecision(6) << pressure << " N/unit_area\n";
        
        // 各節点への力を計算・出力
        for (const auto& [node_tag, node_area] : node_areas) {
            double force_magnitude = pressure * node_area;
            std::vector<double> force_vector = {
                force_magnitude * normalized_direction[0],
                force_magnitude * normalized_direction[1], 
                force_magnitude * normalized_direction[2]
            };
            
            // 各自由度に対する力成分を出力
            for (int dof = 1; dof <= 3; ++dof) {
                if (std::abs(force_vector[dof-1]) > 1e-12) {  // 微小な値は無視
                    f << node_tag << "," << dof << "," 
                      << std::fixed << std::setprecision(6) << force_vector[dof-1] << "\n";
                }
            }
        }
    } else {
        std::cout << "警告: Surface " << surface_number << " の面積が0です。力の境界条件を適用できません。" << std::endl;
    }
}

void Step2Inp::writeEall(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** Define element set Eall\n";
    f << "*ELSET, ELSET=Eall\n";
    f << "volume1\n";
}

void Step2Inp::writeMaterialElementSet(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** Element sets for materials and FEM element type (solid, shell, beam, fluid)\n";
    f << "*ELSET, ELSET=MaterialSolidSolid\n";
    f << "volume1\n";
}

void Step2Inp::writeConstraintNodeSet(std::ofstream& f, const std::vector<int>& node_tags) {
    f << "***********************************************************\n";
    f << "** constraints fixed node sets\n";
    f << "** ConstraintFixed\n";
    f << "*NSET,NSET=ConstraintFixed\n";
    for (int tag : node_tags) {
        f << tag << ",\n";
    }
}

void Step2Inp::writePhysicalConstants(std::ofstream& f) {
    f << "** Physical constants for SI(mm) unit system with Kelvins\n";
    f << "*PHYSICAL CONSTANTS, ABSOLUTE ZERO=0, STEFAN BOLTZMANN=5.670374419e-11\n";
}

void Step2Inp::writeMaterial(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** Materials\n";
    f << "** see information about units at file end\n";
    f << "** FreeCAD material name: PLA-Generic\n";
    f << "** MaterialSolid\n";
    f << "*MATERIAL, NAME=MaterialSolid\n";
    f << "*ELASTIC\n";
    f << "3640,0.36\n";
}

void Step2Inp::writeSections(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** Sections\n";
    f << "*SOLID SECTION, ELSET=MaterialSolidSolid, MATERIAL=MaterialSolid\n";
}

void Step2Inp::writeStep(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** At least one step is needed to run an CalculiX analysis of FreeCAD\n";
    f << "*STEP, INC=2000\n";
    f << "*STATIC\n";
}

void Step2Inp::writeFixedConstraints(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** Fixed Constraints\n";
    f << "** ConstraintFixed\n";
    f << "*BOUNDARY\n";
    f << "ConstraintFixed,1\n";
    f << "ConstraintFixed,2\n";
    f << "ConstraintFixed,3\n";
}

void Step2Inp::writeOutputs(std::ofstream& f) {
    f << "***********************************************************\n";
    f << "** Outputs --> frd file\n";
    f << "*NODE FILE\n";
    f << "U\n";
    f << "*EL FILE\n";
    f << "S, E\n";
    f << "** outputs --> dat file\n";
    f << "** reaction forces for Constraint fixed\n";
    f << "*NODE PRINT, NSET=ConstraintFixed, TOTALS=ONLY\n";
    f << "RF\n";
}

void Step2Inp::writeEndStep(std::ofstream& f) {
    f << "*OUTPUT, FREQUENCY=1\n";
    f << "***********************************************************\n";
    f << "*END STEP\n";
}

std::vector<BoundaryCondition> Step2Inp::parseBoundaryConditions(const std::vector<std::string>& args) {
    std::vector<BoundaryCondition> boundary_conditions;
    
    for (size_t i = 2; i < args.size(); ++i) {
        const std::string& arg = args[i];
        size_t colon_pos = arg.find(':');
        
        if (colon_pos == std::string::npos) {
            std::cerr << "エラー: 境界条件は 'タイプ:面番号' の形式で指定してください: " << arg << std::endl;
            return {};
        }
        
        std::string bc_type = arg.substr(0, colon_pos);
        std::string surface_str = arg.substr(colon_pos + 1);
        
        if (bc_type != "force" && bc_type != "fixed") {
            std::cerr << "エラー: 境界条件タイプは 'force' または 'fixed' を指定してください: " << bc_type << std::endl;
            return {};
        }
        
        try {
            int surface_number = std::stoi(surface_str);
            boundary_conditions.push_back({bc_type, surface_number});
        } catch (const std::invalid_argument&) {
            std::cerr << "エラー: 面番号は整数で指定してください: " << surface_str << std::endl;
            return {};
        }
    }
    
    return boundary_conditions;
}

std::string Step2Inp::getBaseFilename(const std::string& filename) {
    std::filesystem::path path(filename);
    return path.stem().string();
}

int Step2Inp::convert(const std::string& step_file, const std::vector<BoundaryCondition>& boundary_conditions) {
    gmsh::initialize();
    
    try {
        std::cout << "STEPファイルを読み込み中: " << step_file << std::endl;
        gmsh::open(step_file);
        
        gmsh::model::geo::synchronize();
        
        std::vector<std::pair<int, int>> vols;
        gmsh::model::getEntities(vols, 3);
        
        if (vols.empty()) {
            std::cerr << "エラー: STEPファイル内にボリュームが見つかりませんでした。" << std::endl;
            gmsh::finalize();
            return 1;
        }
        
        std::vector<int> vol_tags;
        for (const auto& vol : vols) {
            vol_tags.push_back(vol.second);
        }
        gmsh::model::addPhysicalGroup(3, vol_tags, -1, "SolidVolume");
        
        gmsh::option::setNumber("Mesh.CharacteristicLengthMin", 4);
        gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 20);
        gmsh::option::setNumber("Mesh.HighOrderOptimize", 2);
        
        std::cout << "3Dメッシュを生成中..." << std::endl;
        gmsh::model::mesh::generate(3);
        gmsh::model::mesh::setOrder(2);
        gmsh::model::mesh::optimize("HighOrderElastic");
        
        gmsh::option::setNumber("Mesh.SaveAll", 0);
        
        std::string base_name = getBaseFilename(step_file);
        std::string inp_file = base_name + ".inp";
        std::cout << "INPファイルを出力中: " << inp_file << std::endl;
        
        gmsh::write(inp_file);
        
        std::vector<std::pair<int, int>> surfaces;
        gmsh::model::getEntities(surfaces, 2);
        
        std::vector<int> surface_tags;
        for (const auto& surface : surfaces) {
            surface_tags.push_back(surface.second);
        }
        
        std::cout << "利用可能な面 (Surface):" << std::endl;
        for (int tag : surface_tags) {
            std::cout << "  Surface " << tag << std::endl;
        }
        
        for (const auto& bc : boundary_conditions) {
            if (std::find(surface_tags.begin(), surface_tags.end(), bc.surface_number) == surface_tags.end()) {
                std::cerr << "エラー: Surface " << bc.surface_number << " が見つかりません。" << std::endl;
                gmsh::finalize();
                return 1;
            }
        }
        
        std::ofstream f(inp_file, std::ios::app);
        if (!f.is_open()) {
            std::cerr << "エラー: ファイルを開けませんでした: " << inp_file << std::endl;
            gmsh::finalize();
            return 1;
        }
        
        writeEall(f);
        writeMaterialElementSet(f);
        
        for (const auto& bc : boundary_conditions) {
            if (bc.type == "fixed") {
                std::vector<std::size_t> node_tags;
                std::vector<double> coord, parametricCoord;
                gmsh::model::mesh::getNodes(node_tags, coord, parametricCoord, 2, bc.surface_number, true);
                
                std::vector<int> int_node_tags(node_tags.begin(), node_tags.end());
                std::cout << "Surface " << bc.surface_number << " のノード数: " << node_tags.size() << std::endl;
                writeConstraintNodeSet(f, int_node_tags);
            }
        }
        
        writePhysicalConstants(f);
        writeMaterial(f);
        writeSections(f);
        writeStep(f);
        writeFixedConstraints(f);
        
        for (const auto& bc : boundary_conditions) {
            if (bc.type == "force") {
                std::vector<std::size_t> node_tags;
                std::vector<double> coord, parametricCoord;
                gmsh::model::mesh::getNodes(node_tags, coord, parametricCoord, 2, bc.surface_number, true);
                
                std::cout << "Surface " << bc.surface_number << " のノード数: " << node_tags.size() << std::endl;
                
                // Use area-based force calculation with default values (100N in -Z direction)
                std::vector<double> force_direction = {0.0, 0.0, -1.0};
                writeForceBoundaryConditionWithArea(f, bc.surface_number, 500.0, force_direction);
                std::cout << "Surface " << bc.surface_number << " に寄与面積に基づく力の境界条件を追加しました" << std::endl;
            }
        }
        
        writeOutputs(f);
        writeEndStep(f);
        
        f.close();
        
        std::cout << "変換完了（境界条件追加済み): " << step_file << " -> " << inp_file << std::endl;
        std::cout << "適用された境界条件:" << std::endl;
        for (const auto& bc : boundary_conditions) {
            std::cout << "  Surface " << bc.surface_number << ": " << bc.type << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "エラーが発生しました: " << e.what() << std::endl;
        gmsh::finalize();
        return 1;
    }
    
    gmsh::finalize();
    return 0;
}

BoundaryCondition createBoundaryCondition(const std::string& type, int surface_number) {
    return {type, surface_number};
}

int convertStepToInp(const std::string& step_file, const std::vector<BoundaryCondition>& conditions) {
    Step2Inp converter;
    return converter.convert(step_file, conditions);
}