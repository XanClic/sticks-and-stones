#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <dake/math/matrix.hpp>

#include "amc.hpp"
#include "asf.hpp"


using namespace dake::math;


static std::string strip(const std::string &s)
{
    size_t front = 0, back = s.length();

    if (!back--) {
        return std::string("");
    }

    for (; (front <= back) && isspace(s[front]); front++);
    for (; (back >= front) && isspace(s[back]); back--);

    if (front > back) {
        return std::string("");
    } else {
        return s.substr(front, back - front + 1);
    }
}


static bool getline(std::ifstream &s, std::string &line)
{
    do {
        if (!std::getline(s, line)) {
            return false;
        }

        line = strip(line);
    } while (line.empty() || (line.front() == '#'));

    return true;
}


AMC::AMC(std::ifstream &s, ASF *a):
    asf(a)
{
    std::string line;
    int current_frame = -1, cfi = -1;
    float angle_unit = 1.f; // rad

    std::unordered_map<std::string, int> bone_indices;
    for (size_t i = 0; i < asf->bones().size(); i++) {
        bone_indices[asf->bones()[i].name] = i;
    }

    while (getline(s, line)) {
        if (line.front() == ':') {
            if (line == ":DEGREES") {
                angle_unit = static_cast<float>(M_PI) / 180.f;
            } else if (line != ":FULLY-SPECIFIED") {
                fprintf(stderr, "Warning: Unknown keyword %s\n", line.c_str());
            }
        } else {
            char *end;
            errno = 0;
            long frame = strtol(line.c_str(), &end, 0);
            if (!*end && !errno) {
                if (frame < 0) {
                    throw std::invalid_argument("Negative frame " + line + " specified");
                }

                current_frame = frame;
                if (ff < 0) {
                    ff = current_frame;
                }

                size_t elements_required;
                if (current_frame >= ff) {
                    elements_required = current_frame - ff + 1;
                } else {
                    elements_required = ff - current_frame + fs.size();
                }

                if (fs.size() < elements_required) {
                    size_t old_size = fs.size();

                    fs.resize(elements_required);
                    if (current_frame >= ff) {
                        for (size_t i = old_size; i < elements_required; i++) {
                            fs[i].transformations.resize(asf->bones().size());
                        }
                    } else {
                        // lel
                        memmove(fs.data() + ff - current_frame, fs.data(), old_size);
                        for (int i = 0; i < ff - current_frame; i++) {
                            fs[i].transformations.reserve(asf->bones().size());
                        }

                        ff = current_frame;
                    }
                }

                cfi = current_frame - ff;
            } else {
                std::stringstream line_ss(line);
                std::string bone_name;

                line_ss >> bone_name;
                if (bone_name == "root") {
                    for (ASF::Axis axis: asf->root_order()) {
                        if (line_ss.eof()) {
                            throw std::invalid_argument("Missing axis/axes for root (frame " + std::to_string(current_frame) + ")");
                        }

                        switch (axis) {
                            case ASF::RX: line_ss >> fs[cfi].root_rotation.x(); break;
                            case ASF::RY: line_ss >> fs[cfi].root_rotation.y(); break;
                            case ASF::RZ: line_ss >> fs[cfi].root_rotation.z(); break;
                            case ASF::TX: line_ss >> fs[cfi].root_translation.x(); break;
                            case ASF::TY: line_ss >> fs[cfi].root_translation.y(); break;
                            case ASF::TZ: line_ss >> fs[cfi].root_translation.z(); break;
                            default: throw std::invalid_argument("Unknown root axis");
                        }
                    }

                    if (!line_ss.eof()) {
                        throw std::invalid_argument("Too many axes given for root (frame " + std::to_string(current_frame) + ")");
                    }

                    fs[cfi].root_rotation *= angle_unit;
                    fs[cfi].root_translation *= asf->internal_length_unit();
                } else {
                    const auto &bi = bone_indices.find(bone_name);
                    if (bi == bone_indices.end()) {
                        throw std::invalid_argument("Unkown bone " + bone_name + " specified (frame " + std::to_string(current_frame) + ")");
                    }

                    Transformation &trans = fs[cfi].transformations[bi->second];
                    const ASF::Bone &bone = asf->bones()[bi->second];
                    for (ASF::Axis axis: bone.dof_order) {
                        if (line_ss.eof()) {
                            throw std::invalid_argument("Missing axis/axes for bone " + bone_name + " (frame " + std::to_string(current_frame) + ")");
                        }

                        float val;
                        line_ss >> val;

                        switch (axis) {
                            case ASF::RX: trans.rx = val * angle_unit; break;
                            case ASF::RY: trans.ry = val * angle_unit; break;
                            case ASF::RZ: trans.rz = val * angle_unit; break;
                            default: throw std::invalid_argument("Unknown DOF " + std::to_string(static_cast<int>(axis)) + " for bone " + bone_name);
                        }
                    }

                    if (!line_ss.eof()) {
                        throw std::invalid_argument("Too many axes for bone " + bone_name + " (frame " + std::to_string(current_frame) + ")");
                    }
                }
            }
        }
    }
}


void AMC::apply_frame(int frame)
{
    if ((frame < ff) || (frame - ff >= static_cast<int>(fs.size()))) {
        throw std::range_error("AMC frame out of bounds");
    }

    mat4 root_mv(mat4::identity());
    root_mv.translate(asf->root_position());
    root_mv.translate(fs[frame - ff].root_translation);

    for (auto it = asf->root_axis().rbegin(); it != asf->root_axis().rend(); ++it) {
        switch (*it) {
            case ASF::RX: root_mv.rotate(fs[frame - ff].root_rotation.x(), vec3(1.f, 0.f, 0.f)); break;
            case ASF::RY: root_mv.rotate(fs[frame - ff].root_rotation.y(), vec3(0.f, 1.f, 0.f)); break;
            case ASF::RZ: root_mv.rotate(fs[frame - ff].root_rotation.z(), vec3(0.f, 0.f, 1.f)); break;
            default: throw std::invalid_argument("Bad rotation axis");
        }
    }

    apply_frame_to_bone(asf->root_index(), frame, root_mv);
}


void AMC::apply_frame_to_bone(int bi, int frame, const mat4 &mv)
{
    ASF::Bone &bone = asf->bones()[bi];


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
            case ASF::RX: bone.local_trans.rotate(bone.axis.x(), vec3(1.f, 0.f, 0.f)); break;
            case ASF::RY: bone.local_trans.rotate(bone.axis.y(), vec3(0.f, 1.f, 0.f)); break;
            case ASF::RZ: bone.local_trans.rotate(bone.axis.z(), vec3(0.f, 0.f, 1.f)); break;
            default: throw std::invalid_argument("Bad rotation axis");
        }
    }

    bone.local_trans_inv = bone.local_trans.inverse();


    bone.still_trans = mv;

    mat4 motion(mat4::identity());
    for (auto it = bone.axis_order.rbegin(); it != bone.axis_order.rend(); ++it) {
        switch (*it) {
            case ASF::RX: motion.rotate(fs[frame - ff].transformations[bi].rx, vec3(1.f, 0.f, 0.f)); break;
            case ASF::RY: motion.rotate(fs[frame - ff].transformations[bi].ry, vec3(0.f, 1.f, 0.f)); break;
            case ASF::RZ: motion.rotate(fs[frame - ff].transformations[bi].rz, vec3(0.f, 0.f, 1.f)); break;
            default: throw std::invalid_argument("Bad rotation axis");
        }
    }

    // hell yeah just make it the other way round (it works)
    bone.motion_trans = mv * bone.local_trans * motion * bone.local_trans_inv;


    mat4 child_mv(bone.motion_trans.translated(bone.length * bone.direction));
    for (int child = bone.first_child; child >= 0; child = asf->bones()[child].next_sibling) {
        apply_frame_to_bone(child, frame, child_mv);
    }
}
