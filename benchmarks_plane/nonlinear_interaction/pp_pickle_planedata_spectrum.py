#! /usr/bin/env python3

from mule_local.postprocessing.pickle_PlaneData_Spectrum import *
from mule.exec_program import *

# process only last file (likely the reference solution)
#pickle_PlaneData_Spectrum(params=["only_last_file", "ignore_missing_file"])

# process all files
pickle_PlaneData_Spectrum(params=[])
