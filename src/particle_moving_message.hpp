#ifndef __PARTICLE_MOVING_MESSAGE_HPP__
#define __PARTICLE_MOVING_MESSAGE_HPP__

#include <array>
#include <ostream>
#include <map>
#include <limits>
#include <cmath>

#include "./particle.hpp"

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
struct particle_moving_message{
    std::array<long, DIMS> destination_id;
    particle<TIME, REAL, DIMS> moving_particle;
};

template<typename TIME, std::size_t DIMS>
struct time_and_direction{
    TIME moving_time;
    std::array<long, DIMS> destination_id;
};

template<typename TIME, typename REAL, std::size_t DIMS>
time_and_direction<TIME, DIMS> move_out(particle<TIME, REAL, DIMS> par, std::array<REAL, DIMS> corner, std::array<REAL, DIMS> size, const std::array<long, DIMS> volume_id = {}){
    TIME t = std::numeric_limits<TIME>::infinity();
    std::array<long, DIMS> dest = volume_id;

    for(size_t i = 0; i<DIMS; i++){

        REAL n_val, p_val;

        if(std::isfinite(corner[i]) && std::isfinite(size[i])){
            //c, c+s or c+s, c
            n_val = std::min(corner[i], corner[i]+size[i]);
            p_val = std::max(corner[i], corner[i]+size[i]);
        }else if(std::isfinite(corner[i]) && size[i] > 0){
            // c, inf
            n_val = corner[i];
            p_val = std::numeric_limits<TIME>::infinity();
        }else if(std::isfinite(corner[i]) && size[i] < 0){
            // -inf, c
            n_val = -std::numeric_limits<TIME>::infinity();
            p_val = corner[i];
        }else{
            // -inf, inf
            n_val = -std::numeric_limits<TIME>::infinity();
            p_val = std::numeric_limits<TIME>::infinity();
        }

        if(par.velocity[i] > 0 && par.position[i] <= p_val){
            TIME tt = (p_val-par.position[i])/par.velocity[i];
            if(tt < t){
                t = tt;
                dest = volume_id;
                dest[i]+=1;
            }
        }else if(par.velocity[i] < 0  && par.position[i] >= n_val){
            TIME tt = (n_val-par.position[i])/par.velocity[i];
            if(tt < t){
                t = tt;
                dest = volume_id;
                dest[i]-=1;
            }
        }
    }
    return {t+par.last_updated, dest};
}

template<typename TIME, typename REAL, std::size_t DIMS>
std::array<long, DIMS> move_out_destination(particle<TIME, REAL, DIMS> par, std::array<REAL, DIMS> corner, std::array<REAL, DIMS> size, const std::array<long, DIMS> volume_id){
    return move_out(par, corner, size, volume_id).destination_id;
}

template<typename TIME, typename REAL, std::size_t DIMS>
TIME move_out_time(particle<TIME, REAL, DIMS> par, std::array<REAL, DIMS> corner, std::array<REAL, DIMS> size){
    return move_out(par, corner, size).moving_time;
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
