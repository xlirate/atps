#ifndef __BLOCKING_COLLIDER_MODEL_HPP__
#define __BLOCKING_COLLIDER_MODEL_HPP__


#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <map>
#include <vector>
#include <utility>
#include <tuple>
#include <algorithm>

#include "./particle.hpp"
#include "./particle_delta_message.hpp"
#include "./particle_announcement_message.hpp"
#include "./blocking_collider_rules.hpp"

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
struct blocking_defs{

    struct particle_announcement    : public cadmium::in_port<particle_announcement_message<TIME, REAL, DIMS>> {};

    struct particle_delta           : public cadmium::out_port<particle_delta_message<TIME, REAL, DIMS>> {};

};

template<typename TIME, typename REAL, std::size_t DIMS>
struct blocking_collider_model{
    struct state_type{
        TIME global_time{0};
        std::vector<particle_delta_message<TIME, REAL, DIMS>> pending_deltas{};

        std::map<std::array<long, DIMS>, std::tuple<
            const std::map<std::size_t, particle<TIME, REAL, DIMS>>*, //"copy" of the state of the volume
            std::size_t, //lhs, in this volume, or -1 for no collision
            std::size_t, //rhs, not always in this volume and higher than lhs
            std::array<long, DIMS>, //the volume id that rhs is in
            TIME //the time of the collision
        >> volumes{};

        TIME next_internal_time{};

        friend std::ostream& operator<<(std::ostream& os, const state_type& state) {
            return os;
        }

    };
    state_type state;


    using input_ports = std::tuple<
        typename blocking_defs<TIME, REAL, DIMS>::particle_announcement
    >;

    using output_ports = std::tuple<
        typename blocking_defs<TIME, REAL, DIMS>::particle_delta
    >;

    blocking_collider_model<TIME, REAL, DIMS>(){};

    typename cadmium::make_message_bags<output_ports>::type output() const {
        typename cadmium::make_message_bags<output_ports>::type bag;

        for(auto delta_msg : state.pending_deltas){
            cadmium::get_messages<typename blocking_defs<TIME, REAL, DIMS>::particle_delta>(bag).push_back(delta_msg);
        }

        return bag;

    }

    void internal_transition(){
        state.global_time += time_advance();

        //We just got here from the output function, we can clear the queued deltas.
        state.pending_deltas.clear();
        state.next_internal_time = std::numeric_limits<TIME>::infinity();

        for(auto& kv : state.volumes){
            const std::array<long, DIMS> k = kv.first;
            auto& v = kv.second;

            if(std::get<4>(v) <= state.global_time){
                const auto& v_id_l = k;
                const auto& v_id_r = std::get<3>(v);

                auto deltas = blocking_collide(std::get<0>(v)->at(std::get<1>(v)), std::get<0>(state.volumes.at(v_id_r))->at(std::get<2>(v)), state.global_time);

                deltas[0].volume_id = v_id_l;
                deltas[1].volume_id = v_id_r;

                state.pending_deltas.push_back(deltas[0]);
                state.pending_deltas.push_back(deltas[1]);

                std::get<4>(v) = std::numeric_limits<TIME>::infinity();
                //this is a good place to invalidate cache, but not implementing well that yet
            }
        }

    }

    void external_transition(TIME dt, typename cadmium::make_message_bags<input_ports>::type mbs) {
        state.global_time += dt;
        std::vector<std::array<long, DIMS>> dirty_volumes{};
        for(const auto& msg : cadmium::get_messages<typename blocking_defs<TIME, REAL, DIMS>::particle_announcement>(mbs)){
            dirty_volumes.push_back(msg.volume_id);

            if(!state.volumes.count(msg.volume_id)){
                state.volumes[msg.volume_id] = {msg.volume_update, (size_t)(-1), (size_t)(-1), {}, std::numeric_limits<TIME>::infinity()};
            }else{
                std::get<0>(state.volumes[msg.volume_id]) = msg.volume_update;
            }
        }
        if(dirty_volumes.size()){
            //deduplicate dirty_volumes
            std::sort( dirty_volumes.begin(), dirty_volumes.end() );
            dirty_volumes.erase( std::unique( dirty_volumes.begin(), dirty_volumes.end() ), dirty_volumes.end() );

            for(const auto& lk : dirty_volumes){ //for each volume that changed
                auto& lv = state.volumes.at(lk);
                std::get<4>(lv) = std::numeric_limits<TIME>::infinity(); //we *are* replacing this

                for(auto& rkv : state.volumes){ //for each volume
                    // TODO only iterate over the keys that are good, not all of the keys
                    const auto& rk = rkv.first;
                    bool good = true;
                    for(size_t i = 0; i<DIMS; i++){
                        good &= lk[i] == rk[i]+1 || lk[i] == rk[i] || lk[i] == rk[i]-1;
                    }
                    if(good){//if the second volume is near enough the first or is the first

                        auto& rv = rkv.second;
                        for(const auto& lpkv : *std::get<0>(lv)){//for each particle in the first volume
                            const auto& lp = lpkv.second;
                            for(const auto& rpkv : *std::get<0>(rv)){//for each particle in the second volume
                                const auto& rp = rpkv.second;
                                const TIME tt = blocking_collide_time(lp, rp);
                                if(tt != std::numeric_limits<TIME>::infinity() && tt >= state.global_time){
                                    std::cout << tt << " " << state.global_time;
                                    //check the collision
                                    if(lp.id < rp.id && tt < std::get<4>(lv)){
                                        std::cout << " a\n";
                                        //this is the new hit for the left volume
                                        std::get<1>(lv) = lp.id;
                                        std::get<2>(lv) = rp.id;
                                        std::get<3>(lv) = rk;
                                        std::get<4>(lv) = tt;

                                    }else if(tt < std::get<4>(rv)){
                                        std::cout << " b\n";
                                        //this is the new hit for the right volume
                                        std::get<1>(rv) = rp.id;
                                        std::get<2>(rv) = lp.id;
                                        std::get<3>(rv) = lk;
                                        std::get<4>(rv) = tt;

                                    }else{
                                        std::cout << " c\n";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        std::cout << "find the next time ";
        for(const auto& kv : state.volumes){ //update the next internal time
            TIME tt = std::get<4>(kv.second);
            std::cout << tt << " ";
            if(tt < state.next_internal_time){
                state.next_internal_time = tt;
                std::cout << "U ";
            }
        }
        std::cout << "\n";
    }

    void confluence_transition(TIME, typename cadmium::make_message_bags<input_ports>::type mbs) {
        internal_transition();
        external_transition(TIME{}, std::move(mbs));
    }


    TIME time_advance() const {
        if(state.pending_deltas.size()){
            return {0};
        }else{
            return std::max(state.next_internal_time-state.global_time, {0});
        }
    }


    friend std::ostream& operator<<(std::ostream& os, const blocking_collider_model& bcm) {
        return os << bcm.state;
    }


};



}
#endif /* __BLOCKING_COLLIDER_MODEL_HPP__ */
