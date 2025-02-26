#! /usr/bin/env python3

import numpy as np
import sys
import os
from glob import glob

from mule.postprocessing.JobsData import *
from mule.postprocessing.JobsDataConsolidate import *


file_type = "sweet"

def read_error_file(path):

    lines = [line.rstrip() for line in open(path)];

    if file_type == "csv":
        err_L1 = -1;
        err_L2 = -1;
        err_Linf = -1;

        for line in lines:
            spl = line.split();
            if spl[0] == "errL1":
                err_L1 = float(spl[1]);
                continue;
            if spl[0] == "errL2":
                err_L2 = float(spl[1]);
                continue;
            if spl[0] == "errLinf":
                err_Linf = float(spl[1]);
                continue;

        if err_L1 < 0:
            print("ERROR: err_L1 not found in " + path);
            sys.exit();
        if err_L2 < 0:
            print("ERROR: err_L2 not found in " + path);
        if err_Linf < 0:
            print("ERROR: err_Linf not found in " + path);

        return err_L1, err_L2, err_Linf

    elif file_type == "sweet":
        err_Linf = {};

        for line in lines:
            spl = line.split();
            if spl[0] == "errLinf":
                assert len(spl) == 3, (spl, path);
                rnorm = int(spl[1]);
                err = float(spl[2]);

                err_Linf[rnorm] = err;

        return err_Linf;

path_simulations = sys.argv[1];
tmp_fine_sim = sys.argv[2];
test_spatial_coarsening = False;
if len(sys.argv) > 3:
    test_spatial_coarsening = int(sys.argv[3]);

list_vars = ["runtime.xbraid_cfactor", "runtime.xbraid_max_levels", "runtime.xbraid_pt", "runtime.xbraid_store_iterations", "runtime.xbraid_spatial_coarsening"];
list_vars2 = ["runtime.xbraid_cfactor", "runtime.xbraid_max_levels", "runtime.xbraid_pt", "runtime.xbraid_spatial_coarsening"];
if test_spatial_coarsening:
    list_vars2.remove("runtime.xbraid_spatial_coarsening");

## get list of jobs in this directory
list_jobs = glob.glob(path_simulations + "/job_bench_*");
list_jobs = [os.path.basename(job) for job in list_jobs];
## exclude fine simulation
if tmp_fine_sim in list_jobs:
    list_jobs.remove(tmp_fine_sim);

print("    ** {} jobs found.".format(len(list_jobs)));

jd = JobsData(path_simulations + '/job_bench_*', verbosity=0).get_flattened_data();
## get useful job info
job_info = {};
for key in jd.keys():
    path = os.path.basename(jd[key]["jobgeneration.p_job_dirpath"]);
    if path == tmp_fine_sim:
        continue;
    job_info[path] = {};
    for s in list_vars:
        job_info[path][s] = jd[key][s];


## find identical jobs
small = 1e-7;  ## avoid problems with small values
read_jobs = [];
ipair = 0;
for job1 in list_jobs:
    if job1 in read_jobs:
        continue;
    read_jobs.append(job1);

    found_job = False;
    for job2 in list_jobs:
        if job2 in read_jobs:
            continue;
        found_job = True;
        for s in list_vars2:
            if not job_info[job1][s] == job_info[job2][s]:
                found_job = False;


        if found_job:
            ipair += 1;
            if not test_spatial_coarsening:
                assert not job_info[job1]["runtime.xbraid_store_iterations"] == job_info[job2]["runtime.xbraid_store_iterations"], (job1, job2);
            else:
                assert not job_info[job1]["runtime.xbraid_spatial_coarsening"] == job_info[job2]["runtime.xbraid_spatial_coarsening"], (job1, job2);
                assert job_info[job1]["runtime.xbraid_spatial_coarsening"] * job_info[job2]["runtime.xbraid_spatial_coarsening"] == 0;

            read_jobs.append(job2);

            list_files = glob.glob(path_simulations + "/" + job1 + "/xbraid_error*");
            list_files = [os.path.basename(f) for f in list_files];
            assert len(list_files) > 0;

            max_diff = -1
            print("      -> Pair #{} : comparing {} files".format(ipair, len(list_files)));
            if file_type == "csv":
                not_found_files = 0;
                for f in list_files:
                    if "_spec_" in f:
                        continue;

                    if not os.path.exists(path_simulations + "/" + job2 + "/" + f):
                        not_found_files += 1;
                        continue;
                    err_L1_1, err_L2_1, err_Linf_1 = read_error_file(path_simulations + "/" + job1 + "/" + f);
                    err_L1_2, err_L2_2, err_Linf_2 = read_error_file(path_simulations + "/" + job2 + "/" + f);
                    err_Linf = np.abs(err_Linf_1 - err_Linf_2);
                    err_L1 = np.abs(err_L1_1 - err_L1_2);
                    err_L2 = np.abs(err_L2_1 - err_L2_2);
                    assert err_Linf < small, (err_Linf_1, err_Linf_2, np.abs(err_Linf_1 - err_Linf_2), f, job1, job2);
                    assert err_L1 < small, (err_L1_1, err_L1_2, f, job1, job2);
                    assert err_L2 < small, (err_L2_1, err_L2_2, f, job1, job2);
                    max_diff = np.max([max_diff, err_Linf, err_L1, err_L2]);

            elif file_type == "sweet":
                not_found_files = 0;
                for f in list_files:
                    if not "_spec_" in f:
                        continue;

                    if not os.path.exists(path_simulations + "/" + job2 + "/" + f):
                        not_found_files += 1;
                        continue;

                    err_Linf_1 = read_error_file(path_simulations + "/" + job1 + "/" + f);
                    err_Linf_2 = read_error_file(path_simulations + "/" + job2 + "/" + f);

                    assert len(list(err_Linf_1.keys())) == len(list(err_Linf_2.keys())), (len(list(err_Linf_1.keys())),len(list(err_Linf_2.keys())), err_Linf_1.keys(), err_Linf_2.keys() )
                    for rnorm in err_Linf_1.keys():
                        eps_small = 1e-20;
                        ## ignore too small values
                        if err_Linf_1[rnorm] < eps_small and err_Linf_2[rnorm] < eps_small:
                            continue;
                        err_Linf = np.abs(err_Linf_1[rnorm] - err_Linf_2[rnorm]);
                        assert err_Linf < small, (rnorm, err_Linf_1, err_Linf_2, np.abs(err_Linf_1[rnorm] - err_Linf_2[rnorm]), f, job1, job2, err_Linf, small);
                        max_diff = np.max([max_diff, err_Linf]);


            print("     -> Max diff between errors: " + str(max_diff));
            print("     -> Number of files not found in second job: " + str(not_found_files));
            print("                                             -> OK");
            break;

