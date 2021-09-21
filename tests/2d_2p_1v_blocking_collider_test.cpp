//Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>


#include "./../src/blocking_collider_model.hpp"
#include "./../src/particle.hpp"
#include "./../src/volume_model.hpp"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <string>
#include <fstream>


/***** Define input port for coupled models *****/

/***** Define input port for coupled models *****/

using namespace cadmium;
using namespace tps;

using TIME = double;

template<typename TT>
using volume_model_2d = volume_model<TT, double, 2>;

template<typename TT>
using blocking_collider_model_2d = blocking_collider_model<TT, double, 2>;

template<typename TT>
using particle_2d = particle<TT, double, 2>;

int main(int argc, char ** argv) {
    // the particles get inited in order like this [last_updated, id, species, mass, radius, [position], [velocity], [deferred_dv], deferred_dv_time]

    std::shared_ptr<dynamic::modeling::model> vol_0 = dynamic::translate::make_dynamic_atomic_model<volume_model_2d, TIME>(
        "vol_0", std::array<long, 2>{0, 0}, std::array<double, 2>{0.0, 0.0}, std::array<double, 2>{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()},
        std::vector<particle_2d<TIME>>{
            {{0}, {2}, {0}, {1}, {1}, {10, 60}, {2, -10}, {0}, {std::numeric_limits<TIME>::infinity()}},
            {{0}, {1}, {0}, {3}, {1}, {10, 40}, {2,  10}, {0}, {std::numeric_limits<TIME>::infinity()}},
        });

    std::shared_ptr<dynamic::modeling::model> b_col = dynamic::translate::make_dynamic_atomic_model<blocking_collider_model_2d, TIME>("b_col");


    dynamic::modeling::Ports iports_TOP{};
    dynamic::modeling::Ports oports_TOP{};
    dynamic::modeling::Models submodels_TOP{vol_0, b_col};
    dynamic::modeling::EICs eics_TOP{};
    dynamic::modeling::EOCs eocs_TOP{};
    dynamic::modeling::ICs ics_TOP{
        dynamic::translate::make_IC<volume_defs<TIME, double, 2>::particle_announcement, blocking_defs<TIME, double, 2>::particle_announcement>("vol_0", "b_col"),
        dynamic::translate::make_IC<blocking_defs<TIME, double, 2>::particle_delta, volume_defs<TIME, double, 2>::particle_delta>("b_col", "vol_0")
    };

    std::shared_ptr<dynamic::modeling::coupled<TIME>> TOP = std::make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*** Loggers ***/
    static std::ofstream out_messages("./simulation_results/output_messages.txt");
    struct oss_sink_messages{
        static std::ostream& sink(){
            return out_messages;
        }
    };
    static std::ofstream out_state("./simulation_results/output_state.txt");
    struct oss_sink_state{
        static std::ostream& sink(){
            return out_state;
        }
    };

    using state=logger::logger<logger::logger_state, dynamic::logger::formatter<TIME>, oss_sink_state>;
    using log_messages=logger::logger<logger::logger_messages, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_mes=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_state>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;

    /*** Runner call ***/
    dynamic::engine::runner<TIME, logger_top> r(TOP, {0});
    //r.run_until(NDTime("00:05:00:000"));
    std::cout << "Starting it up!\n";
    r.run_until(TIME{100});
    std::cout << "Wrapping it up!\n";
    return 0;

}

