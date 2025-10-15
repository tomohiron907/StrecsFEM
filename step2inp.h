#ifndef STEP2INP_H
#define STEP2INP_H

#include <string>
#include <vector>
#include <fstream>

struct ConstraintCondition {
    int surface_number;
};

struct LoadCondition {
    int surface_number;
    double magnitude;
    std::vector<double> direction;
};

class Step2Inp {
public:
    Step2Inp();
    ~Step2Inp();
    
    int convert(const std::string& step_file, const std::vector<ConstraintCondition>& constraints, const std::vector<LoadCondition>& loads);
    void calculateNodeForcesByArea(std::ofstream& f, int surface_number, double total_force = 100.0, const std::vector<double>& force_direction = {0.0, 0.0, -1.0});

private:
    void writeForceBoundaryCondition(std::ofstream& f, const std::vector<int>& node_tags, int surface_number);
    void writeForceBoundaryConditionWithArea(std::ofstream& f, int surface_number, double total_force = 100.0, const std::vector<double>& force_direction = {0.0, 0.0, -1.0});
    double calculateElementArea(const std::vector<std::vector<double>>& coords);
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
    
    std::string getBaseFilename(const std::string& filename);
};

// Utility functions
ConstraintCondition createConstraintCondition(int surface_number);
LoadCondition createLoadCondition(int surface_number, double magnitude, const std::vector<double>& direction);
int convertStepToInp(const std::string& step_file, const std::vector<ConstraintCondition>& constraints, const std::vector<LoadCondition>& loads);

#endif // STEP2INP_H