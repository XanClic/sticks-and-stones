#ifndef ASF_HPP
#define ASF_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <dake/math/matrix.hpp>


class ASF {
    public:
        enum Axis {
            RX, RY, RZ,
            TX, TY, TZ
        };

        struct Bone {
            int parent = -1, first_child = -1, next_sibling = -1;

            int id = 0;
            std::string name = std::string("");
            dake::math::vec3 direction = dake::math::vec3::zero();
            float length = 0.f;
            dake::math::vec3 axis = dake::math::vec3::zero();
            std::vector<Axis> axis_order, dof_order;
            std::unordered_map<int, std::pair<float, float>> dof;
        };

        ASF(std::ifstream &s);

        std::vector<Axis> root_axis, root_order;
        dake::math::vec3 root_position, root_orientation;
        std::vector<Bone> bones;
        int root = -1;


    private:
        void read_version_section(std::ifstream &s);
        void read_units_section(std::ifstream &s);
        void read_root_section(std::ifstream &s);
        void read_bonedata_section(std::ifstream &s);
        void read_hierarchy_section(std::ifstream &s);
        Bone &find_bone(const std::string &name);
        bool getline(std::ifstream &s);

        void dump_hierarchy(int parent = -1, int indentation = 0) const;

        float mass_default = 1.f;
        float length_default = 1.f;
        float angle_unit = 1.f; // rad

        // Quality technology
        std::string next_input_line;
        bool refresh_nil = true;
};

#endif
