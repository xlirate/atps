#!/bin/python3

import sys
import matplotlib.pyplot as plt
from atps_output_tools import parse_msg_file, float_range_helper, quantize_state_to_times

def transpose_states(states):
    out = {}
    times = []
    for time, state in states:
        times.append(time)
        for k,v in state.items():
            if k not in out:
                out[k] = []
            for i,p in enumerate(v):
                if(len(out[k]) < i+1):
                    out[k].append([])
                out[k][i].append(p)
    return times, out


if __name__ == "__main__":
    msg_file_path = "./simulation_results/output_messages.txt" if len(sys.argv) <= 1 else sys.argv[1]
    run_time      = 5     if len(sys.argv) <= 2 else float(sys.argv[2])
    dt            = 0.001 if len(sys.argv) <= 3 else float(sys.argv[3])
    start_time    = 0     if len(sys.argv) <= 4 else float(sys.argv[4])

    with open(msg_file_path) as msg_file:
        times, data = transpose_states(quantize_state_to_times(parse_msg_file(msg_file), float_range_helper(start_time, run_time, dt)))
        for name, path in data.items():
            if len(path) == 1:
                plt.plot(times[-len(path[0]):], path[0])#, path[1])
            else:
                plt.plot(path[0], path[1])

        plt.show()
