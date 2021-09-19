#ifndef __PARTICLE_DELTA_MESSAGE_HPP__
#define __PARTICLE_DELTA_MESSAGE_HPP__

#include <array>
#include <ostream>
#include <limits>

#include "./particle.hpp"

namespace tps{


template<typename TIME, typename REAL, std::size_t DIMS>
struct particle_delta_message{
    std::array<long, DIMS> volume_id;
    std::size_t particle_id;
    std::array<REAL, DIMS> dv;
    std::array<REAL, DIMS> deferred_dv;
    TIME deferred_dv_time;
};


template<typename TIME, typename REAL, std::size_t DIMS>
particle<TIME, REAL, DIMS> apply_delta(particle<TIME, REAL, DIMS> par, particle_delta_message<TIME, REAL, DIMS> delta_msg){
    std::cout << "delta_applied " << par << " and " << delta_msg << "\n";
    for(size_t i = 0; i<DIMS; i++){
        par.velocity[i] += delta_msg.dv[i];
        par.deferred_dv[i] += delta_msg.deferred_dv[i];
    }
    par.deferred_dv_time = std::min(par.deferred_dv_time, delta_msg.deferred_dv_time);
    return par;
}

template<typename TIME, typename REAL, std::size_t DIMS>
std::ostream& operator<<(std::ostream& os, const particle_delta_message<TIME, REAL, DIMS>& msg) {
    os << "[[";

    for(size_t i = 0; i<DIMS; i++){
        if(i){
            os << ", ";
        }
        os << msg.volume_id[i];
    }

    os << "], " << msg.particle_id << ", [";

    for(size_t i = 0; i<DIMS; i++){
        if(i){
            os << ", ";
        }
        os << msg.dv[i];
    }

    os << "]";

    if(msg.deferred_dv_time != std::numeric_limits<TIME>::infinity()){
        os << ", [";

        for(size_t i = 0; i<DIMS; i++){
            if(i){
                os << ", ";
            }
            os << msg.deferred_dv[i];
        }

        os << "], " << msg.deferred_dv_time;
    }

    return os << "]";

}

}
#endif /* __PARTICLE_DELTA_MESSAGE_HPP__ */
