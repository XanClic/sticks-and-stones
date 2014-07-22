#include <cmath>
#include <cstdio>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <dake/math/matrix.hpp>

#include "asf.hpp"


using namespace dake::math;


static std::string strip(const std::string &s)
{
    size_t front = 0, back = s.length();

    if (!back--) {
        return std::string("");
    }

    for (; (front <= back) && isspace(s[front]); front++);
    for (; (back >= front) && isspace(s[back]); back++);

    if (front > back) {
        return std::string("");
    } else {
        return s.substr(front, back - front + 1);
    }
}


ASF::ASF(std::ifstream &s)
{
    while (getline(s)) {
        refresh_nil = true;

        if (next_input_line.front() == ':') {
            std::string section_line = strip(next_input_line.substr(1));
            std::stringstream section_line_ss(section_line);
            std::string section;

            section_line_ss >> section;

            if (section == "version") {
                read_version_section(s);
            } else if (section == "units") {
                read_units_section(s);
            } else if (section == "root") {
                read_root_section(s);
            } else if (section == "bonedata") {
                read_bonedata_section(s);
            } else if (section == "hierarchy") {
                read_hierarchy_section(s);
            } else {
                if ((section != "name") &&
                    (section != "documentation"))
                {
                    fprintf(stderr, "Warning: Unknown ASF section %s\n", section.c_str());
                }

                while (getline(s) && (next_input_line.front() != ':')) {
                    refresh_nil = true;
                }
            }
        } else {
            throw std::runtime_error("Unexpected ASF line: " + next_input_line);
        }
    }


    printf("ASF bone hierarchy:\n");
    dump_hierarchy();
}


void ASF::dump_hierarchy(int parent, int indentation) const
{
    if (parent < 0) {
        parent = root;
    }

    printf("%*s%i: %s\n", indentation, "", bs[parent].id, bs[parent].name.c_str());

    for (int child = bs[parent].first_child; child >= 0; child = bs[child].next_sibling) {
        dump_hierarchy(child, indentation + 2);
    }
}


bool ASF::getline(std::ifstream &s)
{
    if (!refresh_nil) {
        return true;
    }

    do {
        if (!std::getline(s, next_input_line)) {
            return false;
        }

        // This will also remove trailing \r
        next_input_line = strip(next_input_line);
    } while (next_input_line.empty() || (next_input_line.front() == '#'));

    refresh_nil = false;

    return true;
}


void ASF::read_version_section(std::ifstream &)
{
    std::stringstream section_line_ss(next_input_line);
    std::string section, version;

    section_line_ss >> section >> version;

    if (version != "1.10") {
        fprintf(stderr, "Warnung: Unkown ASF version %s\n", version.c_str());
    }
}


void ASF::read_units_section(std::ifstream &s)
{
    while (getline(s) && (next_input_line.front() != ':')) {
        refresh_nil = true;

        std::stringstream line_ss(next_input_line);
        std::string unit;

        line_ss >> unit;
        if (unit == "mass") {
            line_ss >> mass_default;
        } else if (unit == "length") {
            line_ss >> length_default;
        } else if (unit == "angle") {
            line_ss >> unit;
            if (unit == "deg") {
                angle_unit = static_cast<float>(M_PI) / 180.f;
            } else if (unit == "rad") {
                angle_unit = 1.f;
            } else {
                throw std::invalid_argument("Unkown ASF angle unit " + unit);
            }
        } else {
            fprintf(stderr, "Warning: Unknown ASF unit %s\n", unit.c_str());
        }
    }
}


void ASF::read_root_section(std::ifstream &s)
{
    if (root >= 0) {
        throw std::runtime_error("root bone redefined");
    }

    while (getline(s) && (next_input_line.front() != ':')) {
        refresh_nil = true;

        std::stringstream line_ss(next_input_line);
        std::string keyword;

        line_ss >> keyword;
        if (keyword == "axis") {
            std::string axis_order;
            line_ss >> axis_order;
            for (int i = 0; axis_order[i]; i++) {
                switch (axis_order[i]) {
                    case 'X': r_axis.push_back(RX); break;
                    case 'Y': r_axis.push_back(RY); break;
                    case 'Z': r_axis.push_back(RZ); break;
                    default: throw std::invalid_argument("Invalid axis given for root object");
                }
            }
        } else if (keyword == "order") {
            std::string axis;
            while (!line_ss.eof()) {
                line_ss >> axis;
                if (axis == "RX") {
                    r_order.push_back(RX);
                } else if (axis == "RY") {
                    r_order.push_back(RY);
                } else if (axis == "RZ") {
                    r_order.push_back(RZ);
                } else if (axis == "TX") {
                    r_order.push_back(TX);
                } else if (axis == "TY") {
                    r_order.push_back(TY);
                } else if (axis == "TZ") {
                    r_order.push_back(TZ);
                } else {
                    throw std::invalid_argument("Invalid axis given for order in root object");
                }
            }
        } else if (keyword == "position") {
            line_ss >> r_pos.x() >> r_pos.y() >> r_pos.z();
        } else if (keyword == "orientation") {
            line_ss >> r_orient.x() >> r_orient.y() >> r_orient.z();
        } else {
            fprintf(stderr, "Warning: Unknown ASF keyword %s for root section\n", keyword.c_str());
        }
    }

    Bone root_bone;
    root_bone.id = 0;
    root_bone.name = "root";
    root_bone.direction = vec3(0.f, 1.f, 0.f);

    bs.push_back(root_bone);
    root = bs.size() - 1;
}


void ASF::read_bonedata_section(std::ifstream &s)
{
    while (getline(s) && (next_input_line.front() != ':')) {
        refresh_nil = true;

        if (next_input_line != "begin") {
            throw std::invalid_argument("Expected 'begin' in :bonedata");
        }

        Bone bone;
        bone.axis_order = r_axis;
        bone.length = length_default;

        bool got_end = false;
        while (getline(s) && (next_input_line.front() != ':')) {
            refresh_nil = true;

            if (next_input_line == "end") {
                got_end = true;
                break;
            }

            std::stringstream line_ss(next_input_line);
            std::string keyword;

            line_ss >> keyword;
            if (keyword == "id") {
                line_ss >> bone.id;
            } else if (keyword == "name") {
                line_ss >> bone.name;
            } else if (keyword == "direction") {
                line_ss >> bone.direction.x() >> bone.direction.y() >> bone.direction.z();
                bone.direction.normalize();
            } else if (keyword == "length") {
                line_ss >> bone.length;
            } else if (keyword == "axis") {
                line_ss >> bone.axis.x() >> bone.axis.y() >> bone.axis.z();
                bone.axis *= angle_unit;

                bone.axis_order.clear();

                std::string axis_order;
                line_ss >> axis_order;
                for (int i = 0; axis_order[i]; i++) {
                    switch (axis_order[i]) {
                        case 'X': bone.axis_order.push_back(RX); break;
                        case 'Y': bone.axis_order.push_back(RY); break;
                        case 'Z': bone.axis_order.push_back(RZ); break;
                        default: throw std::invalid_argument("Invalid axis given for bone " + bone.name);
                    }
                }
            } else if (keyword == "dof") {
                bone.dof_order.clear();

                while (!line_ss.eof()) {
                    std::string dof_axis;
                    line_ss >> dof_axis;
                    if (dof_axis == "rx") {
                        bone.dof_order.push_back(RX);
                    } else if (dof_axis == "ry") {
                        bone.dof_order.push_back(RY);
                    } else if (dof_axis == "rz") {
                        bone.dof_order.push_back(RZ);
                    } else if (dof_axis != "l") {
                        throw std::invalid_argument("Invalid DOF axis given for bone " + bone.name);
                    }
                }

                for (Axis axis: bone.dof_order) {
                    bone.dof[static_cast<int>(axis)] = std::make_pair(-HUGE_VALF, HUGE_VALF);
                }
            } else if (keyword == "limits") {
                refresh_nil = false;
                bool first_line = true;

                for (Axis axis: bone.dof_order) {
                    if (!getline(s)) {
                        throw std::invalid_argument("Unexpected EOF (expected bone limit pair)");
                    }
                    refresh_nil = true;
                    if (next_input_line.front() == ':') {
                        throw std::invalid_argument("Unexpected section end");
                    }

                    std::pair<float, float> &limit = bone.dof[static_cast<int>(axis)];

                    const char *src = next_input_line.c_str();
                    bool success;
                    if (first_line) {
                        success = sscanf(src, "limits (%g %g)", &limit.first, &limit.second) == 2;
                        first_line = false;
                    } else {
                        success = sscanf(src, "(%g %g)", &limit.first, &limit.second) == 2;
                    }

                    if (!success) {
                        throw std::invalid_argument("Expected limit specification, could not match");
                    }

                    limit.first  *= angle_unit;
                    limit.second *= angle_unit;
                }
            } else {
                fprintf(stderr, "Warning: Unknown keyword for bone %s\n", bone.name.c_str());
            }
        }

        if (!got_end) {
            throw std::invalid_argument("Early end of bone " + bone.name + " segment");
        }

        bs.push_back(bone);
    }
}


ASF::Bone &ASF::find_bone(const std::string &name)
{
    for (Bone &bone: bs) {
        if (bone.name == name) {
            return bone;
        }
    }

    throw std::invalid_argument("Could not find bone " + name);
}


void ASF::read_hierarchy_section(std::ifstream &s)
{
    if (!getline(s)) {
        throw std::invalid_argument("Unexpected EOF (expected begin of hierarchy section)");
    }
    refresh_nil = true;

    if (next_input_line != "begin") {
        throw std::invalid_argument("Expected begin of hierarchy section, got " + next_input_line);
    }

    while (getline(s)) {
        refresh_nil = true;

        if (next_input_line == "end") {
            return;
        }

        std::stringstream line_ss(next_input_line);
        std::string bone_name;

        line_ss >> bone_name;
        Bone &parent = find_bone(bone_name);

        int *child_pointer = &parent.first_child;
        while (*child_pointer >= 0) {
            child_pointer = &bs[*child_pointer].next_sibling;
        }

        while (!line_ss.eof()) {
            line_ss >> bone_name;
            Bone &child = find_bone(bone_name);

            if (child.parent >= 0) {
                if (&bs[child.parent] != &parent) {
                    throw std::invalid_argument("Multiple parents given for bone " + child.name + ": " + bs[child.parent].name + " and " + parent.name);
                } else {
                    fprintf(stderr, "Warning: Redefinition of (same) parent %s for bone %s\n", parent.name.c_str(), child.name.c_str());
                }
            } else {
                child.parent = &parent - bs.data();
                *child_pointer = &child - bs.data();
                child_pointer = &child.next_sibling;
            }
        }
    }
}


void ASF::reset_transforms(void)
{
    reset_bone_transform(root, mat4::identity().translated(r_pos));
}


void ASF::reset_bone_transform(int bi, const mat4 &mv)
{
    Bone &bone = bs[bi];


    vec3 rot_axis;
    float angle;

    if (bone.direction != vec3(0.f, 1.f, 0.f)) {
        rot_axis = vec3(0.f, 1.f, 0.f).cross(bone.direction);
        angle = acosf(bone.direction.y()); // acosf(vec3(0.f, 1.f, 0.f).dot(bone.direction))
    } else {
        rot_axis = vec3(0.f, 0.f, 1.f);
        angle = 0.f;
    }

    bone.bone_dir_trans = mat4::identity().rotated(angle, rot_axis);

    bone.local_trans = mat4::identity();
    for (auto it = bone.axis_order.rbegin(); it != bone.axis_order.rend(); ++it) {
        switch (*it) {
            case RX: bone.local_trans.rotate(bone.axis.x(), vec3(1.f, 0.f, 0.f)); break;
            case RY: bone.local_trans.rotate(bone.axis.y(), vec3(0.f, 1.f, 0.f)); break;
            case RZ: bone.local_trans.rotate(bone.axis.z(), vec3(0.f, 0.f, 1.f)); break;
            default: throw std::invalid_argument("Bad rotation axis");
        }
    }

    bone.local_trans_inv = bone.local_trans.inverse();

    bone.motion_trans = mv;


    mat4 child_mv(bone.motion_trans.translated(bone.length * bone.direction));
    for (int child = bone.first_child; child >= 0; child = bs[child].next_sibling) {
        reset_bone_transform(child, child_mv);
    }
}
