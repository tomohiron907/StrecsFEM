#include "frd2vtu.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " input.frd output.vtu" << std::endl;
        return EXIT_FAILURE;
    }

    std::string frd_filename = argv[1];
    std::string vtu_filename = argv[2];

    int result = convertFrdToVtu(frd_filename, vtu_filename);
    if (result == EXIT_SUCCESS) {
        std::cout << "Successfully converted " << frd_filename << " to " << vtu_filename << std::endl;
    } else {
        std::cerr << "Error: Cannot open file " << frd_filename << std::endl;
    }

    return result;
}