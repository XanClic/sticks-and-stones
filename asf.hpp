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

            // Local transformation (from the axis)
            dake::math::mat4 local_trans, local_trans_inv;
            // Non-motion transformation
            dake::math::mat4 still_trans;
            // Motion transformation (some site calls it "local transform", but
            // it isn't even in the local coordinate system)
            dake::math::mat4 motion_trans;
            // Transforms global (0; 1; 0) to Bone.direction
            dake::math::mat4 bone_dir_trans;
        };

        ASF(std::ifstream &s);

        std::vector<Bone> &bones(void) { return bs; }
        const std::vector<Bone> &bones(void) const { return bs; }

        const std::vector<Axis> &root_axis(void) const { return r_axis; }
        const std::vector<Axis> &root_order(void) const { return r_order; }
        const dake::math::vec3 &root_position(void) const { return r_pos; }
        const dake::math::vec3 &root_orientation(void) const { return r_orient; }
        int root_index(void) const { return root; }

        void reset_transforms(void);

        float internal_length_unit(void) const { return length_unit; }


    private:
        void read_version_section(std::ifstream &s);
        void read_units_section(std::ifstream &s);
        void read_root_section(std::ifstream &s);
        void read_bonedata_section(std::ifstream &s);
        void read_hierarchy_section(std::ifstream &s);
        Bone &find_bone(const std::string &name);
        bool getline(std::ifstream &s);
        void reset_bone_transform(int bi, const dake::math::mat4 &mv);

        void dump_hierarchy(int parent = -1, int indentation = 0) const;

        std::vector<Bone> bs;

        std::vector<Axis> r_axis, r_order;
        dake::math::vec3 r_pos, r_orient;
        int root = -1;

        float mass_default = 1.f;
        float length_unit = 2.54e-2f; // inches -> meters
        float angle_unit = 1.f; // rad

        // Quality technology
        std::string next_input_line;
        bool refresh_nil = true;
};

#endif
