#ifndef __PARTICLE_MOVING_MESSAGE_HPP__
#define __PARTICLE_MOVING_MESSAGE_HPP__

#include <array>
#include <ostream>
#include <map>
#include <limits>

#include "./particle.hpp"

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
struct particle_moving_message{
    std::array<long, DIMS> destination_id;
    particle<TIME, REAL, DIMS> moving_particle;
};


template<typename TIME, typename REAL, std::size_t DIMS>
TIME move_out_time(particle<TIME, REAL, DIMS> par, std::array<REAL, DIMS> corner, std::array<REAL, DIMS> size){
    TIME t = std::numeric_limits<TIME>::infinity();

    for(size_t i = 0; i<DIMS; i++){
        if(par.velocity[i] == 0){
            continue;
        }else if(par.velocity[i] > 0){
            /* moving to positive */
            REAL wall = std::max(corner[i], corner[i]+size[i]);
            if(par.position[i] > wall || wall == std::numeric_limits<TIME>::infinity()){
                continue;
            }else{
                t = std::min(((wall-par.position[i])/par.velocity[i]), t);
            }
        }else{
            /* moving to negative */
            REAL wall = std::min(corner[i], corner[i]+size[i]);
            if(par.position[i] < wall || wall == -std::numeric_limits<TIME>::infinity()){
                continue;
            }else{
                t = std::min(((par.position[i]-wall)/par.velocity[i]), t);
            }
        }
    }
    return t;
}

template<typename TIME, typename REAL, std::size_t DIMS>
std::array<long, DIMS> move_out_destination(particle<TIME, REAL, DIMS> par, std::array<REAL, DIMS> corner, std::array<REAL, DIMS> size, std::array<long, DIMS> volume_id){
    TIME t = std::numeric_limits<TIME>::infinity();
    std::array<long, DIMS> dest = volume_id;

    for(size_t i = 0; i<DIMS; i++){
        if(par.velocity[i] == 0){
            continue;
        }else if(par.velocity[i] > 0){
            /* moving to positive */
            REAL wall = std::max(corner[i], corner[i]+size[i]);
            if(par.position[i] > wall || wall == std::numeric_limits<TIME>::infinity()){
                continue;
            }else{
                auto tt = ((wall-par.position[i])/par.velocity[i]);
                if(tt < t){
                    t = tt;
                    dest = volume_id;
                    dest[i] = dest[i]+1;
                }
            }
        }else{
            /* moving to negative */
            REAL wall = std::min(corner[i], corner[i]+size[i]);
            if(par.position[i] < wall || wall == -std::numeric_limits<TIME>::infinity()){
                continue;
            }else{
                auto tt = ((par.position[i]-wall)/par.velocity[i]);
                if(tt < t){
                    t = tt;
                    dest = volume_id;
                    dest[i] = dest[i]-1;
                }
            }
        }
    }
    return dest;
}


template<typename TIME, typename REAL, std::size_t DIMS>
std::ostream& operator<<(std::ostream& os, const particle_moving_message<TIME, REAL, DIMS>& msg) {
    os << "[[";

    for(size_t i = 0; i<DIMS; i++){
        if(i){
            os << ", ";
        }
        os << msg.destination_id[i];
    }

    return os << "], " << msg.moving_particle << "]";
}

}
#endif /* __PARTICLE_MOVING_MESSAGE_HPP__ */
