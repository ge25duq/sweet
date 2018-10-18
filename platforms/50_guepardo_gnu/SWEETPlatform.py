import platform
import socket
import sys

from SWEET import *

import multiprocessing

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



def p_gen_script_info(jobgeneration : SWEETJobGeneration):
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

	return "guepardo_gnu"


def get_platform_resources():
	"""
	Return information about hardware
	"""

	h = SWEETPlatformResources()

	h.num_cores_per_node = multiprocessing.cpu_count()
	h.num_nodes = 1

	# TODO: So far, we only assume a single socket system as a fallback
	h.num_cores_per_socket = h.num_cores_per_node

	return h



def jobscript_setup(jobgeneration : SWEETJobGeneration):
	"""
	Setup data to generate job script
	"""

	global _job_id
	_job_id = jobgeneration.runtime.getUniqueID(jobgeneration.compile)
	return



def jobscript_get_header(jobgeneration : SWEETJobGeneration):
	"""
	These headers typically contain the information on e.g. Job exection, number of compute nodes, etc.

	Returns
	-------
	string
		multiline text for scripts
	"""
	content = """#! /bin/bash

"""+p_gen_script_info(jobgeneration)+"""

"""

	return content




def jobscript_get_exec_prefix(jobgeneration : SWEETJobGeneration):
	"""
	Prefix before executable

	Returns
	-------
	string
		multiline text for scripts
	"""

	j = jobgeneration
	p = j.parallelization

	content = ""

	if j.compile.threading != 'off':
		content += """
export OMP_NUM_THREADS="""+str(p.num_threads_per_rank)+"""
export OMP_DISPLAY_ENV=VERBOSE
"""

	if p.core_oversubscription:
		raise Exception("Not supported with this script!")
	else:
		if p.core_affinity != None:
			content += "\necho \"Affnity: "+str(p.core_affinity)+"\"\n"
			if p.core_affinity == 'compact':
				content += "source $SWEET_ROOT/platforms/bin/setup_omp_places.sh nooversubscription close\n"
				#content += "\nexport OMP_PROC_BIND=close\n"
			elif p.core_affinity == 'scatter':
				raise Exception("Affinity '"+str(p.core_affinity)+"' not supported")
				content += "\nexport OMP_PROC_BIND=spread\n"
			else:
				raise Exception("Affinity '"+str(p.core_affinity)+"' not supported")

			content += "\n"

	return content



def jobscript_get_exec_command(jobgeneration : SWEETJobGeneration):
	"""
	Prefix to executable command

	Returns
	-------
	string
		multiline text for scripts
	"""

	p = jobgeneration.parallelization

	content = """

"""+p_gen_script_info(jobgeneration)+"""

# mpiexec ... would be here without a line break
EXEC=\"$SWEET_ROOT/build/"""+jobgeneration.compile.getProgramName()+"""\"
PARAMS=\""""+jobgeneration.runtime.getRuntimeOptions()+"""\"
echo \"${EXEC} ${PARAMS}\"

"""
	if jobgeneration.compile.sweet_mpi == 'enable':
		content += 'mpiexec -n '+str(p.num_ranks)+' '

	content += "$EXEC $PARAMS || exit 1"
	content += "\n"
	content += "\n"

	return content



def jobscript_get_exec_suffix(jobgeneration : SWEETJobGeneration):
	"""
	Suffix before executable

	Returns
	-------
	string
		multiline text for scripts
	"""

	content = """

"""+p_gen_script_info(jobgeneration)+"""

"""

	return content



def jobscript_get_footer(jobgeneration : SWEETJobGeneration):
	"""
	Footer at very end of job script

	Returns
	-------
	string
		multiline text for scripts
	"""
	content = """

"""+p_gen_script_info(jobgeneration)+"""

"""

	return content



def jobscript_get_compile_command(jobgeneration : SWEETJobGeneration):
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

SCONS="scons """+jobgeneration.compile.getSConsParams()+' -j 4"'+"""
echo "$SCONS"
$SCONS || exit 1
"""

	return content

