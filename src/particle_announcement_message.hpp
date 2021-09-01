#ifndef __PARTICLE_ANNOUNCEMENT_MESSAGE_HPP__
#define __PARTICLE_ANNOUNCEMENT_MESSAGE_HPP__

#include <array>
#include <vector>
#include <map>
#include <ostream>

#include "./particle.hpp"

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
struct particle_announcement_message{
    std::array<long, DIMS> volume_id;
    std::vector<size_t> particle_changed;
    std::vector<size_t> particle_removed;
    const std::map<std::size_t, particle<TIME, REAL, DIMS>>* volume_update;
};

template<typename TIME, typename REAL, std::size_t DIMS>
std::ostream& operator<<(std::ostream& os, const particle_announcement_message<TIME, REAL, DIMS>& msg) {
    os << "[[";

    for(size_t i = 0; i<DIMS; i++){
        if(i){
            os << ", ";
        }
        os << msg.volume_id[i];
    }

    os << "], [";

    for(size_t i = 0; i<msg.particle_changed.size(); i++){
        if(i){
            os << ", ";
        }
        os << msg.volume_update->at(msg.particle_changed[i]);
    }

    os << "], [";

    for(size_t i = 0; i<msg.particle_removed.size(); i++){
        if(i){
            os << ", ";
        }
        os << msg.particle_removed[i];
    }

    return os << "]]";
}

}
#endif /* __PARTICLE_ANNOUNCEMENT_MESSAGE_HPP__ */
