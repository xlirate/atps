#ifndef __VOLUME_MODEL_HPP__
#define __VOLUME_MODEL_HPP__


#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <map>
#include <vector>
#include <utility>

#include "./particle.hpp"
#include "./particle_moving_message.hpp"
#include "./particle_delta_message.hpp"
#include "./particle_announcement_message.hpp"

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
struct volume_defs{

    struct particle_entering        : public cadmium::in_port<particle_moving_message<TIME, REAL, DIMS>> {};
    struct particle_delta           : public cadmium::in_port<particle_delta_message<TIME, REAL, DIMS>> {};

    struct particle_leaving         : public cadmium::out_port<particle_moving_message<TIME, REAL, DIMS>> {};
    struct particle_announcement    : public cadmium::out_port<particle_announcement_message<TIME, REAL, DIMS>> {};

};

template<typename TIME, typename REAL, std::size_t DIMS>
struct volume_model{

    struct state_type{
        /* These fields are core to the functioning of this model */
        std::array<long, DIMS> volume_id;
        std::array<REAL, DIMS> one_corner;
        std::array<REAL, DIMS> size;
        std::map<std::size_t, particle<TIME, REAL, DIMS>> particles{};


        /* These fields are here to make outputing possible */
        std::vector<size_t> pending_updates{};
        std::vector<size_t> pending_removals{};
        std::vector<particle_moving_message<TIME, REAL, DIMS>> pending_moves{};

        /* These fields are here to make that functioning faster */
        TIME next_internal_time{std::numeric_limits<TIME>::infinity()};

        /* everything in this model runs on absolute time, not reletive time, so we need this */
        TIME global_time;

        bool operator<(const state_type& state){
            return volume_id<state.volume_id;
        }

        bool operator==(const state_type& state){
            return volume_id==state.volume_id;
        }

        friend std::ostream& operator<<(std::ostream& os, const state_type& state) {

            os << "{\"id\":[";

            for(size_t i = 0; i<DIMS; i++){
                if(i){//if we are not on the first element
                    os << ", ";
                }
                os << state.volume_id[i];
            }

            os << "], \"corner\":[";

            for(size_t i = 0; i<DIMS; i++){
                if(i){//if we are not on the first element
                    os << ", ";
                }
                os << state.one_corner[i];
            }

            os << "], \"size\":[";

            for(size_t i = 0; i<DIMS; i++){
                if(i){//if we are not on the first element
                    os << ", ";
                }
                os << state.size[i];
            }

            os << "], \"particles\":[";

            bool first = true;
            for(const auto& kv : state.particles){
                if(first){//if we are not on the first element
                    first = false;
                }else{
                    os << ", ";
                }
                os << kv.second;
            }

            return os << "]}";

        }

    };
    state_type state;

    bool operator<(const volume_model<TIME, REAL, DIMS>& vol){
        return state.volume_id<vol.state.volume_id;
    }
    bool operator==(const volume_model<TIME, REAL, DIMS>& vol){
        return state.volume_id==vol.state.volume_id;
    }

    using input_ports = std::tuple<
        typename volume_defs<TIME, REAL, DIMS>::particle_entering,
        typename volume_defs<TIME, REAL, DIMS>::particle_delta
    >;

    using output_ports = std::tuple<
        typename volume_defs<TIME, REAL, DIMS>::particle_leaving,
        typename volume_defs<TIME, REAL, DIMS>::particle_announcement
    >;

    volume_model<TIME, REAL, DIMS>(){};
    volume_model<TIME, REAL, DIMS>(
            std::array<long, DIMS> volume_id,
            std::array<REAL, DIMS> one_corner,
            std::array<REAL, DIMS> size,
            std::vector<particle<TIME, REAL, DIMS>> particles = {}
        ){
        state.volume_id = volume_id;
        state.one_corner = one_corner;
        state.size = size;

        for(auto& p : particles){
            state.particles[p.id] = p;
            state.pending_updates.push_back(p.id);
        }
    }

    typename cadmium::make_message_bags<output_ports>::type output() const {
        typename cadmium::make_message_bags<output_ports>::type bag;

        for(auto move_msg : state.pending_moves){
            cadmium::get_messages<typename volume_defs<TIME, REAL, DIMS>::particle_leaving>(bag).push_back(move_msg);
        }

        if(state.pending_moves.size() || state.pending_updates.size()){
            cadmium::get_messages<typename volume_defs<TIME, REAL, DIMS>::particle_announcement>(bag).push_back({state.volume_id, state.pending_updates, state.pending_removals, &state.particles});
        }

        return bag;

    }

    void internal_transition(){
        state.global_time += time_advance();

        //We just got here from the output function, we can clear the queued updates.
        state.pending_updates.clear();
        state.pending_removals.clear();
        state.pending_moves.clear();

        //set the next internal time to inf, we bring it down as we find particles that need internal transitions sooner
        state.next_internal_time = std::numeric_limits<TIME>::infinity();

        for(auto& kv : state.particles){
            // k -> key, v -> value, very creative
            auto& k = kv.first;
            auto& v = kv.second;

            // if the particle is leaving the volume right now, have it leave
            // otherwise, if it has a deffered dv to apply right now, do so
            // otherwise compare both the next time it would leave the volume and the next time it would have an deffered dv to the standing 'next_internal_time' and take the min
            TIME next_move_out_time = move_out_time(v, state.one_corner, state.size);
            if(next_move_out_time <= state.global_time){
                //put the patricle into the moving-out queue, and add it to the removal update queue
                //we would remove it from state.particles here, but that would invalidate our iterator, so we wait until we finish first
                state.pending_moves.push_back({move_out_destination(v, state.one_corner, state.size, state.volume_id), v});
                state.pending_removals.push_back(k);
            }else if(v.deferred_dv_time <= state.global_time){
                //we simply calculate what the particle would look like after its dv is applied
                //we could advance it to now, but the function alrady advances it to when the dv was going to be applied, so the diference should be negligable
                v = apply_dv(v);
                state.pending_updates.push_back(v.id);
            }else{
                // std::min takes only 2 arguments, not variadic arguments, so we need to place 2 calls to resolve the min of 3 elements
                state.next_internal_time = std::min(state.next_internal_time, std::min(next_move_out_time, v.deferred_dv_time));
            }
        }
        //We remove all of the particles that move out of this volume here
        for(size_t i : state.pending_removals){
            state.particles.extract(i);
        }
    }

    void external_transition(TIME dt, typename cadmium::make_message_bags<input_ports>::type mbs) {
        state.global_time += dt;

        for(const auto& move_msg : cadmium::get_messages<typename volume_defs<TIME, REAL, DIMS>::particle_entering>(mbs)){
            //we take each moving particle who's destination is this volume and add it, and queue an update about it
            if(move_msg.destination_id == state.volume_id){
                state.particles[move_msg.moving_particle.id] = move_msg.moving_particle;
                state.pending_updates.push_back(move_msg.moving_particle.id);
            }
        }

        for(const auto& delta_msg : cadmium::get_messages<typename volume_defs<TIME, REAL, DIMS>::particle_delta>(mbs)){
            if(delta_msg.volume_id == state.volume_id){
                if(state.particles.count(delta_msg.particle_id)){
                    //for each incoming delta message, if we have a particle with that id, we apply the delta and queue an update message about it
                    auto& par = state.particles[delta_msg.particle_id];
                    par = apply_delta(advance_to_time(par, state.global_time), delta_msg);
                    state.pending_updates.push_back(par.id);
                }else{
                    for(auto& pp : state.pending_moves){
                        if(pp.moving_particle.id == delta_msg.particle_id){
                            //it is possible for a delta to come in for a particle that is in the queue to leave this volume.
                            //We do not want to miss deltas if we can avoid it, so we apply the delta here
                            //There is no need to send an anouncement, as the volume that is going to get this particle is going to anounce it there
                            pp.moving_particle = apply_delta(pp.moving_particle, delta_msg);
                        }
                    }
                }
            }
        }
    }

    void confluence_transition(TIME, typename cadmium::make_message_bags<input_ports>::type mbs) {
        internal_transition();
        external_transition(TIME{}, std::move(mbs));
    }


    TIME time_advance() const {
        if(state.pending_moves.size() || state.pending_updates.size()){
            return {0};
        }else{
            return std::max(state.next_internal_time-state.global_time, {0});
        }
    }


    friend std::ostream& operator<<(std::ostream& os, const volume_model& vol) {
        return os << vol.state;
    }


};



}
#endif /* __VOLUME_MODEL_HPP__ */
