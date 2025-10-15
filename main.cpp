#include "frd2vtu.h"
#include "step2inp.h"
#include "simulation_config.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::string config_file = "resources/simulation_config.json";
    
    // Allow config file to be specified as command line argument
    if (argc == 2) {
        config_file = argv[1];
    } else if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        std::cerr << "If no config file is specified, uses resources/simulation_config.json" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Load simulation configuration from JSON
    SimulationConfig config;
    try {
        config = SimulationConfig::fromJsonFile(config_file);
        std::cout << "Loaded configuration from: " << config_file << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "エラー: 設定ファイルの読み込みに失敗しました: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::string step_file = config.step_file;
    
    // Create constraint conditions from config
    std::vector<ConstraintCondition> constraints;
    for (const auto& fixed_face : config.constraints.fixed_faces) {
        constraints.push_back(createConstraintCondition(fixed_face.surface_id));
    }
    
    // Create load conditions from config
    std::vector<LoadCondition> loads;
    for (const auto& load : config.loads.applied_loads) {
        std::vector<double> direction = {load.direction.x, load.direction.y, load.direction.z};
        loads.push_back(createLoadCondition(load.surface_id, load.magnitude, direction));
    }
    
    if (constraints.empty() && loads.empty()) {
        std::cerr << "エラー: 設定ファイルに境界条件が指定されていません。" << std::endl;
        return EXIT_FAILURE;
    }

    // Step 1: Convert STEP to INP
    std::cout << "Step 1: Converting STEP to INP..." << std::endl;
    int result = convertStepToInp(step_file, constraints, loads);
    if (result != 0) {
        std::cerr << "エラー: STEP to INP conversion failed" << std::endl;
        return result;
    }
    
    // Get base filename for subsequent operations
    std::filesystem::path path(step_file);
    std::string base_name = path.stem().string();
    std::string inp_file = base_name + ".inp";
    std::string frd_file = base_name + ".frd";
    std::string vtu_file = base_name + ".vtu";
    
    // Step 2: Run CalculiX analysis
    std::cout << "Step 2: Running CalculiX analysis..." << std::endl;
    std::string ccx_command = "ccx_2.22 " + base_name;
    result = std::system(ccx_command.c_str());
    if (result != 0) {
        std::cerr << "エラー: CalculiX analysis failed" << std::endl;
        return result;
    }
    
    // Check if FRD file was generated
    if (!std::filesystem::exists(frd_file)) {
        std::cerr << "エラー: FRD file was not generated: " << frd_file << std::endl;
        return EXIT_FAILURE;
    }
    
    // Step 3: Convert FRD to VTU
    std::cout << "Step 3: Converting FRD to VTU..." << std::endl;
    result = convertFrdToVtu(frd_file, vtu_file);
    if (result == EXIT_SUCCESS) {
        std::cout << "Analysis pipeline completed successfully!" << std::endl;
        std::cout << "Generated files:" << std::endl;
        std::cout << "  - INP file: " << inp_file << std::endl;
        std::cout << "  - FRD file: " << frd_file << std::endl;
        std::cout << "  - VTU file: " << vtu_file << std::endl;
    } else {
        std::cerr << "エラー: FRD to VTU conversion failed" << std::endl;
    }

    return result;
}