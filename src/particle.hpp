#ifndef __PARTICLE_HPP__
#define __PARTICLE_HPP__

#include <cstddef>
#include <array>
#include <limits>
#include <iostream>

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
struct particle{
    //specifically has no constructor or destructor
    TIME last_updated;
    size_t id;
    size_t species;
    REAL mass;
    REAL radius;
    std::array<REAL, DIMS> position;
    std::array<REAL, DIMS> velocity;
    std::array<REAL, DIMS> deferred_dv;
    TIME deferred_dv_time; /* use this for no defered -> std::numeric_limits<TIME>::infinity */

};

//apply_dv(v)
template<typename TIME, typename REAL, std::size_t DIMS>
particle<TIME, REAL, DIMS> apply_dv(particle<TIME, REAL, DIMS> par){
    par = advance_to_time(par, par.deferred_dv_time);
    for(size_t i=0; i<DIMS; i++){
        par.velocity[i] += par.deferred_dv[i];
    }
    par.deferred_dv = {};
    par.deferred_dv_time = std::numeric_limits<TIME>::infinity();
    return par;
}

template<typename TIME, typename REAL, std::size_t DIMS>
particle<TIME, REAL, DIMS> advance_to_time(particle<TIME, REAL, DIMS> par, TIME t){
    /*
    if(t < par.last_updated){
        std::cerr << "This particle was asked to go back in time. " << t << " is before " << par.last_updated << ".\n";
    }
    */

    auto dt = t - par.last_updated;
    for(size_t i=0; i<DIMS; i++){
        par.position[i] += par.velocity[i]*dt;
    }
    par.last_updated = t;
    return par;
}



template<typename TIME, typename REAL, std::size_t DIMS>
std::ostream &operator<<(std::ostream &os, const particle<TIME, REAL, DIMS> &p) {
    os << "[" << p.last_updated << ", " << p.id << ", " << p.species << ", " << p.mass << ", " << p.radius << ", [";

    bool first = true;
    for(const auto pn : p.position){
        if(first){
            first = false;
        }else{
            os << ", ";
        }
        os << pn;
    }
    os << "], [";
    first = true;
    for(const auto vn : p.velocity){
        if(first){
            first = false;
        }else{
            os << ", ";
        }
        os << vn;
    }

    if(p.deferred_dv_time != std::numeric_limits<TIME>::infinity()){
        os << "], [";
        first = true;
        for(const auto dn : p.deferred_dv){
            if(first){
                first = false;
            }else{
                os << ", ";
            }
            os << dn;
        }
        os << "], " << p.deferred_dv_time;
    }
    return os << "]";
}

}

#endif /* __PARTICLE_HPP__ */
