#ifndef __BLOCKING_COLLIDER_RULES_HPP__
#define __BLOCKING_COLLIDER_RULES_HPP__

#include "./particle.hpp"
#include "./particle_delta_message.hpp"

#include <cmath>
#include <array>

namespace tps{

template<typename TIME, typename REAL, std::size_t DIMS>
TIME blocking_collide_time(particle<TIME, REAL, DIMS> lhs, particle<TIME, REAL, DIMS> rhs, TIME global_time){
    lhs = advance_to_time(lhs, global_time);
    rhs = advance_to_time(rhs, global_time);

    REAL radius = lhs.radius+rhs.radius;

    std::array<REAL, DIMS> rel_vel{};
    std::array<REAL, DIMS> rel_pos{};
    REAL dist{};
    for(size_t i = 0; i<DIMS; i++){
        rel_vel[i] = lhs.velocity[i]-rhs.velocity[i];
        rel_pos[i] = lhs.position[i]-rhs.position[i];
        dist += rel_pos[i]*rel_pos[i];
    }

    REAL a=0, b=0, c=0;

    for(size_t i = 0; i<DIMS; i++){

        a += rel_vel[i]*rel_vel[i];
        b += 2*rel_vel[i]*rel_pos[i];
        c += rel_pos[i]*rel_pos[i];
    }
    c -= radius*radius;

    REAL d = (b*b)-(4*a*c);

    if(b >= 0){
        /* they are already moving appart*/
        return std::numeric_limits<TIME>::infinity();
    }else if(dist < radius*radius){
        /* they are intersecting and not moving appart */
        //std::cout << lhs.id << " and " << rhs.id << " are intersecting\n";
        return global_time;

    }else if(d < 0){
        /* no collision */
        return std::numeric_limits<TIME>::infinity();
    }else{
        /*
            (-b +- sqrt(b*b-4*a*c))/2a
            We want the root where we are getting closer together, so the root where 2at+b is negative
            One root will be positive and the other negative. If a is positive it will the the lower one, if it is negative it will be the higher one
            a is the sum of all positive numbers, so a will always be positive
            We always want the first root
        */
        return std::max(((-b)-std::sqrt(d))/(2*a), REAL{0})+global_time;
    }
}


/*
    at time t, colide these two particles and generate the deltas
*/
template<typename TIME, typename REAL, std::size_t DIMS>
std::array<particle_delta_message<TIME, REAL, DIMS>, 2> blocking_collide(particle<TIME, REAL, DIMS> lhs, particle<TIME, REAL, DIMS> rhs, TIME t, REAL losses = 0.0, TIME stick_time = 0.000001, REAL extra_push = 0.001){

    lhs = advance_to_time(lhs, t);
    rhs = advance_to_time(rhs, t);

    for(std::size_t i = 0; i<DIMS; i++){
        lhs.velocity[i]    += lhs.deferred_dv[i];
        lhs.deferred_dv[i] = REAL{0};

        rhs.velocity[i]    += rhs.deferred_dv[i];
        rhs.deferred_dv[i] = REAL{0};
    }
    if(blocking_collide_time(lhs, rhs, t) > t){
        /* we only need to flush the deffered dv */
        std::array<particle_delta_message<TIME, REAL, DIMS>, 2> out{};
        out[0] = {{},lhs.id,{},{}, t};
        out[1] = {{},rhs.id,{},{}, t};
        return out;
    }

    std::array<particle_delta_message<TIME, REAL, DIMS>, 2> out{};
    out[0] = {{},lhs.id,{},{}, t+stick_time};
    out[1] = {{},rhs.id,{},{}, t+stick_time};

    REAL push_term = 1+extra_push*(lhs.hits_since_last_deferred_dv_clear+rhs.hits_since_last_deferred_dv_clear);

    for(std::size_t i = 0; i<DIMS; i++){
        REAL average_v_i = (lhs.mass * lhs.velocity[i] + rhs.mass * rhs.velocity[i])/(lhs.mass+rhs.mass);
        out[0].dv[i] = (average_v_i - lhs.velocity[i])*(push_term);
        out[1].dv[i] = (average_v_i - rhs.velocity[i])*(push_term);
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
        impulse[i] = relative_velocity[i]*unit_relative_position[i]*(1-losses);
    }

    for(std::size_t i = 0; i<DIMS; i++){
        out[0].deferred_dv[i] =  2*(rhs.mass/(lhs.mass+rhs.mass))*impulse[i] - out[0].dv[i];
        out[1].deferred_dv[i] = -2*(lhs.mass/(lhs.mass+rhs.mass))*impulse[i] - out[1].dv[i];
    }

    return out;
}



}

#endif /* __BLOCKING_COLLIDER_RULES_HPP__ */
