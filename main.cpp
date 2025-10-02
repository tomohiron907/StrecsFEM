#include "frd2vtu.h"
#include "step2inp.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " input.step boundary_condition1 [boundary_condition2] ..." << std::endl;
        std::cerr << "Boundary condition format: type:surface_number" << std::endl;
        std::cerr << "Types: force or fixed" << std::endl;
        std::cerr << "Example: " << argv[0] << " model.step fixed:1 force:2" << std::endl;
        return EXIT_FAILURE;
    }

    std::string step_file = argv[1];
    
    // Parse boundary conditions
    std::vector<BoundaryCondition> boundary_conditions;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        size_t colon_pos = arg.find(':');
        
        if (colon_pos == std::string::npos) {
            std::cerr << "エラー: 境界条件は 'タイプ:面番号' の形式で指定してください: " << arg << std::endl;
            return EXIT_FAILURE;
        }
        
        std::string bc_type = arg.substr(0, colon_pos);
        std::string surface_str = arg.substr(colon_pos + 1);
        
        if (bc_type != "force" && bc_type != "fixed") {
            std::cerr << "エラー: 境界条件タイプは 'force' または 'fixed' を指定してください: " << bc_type << std::endl;
            return EXIT_FAILURE;
        }
        
        try {
            int surface_number = std::stoi(surface_str);
            boundary_conditions.push_back(createBoundaryCondition(bc_type, surface_number));
        } catch (const std::invalid_argument&) {
            std::cerr << "エラー: 面番号は整数で指定してください: " << surface_str << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    if (boundary_conditions.empty()) {
        std::cerr << "エラー: 少なくとも1つの境界条件を指定してください。" << std::endl;
        return EXIT_FAILURE;
    }

    // Step 1: Convert STEP to INP
    std::cout << "Step 1: Converting STEP to INP..." << std::endl;
    int result = convertStepToInp(step_file, boundary_conditions);
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