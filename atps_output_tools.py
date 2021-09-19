#!/bin/python3


import sys
import json

def parse_msg_file(msg_file):
    start_s = ">::particle_announcement: {"
    end_s   = "}"

    time = float('-inf')
    for line in msg_file:
        #this code may need to change for new/different file formats
        if line.strip().replace('.','',1).isdigit():
            #this is a time change
            time = float(line.strip())
        elif start_s in line:
            #this is a line with particle info
            start_i = line.find(start_s)+len(start_s)
            end_i   = line.find(end_s, start_i)
            if(end_i > start_i):
                v_id, dv, leaving = json.loads(line[start_i:end_i])

                for p_time, p_id, p_species, p_mass, p_radius, p_pos, p_vel, *p_deferred_dv_and_time in dv:
                    yield([time, p_id, p_time, p_pos, p_vel])


def quantize_state_to_times(events, times):
    state = {}
    next_event = next(events, None)
    #print(0, next_event)
    for output_time in times:
        while next_event is not None and next_event[0] < output_time:
            event_time, pid, update_time, pos, vel = next_event

            state[pid] = (update_time, pos, vel)
            next_event = next(events, None)
            #print(output_time, next_event)

        out = {}
        for pid, (last_update_time, pos, vel) in state.items():
            out[pid] = list([p+v*(output_time-last_update_time) for p,v in zip(pos, vel)])
        yield output_time, out



def float_range_helper(start, end, step):
    while start <= end:
        yield(start)
        start += step

if __name__ == "__main__":
    #print(str(sys.argv))
    if len(sys.argv) > 1 and '-h' in sys.argv[1]:
        print(
        'Usage: \n'+
        '\tmessages.txt | python3 output_tools.py                                       #outputs a cleanned sequence of events of the form [time, p_id, [pos], [vel]]\n'+
        '\tpython3 output_tools.py messages.txt                                         #as if messages.txt was piped in\n'+
        '\tpython3 output_tools.py messages.txt <end time>                              #at each time in [0.0, end] with a stepsize of 1.0, print a snapeshot of the state, of the form [time, {p_id:[pos]}]\n'+
        '\tpython3 output_tools.py messages.txt <end time> <timestep size>              #as the last case, but with the specified step size instead of 1.0\n'+
        '\tpython3 output_tools.py messages.txt <end time> <timestep size> <start time> #as the last case, but with the specified start time instead of 0.0\n'
        )
        exit()
    if len(sys.argv) == 2:
        with open(sys.argv[1]) as msg_file:
            for event in parse_msg_file(msg_file):
                print(event)
    elif len(sys.argv) > 2:
        #end | end, step size | end, step size, start
        end  = float(sys.argv[2])
        step = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
        start = float(sys.argv[4]) if len(sys.argv) > 4 else 0.0
        if(start > end):
            print(f"start:{start} is > end:{end}, no states will be printed")
            exit(-1)
        if(step == 0):
            print("step size is set to 0. This program does not suport a step size of 0")
            exit(-1)
        if(step < 0):
            print(f"step:{step} is <0, we can only walk forwards through the input, we cannot produce states out of order or in reverse order like this")
            exit(-1)
        with open(sys.argv[1]) as msg_file:
            for state in quantize_state_to_times(parse_msg_file(msg_file), float_range_helper(start, end, step)):
                print(state)

    else:
        for event in parse_msg_file(sys.stdin):
            print(event)
