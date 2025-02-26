#! /usr/bin/env python3

import sys
import math
import copy

from mule.JobMule import *
from mule.plotting.Plotting import *
from mule.postprocessing.JobsData import *
from mule.postprocessing.JobsDataConsolidate import *
import sys


groups = [
        'runtime.timestepping_method',
    ]

tagname_x = 'runtime.timestep_size'
tagname_y = 'output.errors'

j = JobsData('./job_bench_*', verbosity=0)

c = JobsDataConsolidate(j)


print("")
print("Groups:")
job_groups = c.create_groups(groups)
for key, g in job_groups.items():
    print(key)


errors = []
for key, jobs_data in job_groups.items():
    for job_key, job_data in jobs_data.items():

        # Get all output.errors.* lines
        error_line_keys = []
        for i in job_data:
            tag = 'output.errors.'
            if i.startswith(tag):
                error_line_keys.append(i)

        # Sort and get last one
        error_line_keys.sort()
        last_error_line_key = error_line_keys[-1]

        if last_error_line_key != tag+"00000010":
            print("Error")
            print("Printing full job data")
            print(job_data)

            ntag = tag+"00000010"
            print(f"Error with last key: '{last_error_line_key}' compared to value '{ntag}'")

            try:
                # opening and reading the file
                print("")
                print("output.out")
                file_read = open(job_data["jobgeneration.job_dirpath"]+"/output.out", "r")
                print(file_read.read())
                print("")

                print("")
                print("output.err")
                file_read = open(job_data["jobgeneration.job_dirpath"]+"/output.err", "r")
                print(file_read.read())
                print("")

            except:
                print("An exception occurred")

            assert last_error_line_key == tag+"00000010"

        error_split = job_data[last_error_line_key].split("\t")
        if len(error_split) != 4:
            print(f"Trying to split value with mule key '{last_error_line_key}' by 4 with tab separation, but it's not possible")
            raise Exception("Inconsistent number of elements in error output")

        # Avoid including unstable results
        simtime = error_split[0]
        if abs(float(simtime.replace('simtime=', '')) - job_data['runtime.max_simulation_time']) > 1e-10:
            continue

        error_linf_phi = float(error_split[1].replace('error_linf_phi=', ''))

        g = 9.80616
        gh = 29400
        rel_error_h = error_linf_phi / gh

        print(job_data['jobgeneration.job_dirpath']+": rel_error_h = "+str(rel_error_h))
        errors.append(rel_error_h)


import math
import numpy

max_error = numpy.amax(numpy.abs(errors))
print("Maximum overall error: "+str(max_error))

err_threshold = 1e-9
if max_error > err_threshold or not math.isfinite(max_error):
    raise Exception("Relative overall error exceeeds threshold "+str(err_threshold))

print("Tests successful")
