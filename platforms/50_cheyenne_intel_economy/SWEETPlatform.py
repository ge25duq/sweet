import platform
import socket
import sys
import os

from SWEET import *
from . import SWEETPlatformAutodetect

# Underscore defines symbols to be private
_job_id = None

def _whoami(depth=1):
	"""
	String of function name to recycle code

	https://www.oreilly.com/library/view/python-cookbook/0596001673/ch14s08.html

	Returns
	-------
	string
		Return function name
	"""
	return sys._getframe(depth).f_code.co_name



def p_gen_script_info(j : SWEETJobGeneration):
	global _job_id

	return """#
# Generating function: """+_whoami(2)+"""
# Platform: """+get_platform_id()+"""
# Job id: """+_job_id+"""
#
"""



def get_platform_autodetect():
	"""
	Returns
	-------
	bool
		True if current platform matches, otherwise False
	"""

	return SWEETPlatformAutodetect.get_platform_autodetect()



def get_platform_id():
	"""
	Return platform ID

	Returns
	-------
	string
		unique ID of platform
	"""

	return "cheyenne_intel_economy"



def get_platform_resources():
	"""
	Return information about hardware
	"""

	r = SWEETPlatformResources()

	r.num_cores_per_node = 36

	# Physical number of nodes, maybe the limit is different
	r.num_nodes = 4032

	r.num_cores_per_socket = 18

	# 12h limit
	r.max_wallclock_seconds = 60*60*12
	return r



def jobscript_setup(j : SWEETJobGeneration):
	"""
	Setup data to generate job script
	"""

	global _job_id
	_job_id = j.runtime.getUniqueID(j.compile)
	return



def jobscript_get_header(j : SWEETJobGeneration):
	"""
	These headers typically contain the information on e.g. Job exection, number of compute nodes, etc.

	Returns
	-------
	string
		multiline text for scripts
	"""
	global _job_id

	p = j.parallelization

	time_str = p.get_max_wallclock_seconds_hh_mm_ss()
	
	#
	# See https://www.lrz.de/services/compute/linux-cluster/batch_parallel/example_jobs/
	#
	content = """#! /bin/bash
#
## project code
#PBS -A NCIS0002
## economy queue
#PBS -q economy
## wall-clock time (hrs:mins:secs)
#PBS -l walltime="""+time_str+"""
## select: number of nodes
## ncpus: number of CPUs per node
## mpiprocs: number of ranks per node
#PBS -l select="""+str(p.num_nodes)+""":ncpus="""+str(p.num_cores_per_node)+""":mpiprocs="""+str(p.num_ranks_per_node)+""":ompthreads="""+str(p.num_threads_per_rank)+"\n"

	#"default": 2301000 
	#"turbo": 2301000
	#"rated": 2300000
	#"slow": 1200000
	if p.force_turbo_off:
		content += "#PBS -l select=cpufreq=2300000\n"

	ld_library_path = os.getenv('LD_LIBRARY_PATH')

	content += """#
#PBS -N """+_job_id[0:100]+"""
#PBS -o """+j.p_job_stdout_filepath+"""
#PBS -e """+j.p_job_stderr_filepath+"""

#source /etc/profile.d/modules.sh

#module load openmpi
"""+("module load mkl" if j.compile.mkl==True or j.compile.mkl=='enable' else "")+"""


"""+p_gen_script_info(j)+"""

echo
echo "LD_LIBRARY_PATH"
echo "${LD_LIBRARY_PATH}"
echo


# Make sure that SWEET library path is really known
export LD_LIBRARY_PATH=\""""+ld_library_path+""":$LD_LIBRARY_PATH\"


echo
echo "LD_LIBRARY_PATH"
echo "${LD_LIBRARY_PATH}"
echo


echo
echo "hostname"
hostname
echo

echo
echo "lscpu -e"
lscpu -e 
echo

echo
echo "CPU Frequencies (uniquely reduced):"
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq | sort -u
echo

"""

	if j.compile.threading != 'off':
		content += """
export OMP_NUM_THREADS="""+str(p.num_threads_per_rank)+"""
"""

	if p.core_oversubscription:
		raise Exception("Not supported with this script!")

	if p.core_affinity != None:
		
		content += "\necho \"Affnity: "+str(p.core_affinity)+"\"\n"
		if p.core_affinity == 'compact':
			content += "\nexport OMP_PROC_BIND=close\n"
		elif p.core_affinity == 'scatter':
			content += "\nexport OMP_PROC_BIND=spread\n"
		else:
			raise Exception("Affinity '"+str(p.core_affinity)+"' not supported")

	return content






def jobscript_get_exec_prefix(j : SWEETJobGeneration):
	"""
	Prefix before executable

	Returns
	-------
	string
		multiline text for scripts
	"""

	content = ""
	p = j.parallelization

	plan_files = []
	if j.compile.plane_spectral_space == 'enable':
		plan_files.append('sweet_fftw')

	if j.compile.sphere_spectral_space == 'enable':
		plan_files.append('shtns_cfg')
		plan_files.append('shtns_cfg_fftw')

	#
	# Reusing plans assumes them to be stored in the folder one level up in the hierarchy
	#
	if j.runtime.reuse_plans == -1:
		# Quick plan generation mode, nothing to do
		pass

	elif j.runtime.reuse_plans == 0:
		# Create plans, don't load/store them
		pass

	elif j.runtime.reuse_plans == 1:
		content += "\n"
		# Reuse plans if available
		for i in plan_files:
			content += "cp ../"+i+" ./ 2>/dev/null\n"
			
	elif j.runtime.reuse_plans == 2:
		content += "\n"
		# Reuse and trigger error if they are not available
		for i in plan_files:
			content += "cp ../"+i+" ./ || exit 1\n"
	else:
		raise Exception("Invalid reuse_plans value"+str(p.runtime.reuse_plans))

	return content



def jobscript_get_exec_command(j : SWEETJobGeneration):
	"""
	Prefix to executable command

	Returns
	-------
	string
		multiline text for scripts
	"""

	p = j.parallelization

	mpiexec = ""

	#
	# Only use MPI exec if we are allowed to do so
	# We shouldn't use mpiexec for validation scripts
	#
	if not p.mpiexec_disabled:
		# Use mpiexec_mpt for Intel MPI
		#mpiexec = "mpiexec_mpt -n "+str(p.num_ranks)

		# Use mpiexec for GNU
		if j.compile.sweet_mpi == 'enable':
			mpiexec = "mpiexec_mpt -n "+str(p.num_ranks)

			mpiexec += " omplace "
			mpiexec += " -nt "+str(p.num_threads_per_rank)+" "
			mpiexec += " -tm intel"
			mpiexec += " -vv"
			if mpiexec[-1] != ' ':
				mpiexec += ' '


	content = """

EXEC=\"$SWEET_ROOT/build/"""+j.compile.getProgramName()+"""\"
PARAMS=\""""+j.runtime.getRuntimeOptions()+"""\"

echo
echo "ldd"
ldd $EXEC
echo

E=\""""+mpiexec+"""${EXEC} ${PARAMS}\"

echo
echo "Executing..."
echo "$E"
$E

"""

	return content



def jobscript_get_exec_suffix(j : SWEETJobGeneration):
	"""
	Suffix before executable

	Returns
	-------
	string
		multiline text for scripts
	"""

	content = """
echo
echo "CPU Frequencies (uniquely reduced):"
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq | sort -u
echo

"""

	return content



def jobscript_get_footer(j : SWEETJobGeneration):
	"""
	Footer at very end of job script

	Returns
	-------
	string
		multiline text for scripts
	"""
	content = ""

	return content



def jobscript_get_compile_command(j : SWEETJobGeneration):
	"""
	Compile command(s)

	This is separated here to put it either
	* into the job script (handy for workstations)
	or
	* into a separate compile file (handy for clusters)

	Returns
	-------
	string
		multiline text with compile command to generate executable
	"""

	content = """

SCONS="scons """+j.compile.getSConsParams()+' -j 4"'+"""
echo "$SCONS"
$SCONS || exit 1
"""

	return content

