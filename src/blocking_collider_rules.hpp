#ifndef __BLOCKING_COLLIDER_RULES_HPP__
#define __BLOCKING_COLLIDER_RULES_HPP__

#include "./particle.hpp"
#include "./particle_delta_message.hpp"

#include <cmath>

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
TIME blocking_collide_time(const particle<TIME, REAL, DIMS>& lhs, const particle<TIME, REAL, DIMS>& rhs){
    std::cout << "c_time_check " << lhs.id << " and " << rhs.id;
    REAL radius = lhs.radius+rhs.radius;

    REAL a=0, b=0, c= -radius*radius; /* we bake the r^2 in here to save a bit of time */

    for(size_t i = 0; i<DIMS; i++){
        REAL p = (lhs.position[i]-(lhs.velocity[i]*lhs.last_updated)) - (rhs.position[i]-(rhs.velocity[i]*rhs.last_updated));
        REAL v = lhs.velocity[i]-rhs.velocity[i];

        a += v*v;
        b += 2*v*p;
        c += p*p;
    }

    /*
    collisions only happen when
    0 == att + bt + c
    use quadratic formula
    */
    REAL core = b*b-4*a*c;
    if(core <= 0){
        /* no real collisions, or they just kiss, but we will consider a kiss a miss because they would empart no dv anyway */
        std::cout << "\n";
        return std::numeric_limits<TIME>::infinity();
    }else{
        /*
            (-b +- sqrt(b*b-4*a*c))/2a
            We want the root where we are getting closer together, so the root where 2at+b is negative
            One root will be positive and the other negative. If a is positive it will the the lower one, if it is negative it will be the higher one
            a is the sum of all positive numbers, so a will always be positive
            We always want the first root
        */
        std::cout << " Hit! Time: " << ((-b)-sqrt(core))/(2*a) << "\n";
        const TIME hit_time = ((-b)-sqrt(core))/(2*a);
        if(hit_time >= lhs.last_updated && hit_time >= rhs.last_updated){
            return ((-b)-sqrt(core))/(2*a);
        }else{
            return std::numeric_limits<TIME>::infinity();
        }

    }
}


/*
    at time t, colide these two particles and generate the deltas
*/
template<typename TIME, typename REAL, std::size_t DIMS>
std::array<particle_delta_message<TIME, REAL, DIMS>, 2> blocking_collide_with_deferred(particle<TIME, REAL, DIMS> lhs, particle<TIME, REAL, DIMS> rhs, TIME t, TIME stick_time = 0.1){

    lhs = advance_to_time(lhs, t);
    rhs = advance_to_time(rhs, t);
    /*
    std::array<long, DIMS> volume_id;
    std::size_t particle_id;
    std::array<REAL, DIMS> dv;
    std::array<REAL, DIMS> deferred_dv;
    TIME deferred_dv_time;
    */
    std::array<particle_delta_message<TIME, REAL, DIMS>, 2> out{};
    out[0] = {{},lhs.id,{},{}, t+stick_time};
    out[1] = {{},rhs.id,{},{}, t+stick_time};

    for(std::size_t i = 0; i<DIMS; i++){
        REAL average_v_i = (lhs.mass * lhs.velocity[i] + rhs.mass * rhs.velocity[i])/(lhs.mass+rhs.mass);
        out[0].dv[i] = average_v_i - lhs.velocity[i];
        out[1].dv[i] = average_v_i - rhs.velocity[i];
    }

    std::array<REAL, DIMS> relative_position = {0};
    REAL relative_position_s = 0;
    std::array<REAL, DIMS> relative_velocity = {0};
    REAL relative_velocity_s = 0;
    for(std::size_t i = 0; i<DIMS; i++){
        relative_position[i] = rhs.position[i] - lhs.position[i];
        relative_position_s += relative_position[i]*relative_position[i];
        relative_velocity[i] = rhs.velocity[i] - lhs.velocity[i];
        relative_velocity_s += relative_velocity[i]*relative_velocity[i];
    }
    relative_position_s = std::sqrt(relative_position_s);
    relative_velocity_s = std::sqrt(relative_velocity_s);

    std::array<REAL, DIMS> unit_relative_position = {0};
    std::array<REAL, DIMS> unit_relative_velocity = {0};
    REAL relative_dot = 0;
    for(std::size_t i = 0; i<DIMS; i++){
        unit_relative_position[i] = relative_position[i]/relative_position_s;
        unit_relative_velocity[i] = relative_velocity[i]/relative_velocity_s;
        relative_dot += unit_relative_position[i]*unit_relative_velocity[i];
    }

    std::array<REAL, DIMS> impulse = {};
    for(std::size_t i = 0; i<DIMS; i++){
        impulse[i] = relative_velocity[i]*unit_relative_position[i];
    }

    for(std::size_t i = 0; i<DIMS; i++){
        out[0].deferred_dv[i] = -2*(rhs.mass/(lhs.mass+rhs.mass))*impulse[i] - out[0].dv[i];
        out[1].deferred_dv[i] =  2*(lhs.mass/(lhs.mass+rhs.mass))*impulse[i] - out[1].dv[i];
    }

    return out;
}


/*
    at time t, colide these two particles and generate the deltas
*/
template<typename TIME, typename REAL, std::size_t DIMS>
std::array<particle_delta_message<TIME, REAL, DIMS>, 2> blocking_collide(particle<TIME, REAL, DIMS> lhs, particle<TIME, REAL, DIMS> rhs, TIME t){

    std::cout << "collide " << lhs << " and " << rhs << "\n";

    lhs = advance_to_time(lhs, t);
    rhs = advance_to_time(rhs, t);
    /*
    std::array<long, DIMS> volume_id;
    std::size_t particle_id;
    std::array<REAL, DIMS> dv;
    std::array<REAL, DIMS> deferred_dv;
    TIME deferred_dv_time;
    */
    std::array<particle_delta_message<TIME, REAL, DIMS>, 2> out{};
    out[0] = {{},lhs.id,{},{}, std::numeric_limits<TIME>::infinity()};
    out[1] = {{},rhs.id,{},{}, std::numeric_limits<TIME>::infinity()};

    std::array<REAL, DIMS> relative_position = {0};
    REAL relative_position_s = 0;
    std::array<REAL, DIMS> relative_velocity = {0};
    REAL relative_velocity_s = 0;
    for(std::size_t i = 0; i<DIMS; i++){
        relative_position[i] = rhs.position[i] - lhs.position[i];
        relative_position_s += relative_position[i]*relative_position[i];
        relative_velocity[i] = rhs.velocity[i] - lhs.velocity[i];
        relative_velocity_s += relative_velocity[i]*relative_velocity[i];
    }
    relative_position_s = std::sqrt(relative_position_s);
    relative_velocity_s = std::sqrt(relative_velocity_s);

    std::array<REAL, DIMS> unit_relative_position = {0};
    std::array<REAL, DIMS> unit_relative_velocity = {0};
    REAL relative_dot = 0;
    for(std::size_t i = 0; i<DIMS; i++){
        unit_relative_position[i] = relative_position[i]/relative_position_s;
        unit_relative_velocity[i] = relative_velocity[i]/relative_velocity_s;
        relative_dot += unit_relative_position[i]*unit_relative_velocity[i];
    }

    std::array<REAL, DIMS> impulse = {};
    for(std::size_t i = 0; i<DIMS; i++){
        impulse[i] = relative_velocity[i]*unit_relative_position[i];
    }

    for(std::size_t i = 0; i<DIMS; i++){
        out[0].dv[i] = -2*(rhs.mass/(lhs.mass+rhs.mass))*impulse[i];
        out[1].dv[i] =  2*(lhs.mass/(lhs.mass+rhs.mass))*impulse[i];
    }

    return out;
}


}

#endif /* __BLOCKING_COLLIDER_RULES_HPP__ */
