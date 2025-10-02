#ifndef STEP2INP_H
#define STEP2INP_H

#include <string>
#include <vector>
#include <fstream>

struct BoundaryCondition {
    std::string type;
    int surface_number;
};

class Step2Inp {
public:
    Step2Inp();
    ~Step2Inp();
    
    int convert(const std::string& step_file, const std::vector<BoundaryCondition>& boundary_conditions);

private:
    void writeForceBoundaryCondition(std::ofstream& f, const std::vector<int>& node_tags, int surface_number);
    void writeEall(std::ofstream& f);
    void writeMaterialElementSet(std::ofstream& f);
    void writeConstraintNodeSet(std::ofstream& f, const std::vector<int>& node_tags);
    void writePhysicalConstants(std::ofstream& f);
    void writeMaterial(std::ofstream& f);
    void writeSections(std::ofstream& f);
    void writeStep(std::ofstream& f);
    void writeFixedConstraints(std::ofstream& f);
    void writeOutputs(std::ofstream& f);
    void writeEndStep(std::ofstream& f);
    
    std::vector<BoundaryCondition> parseBoundaryConditions(const std::vector<std::string>& args);
    std::string getBaseFilename(const std::string& filename);
};

// Utility function
BoundaryCondition createBoundaryCondition(const std::string& type, int surface_number);
int convertStepToInp(const std::string& step_file, const std::vector<BoundaryCondition>& conditions);

#endif // STEP2INP_H