#ifndef AMC_HPP
#define AMC_HPP

#include <fstream>
#include <iostream>
#include <vector>

#include <dake/math/matrix.hpp>

#include "asf.hpp"


class AMC {
    public:
        struct Transformation {
            float rx = 0.f;
            float ry = 0.f;
            float rz = 0.f;
        };

        struct Frame {
            dake::math::vec3 root_translation = dake::math::vec3::zero();
            dake::math::vec3 root_rotation    = dake::math::vec3::zero();
            // Index is equal to the bone index in ASF.bones
            std::vector<Transformation> transformations;
        };

        AMC(std::ifstream &s, ASF *asf);

        int first_frame(void) const { return ff; }

        const std::vector<Frame> &frames(void) const { return fs; }

        void apply_frame(int frame);


    private:
        void apply_frame_to_bone(int bi, int frame, const dake::math::mat4 &mv);

        ASF *asf;
        std::vector<Frame> fs;
        int ff = -1;
};

#endif
