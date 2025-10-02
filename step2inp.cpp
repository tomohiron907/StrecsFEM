#include "step2inp.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <gmsh.h>

Step2Inp::Step2Inp() {}

Step2Inp::~Step2Inp() {}

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
        
        gmsh::option::setNumber("Mesh.CharacteristicLengthMin", 40);
        gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 80);
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
                
                std::vector<int> int_node_tags(node_tags.begin(), node_tags.end());
                std::cout << "Surface " << bc.surface_number << " のノード数: " << node_tags.size() << std::endl;
                writeForceBoundaryCondition(f, int_node_tags, bc.surface_number);
                std::cout << "Surface " << bc.surface_number << " に力の境界条件を追加しました" << std::endl;
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