#! /usr/bin/env python3
# This file describes all possible compile options
#
# The underlying concept is to separate compile options
# and the unique Id generation into this to make it reusable
# by other scripts.
#
# This also leads to a clean separation between compile and runtime options.
#

import sys
import os
import glob
from importlib import import_module
from mule.InfoError import *
import mule.utils
import mule.Shacks

import subprocess


__all__ = ['JobCompileOptions']


def _exec_command(command):
    process = subprocess.Popen(command.split(' '), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    # combine stdout and stderr
    out = out+err
    out = out.decode("utf-8")
    out = out.replace("\r", "")
    if len(out) > 0:
        if out[-1] in ["\n", "\r"]:
            out = out[:-1]
    return out


class JobCompileOptions(InfoError):


    def __init__(self, dummy_init = False):
        self.init_phase = True

        InfoError.__init__(self, "JobCompileOptions")

        self.shacksCompile = mule.Shacks.getShacksDict("shacksCompile").values()

        for _ in self.shacksCompile:
            _.__init__(self)
        
        # Program or unit test
        self.program = ""
        self.program_name = ""

        # Compile options
        self.mode = 'release'

        self.debug_symbols = 'disable'
        self.simd = 'enable'

        self.fortran_source = 'disable'
        self.lapack = 'disable'

        self.program_binary_name = ''

        # Parallelization
        self.sweet_mpi = 'disable'
        self.threading = 'omp'
        self.rexi_thread_parallel_sum = 'disable'

        self.benchmark_timings = 'disable'

        # Additional barriers to overcome issues of turbo boost
        self.rexi_timings_additional_barriers = 'disable'

        # Use reduce all instead of reduce to root rank
        self.rexi_allreduce = 'disable'


        # Program / Unit test
        self.program = ''


        #LibPFASST
        self.libpfasst = 'disable'

        # Eigen library
        self.eigen = 'disable'

        # Libraries
        self.libfft = 'disable'
        self.libsph = 'disable'
        self.mkl = 'disable'

        # Features
        self.plane_spectral_space = 'disable'
        self.plane_spectral_dealiasing = 'enable'
        self.sphere_spectral_space = 'disable'
        self.sphere_spectral_dealiasing = 'enable'
        self.libxml = 'disable'

        # GUI
        self.gui = 'disable'

        # Which parallel sdc parallelizatino model to use
        # Only 'omp' supported so far
        self.parallel_sdc_par_model = None

        self.quadmath = 'disable'

        self.init_phase = False



    def __setattr__(self, name, value):
        if name != 'init_phase':
            if not self.init_phase:
                if not name in self.__dict__:
                    raise Exception("Attribute '"+name+"' does not exist!")

        self.__dict__[name] = value



    def getSConsParams(self):
        retval = ''

        # Program / Unit test
        if self.program != '':
            retval += ' --program='+self.program

        retval += ' --mode='+self.mode
        retval += ' --debug-symbols='+("enable" if self.debug_symbols else "disable")
        retval += ' --simd='+self.simd

        retval += ' --fortran-source='+self.fortran_source

        retval += ' --lapack='+self.lapack

        retval += ' --program-binary-name='+self.program_binary_name

        # Parallelization
        retval += ' --sweet-mpi='+self.sweet_mpi
        retval += ' --threading='+self.threading
        retval += ' --rexi-thread-parallel-sum='+self.rexi_thread_parallel_sum
        retval += ' --benchmark-timings='+self.benchmark_timings
        retval += ' --rexi-timings-additional-barriers='+self.rexi_timings_additional_barriers
        retval += ' --rexi-allreduce='+self.rexi_allreduce



        for _ in self.shacksCompile:
            retval += _.getSConsParams(self)


        # LibPFASST
        retval += ' --libpfasst='+self.libpfasst

        retval += ' --eigen='+self.eigen

        # Libraries
        retval += ' --libfft='+self.libfft
        retval += ' --libsph='+self.libsph
        retval += ' --mkl='+self.mkl

        # Features
        retval += ' --plane-spectral-space='+self.plane_spectral_space
        retval += ' --plane-spectral-dealiasing='+self.plane_spectral_dealiasing
        retval += ' --sphere-spectral-space='+self.sphere_spectral_space
        retval += ' --sphere-spectral-dealiasing='+self.sphere_spectral_dealiasing
        retval += ' --libxml='+self.libxml

        # GUI
        retval += ' --gui='+self.gui
        
        # Parallel SDC
        if self.parallel_sdc_par_model is not None:
            retval += ' --parallel-sdc-par-model='+self.parallel_sdc_par_model
        

        # Activate quadmath
        retval += ' --quadmath='+self.quadmath

        return retval



    def postprocessOptions(self):

        """
        Handle program specific options

        Process scons options provided within source file, e.g.,
         * MULE_COMPILE_FILES_AND_DIRS: src/programs/swe_sphere/
         * MULE_COMPILE_FILES_AND_DIRS: src/include/benchmarks_sphere_swe/
         * MULE_SCONS_OPTIONS: --sphere-spectral-space=enable
        """
        pso_ = self.get_program_specific_options()
        if pso_ is None:
            raise Exception("Error with program specific options. Did you specify a program with --program=... ?")

        self.process_scons_options(pso_['scons_options'])


        if self.libpfasst == 'enable':
            self.fortran_source = 'enable'

        if self.plane_spectral_space == 'enable':
            self.libfft = 'enable'
        else:
            self.plane_spectral_dealiasing = 'disable'

        if self.sphere_spectral_space == 'enable':
            self.libsph = 'enable'
        else:
            self.sphere_spectral_dealiasing = 'disable'

        if self.libsph == 'enable':
            # activate linking with libfft!
            self.libfft = 'enable'

        return


    def sconsProcessCommandlineOptions(self):
        scons = import_module("SCons.Script")
        
        self.sconsAddOptions(scons)
        self.sconsValidateOptions()


    def sconsAddOptions(self, scons):

        scons.AddOption(      '--mode',
                dest='mode',
                type='choice',
                choices=['debug', 'release'],
                default='release',
                help='specify compilation mode to use: debug, release [default: %default]'
        )
        self.mode = scons.GetOption('mode')

        scons.AddOption(    '--simd',
                dest='simd',
                type='choice',
                choices=['enable', 'disable'],
                default='enable',
                help="Use SIMD for operations such as folding [default: %default]"
        )
        self.simd = scons.GetOption('simd')


        scons.AddOption(    '--eigen',
                dest='eigen',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Activate utilization of Eigen library [default: %default]"
        )
        self.eigen = scons.GetOption('eigen')


        scons.AddOption(    '--libpfasst',
                dest='libpfasst',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Activate utilization of libPFASST (FOortran version) [default: %default]"
        )
        self.libpfasst = scons.GetOption('libpfasst')



        scons.AddOption(    '--debug-symbols',
                dest='debug_symbols',
                type='choice',
                choices=['enable', 'disable'],
                default='enable',
                help="Create binary with debug symbols [default: %default]"
        )
        self.debug_symbols = scons.GetOption('debug_symbols')



        scons.AddOption(    '--plane-spectral-space',
                dest='plane_spectral_space',
                type='choice',
                choices=['enable', 'disable'],
                default='enable',
                help="Activate spectral space for data on the plane (2D FFT) [default: %default]"
        )
        self.plane_spectral_space = scons.GetOption('plane_spectral_space')

        scons.AddOption(    '--sphere-spectral-space',
                dest='sphere_spectral_space',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Activate spectral space for data on the sphere (Spherical Harmonics) [default: %default]"
        )
        self.sphere_spectral_space = scons.GetOption('sphere_spectral_space')


        scons.AddOption(    '--fortran-source',
                dest='fortran_source',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Activate linking with Fortran source [default: %default]"
        )
        self.fortran_source = scons.GetOption('fortran_source')


        scons.AddOption(    '--lapack',
                dest='lapack',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Enable lapack [default: %default]"
        )
        self.lapack = scons.GetOption('lapack')



        scons.AddOption(    '--libfft',
                dest='libfft',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Enable compiling and linking with FFT library [default: %default]"
        )
        self.libfft = scons.GetOption('libfft')


        scons.AddOption(    '--libsph',
                dest='libsph',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Enable compiling and linking with SPH library [default: %default]"
        )
        self.libsph = scons.GetOption('libsph')


        scons.AddOption(    '--mkl',
                dest='mkl',
                type='choice',
                choices=['enable', 'disable'],
                default='disable',
                help="Enable Intel MKL [default: %default]"
        )
        self.mkl = scons.GetOption('mkl')


        #
        # LIB XML
        #
        scons.AddOption(    '--libxml',
                dest='libxml',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='Compile with libXML related code: enable, disable [default: %default]'
        )
        self.libxml = scons.GetOption('libxml')


        scons.AddOption(    '--plane-spectral-dealiasing',
                dest='plane_spectral_dealiasing',
                type='choice',
                choices=['enable','disable'],
                default='enable',
                help='spectral dealiasing (3N/2-1 rule): enable, disable [default: %default]'
        )
        self.plane_spectral_dealiasing = scons.GetOption('plane_spectral_dealiasing')

        scons.AddOption(    '--sphere-spectral-dealiasing',
                dest='sphere_spectral_dealiasing',
                type='choice',
                choices=['enable','disable'],
                default='enable',
                help='spectral dealiasing (3N/2-1 rule): enable, disable [default: %default]'
        )
        self.sphere_spectral_dealiasing = scons.GetOption('sphere_spectral_dealiasing')

        # Always use dealiasing for sphere
        if self.sphere_spectral_space == 'enable' and self.sphere_spectral_dealiasing != 'enable':
            raise Exception("self.sphere_spectral_dealiasing != enable")

        scons.AddOption(    '--quadmath',
                dest='quadmath',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='quadmath: enable, disable [default: %default]'
        )
        self.quadmath = scons.GetOption('quadmath')



        scons.AddOption(    '--gui',
                dest='gui',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='gui: enable, disable [default: %default]'
        )
        self.gui = scons.GetOption('gui')



        scons.AddOption(    '--parallel-sdc-par-model',
                dest='parallel_sdc_par_model',
                type='choice',
                choices=['off','omp'],
                default='off',
                help='Which parallelization model to use for parallel SDC [default: %default]'
        )
        self.parallel_sdc_par_model = scons.GetOption('parallel_sdc_par_model')




        scons.AddOption(    '--rexi-thread-parallel-sum',
                dest='rexi_thread_parallel_sum',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='Use a par for loop over the sum in REXI: (enable, disable) [default: %default]\n\tWARNING: This also disables the parallelization-in-space with OpenMP'
        )
        self.rexi_thread_parallel_sum = scons.GetOption('rexi_thread_parallel_sum')


        scons.AddOption(    '--benchmark-timings',
                dest='benchmark_timings',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='REXI timings: enable, disable [default: %default]'
        )
        self.benchmark_timings = scons.GetOption('benchmark_timings')

        scons.AddOption(    '--rexi-timings-additional-barriers',
                dest='rexi_timings_additional_barriers',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='REXI timings with additional barriers: enable, disable [default: %default]\nThis is helpful for improved measurements if TurboBoost is activated'
        )
        self.rexi_timings_additional_barriers = scons.GetOption('rexi_timings_additional_barriers')

        scons.AddOption(    '--rexi-allreduce',
                dest='rexi_allreduce',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='For REXI, use MPI_Allreduce operations instead of MPI_Reduce: enable, disable [default: %default]'
        )
        self.rexi_allreduce = scons.GetOption('rexi_allreduce')


        scons.AddOption(    '--sweet-mpi',
                dest='sweet_mpi',
                type='choice',
                choices=['enable','disable'],
                default='disable',
                help='Enable MPI commands e.g. for REXI sum: enable, disable [default: %default]'
        )
        self.sweet_mpi = scons.GetOption('sweet_mpi')


        for _ in self.shacksCompile:
            _.sconsAddOptions(self, scons)



        program_files = glob.glob('src/programs/*.cpp')
        tutorial_files = glob.glob('src/tutorials/*.cpp')
        tests_files = glob.glob('src/tests/*.cpp')

        example_programs = program_files + tutorial_files + tests_files
        example_programs = [mule.utils.remove_file_ending(_)[4:] for _ in example_programs]

        scons.AddOption(      '--program',
                dest='program',
                type='string',
                default="",
                help='Specify program to compile: '+', '.join(example_programs)+' '*80+' [default: %default]'
        )
        self.program = scons.GetOption('program')


        # Be a little bit more tolerant to users

        # Remove "src/" at the beginning
        if self.program.startswith("src/"):
            self.program = self.program[4:]
        if self.program.startswith("./src/"):
            self.program = self.program[6:]

        # Remove .cpp at the end
        if self.program.endswith(".cpp"):
            self.program = self.program[:-4]

        if self.program not in example_programs:
            #
            # If it doesn't exist, maybe it's in a subfolder, e.g.
            # programs/libpfasst/foo.cpp
            #
            prog = self.program[:]
            if not prog.endswith(".cpp"):
                prog = prog+".cpp"

            if not prog.startswith("src/") and not prog.startswith("./src/"):
                prog = "src/"+prog


            if not os.path.exists(prog):
                print(f"Program {self.program} not found")
                print("")
                print(f"The following options are possible:")
                for i in example_programs:
                    print(f" + {i}")
                print("")
                sys.exit(1)

        scons.AddOption(      '--program-binary-name',
                dest='program_binary_name',
                type='string',
                action='store',
                help='Name of program binary, default: [auto]',
                default=''
        )
        self.program_binary_name = scons.GetOption('program_binary_name')

        threading_constraints = ['off', 'omp']
        scons.AddOption(    '--threading',
                dest='threading',
                type='choice',
                choices=threading_constraints,
                default='omp',
                help='Threading to use '+' / '.join(threading_constraints)+', default: off'
        )
        self.threading = scons.GetOption('threading')


    def sconsValidateOptions(self):

        for _ in self.shacksCompile:
            _.sconsValidateOptions(self)


    def sconsAddFlags(self, env):
        for _ in self.shacksCompile:
            _.sconsAddFlags(self, env)



    def getProgramExec(self, ignore_errors = False):
        self.postprocessOptions()

        if self.program != '':
            self.program_name = self.program

        else:
            self.program_name = 'DUMMY'
            self.info("")
            self.info("")
            self.info("Neither a program name, nor a unit test is given:")
            self.info("  use --program=[program name] to specify the program")
            self.info("  or --unit-test=[unit test] to specify a unit test")
            self.info("")
            if not ignore_errors:
                sys.exit(1)

        retval = self.program_name
        s = self.getUniqueID([])

        if s != '':
            retval += '_'+s

        return retval


    #
    # Return a dictionary with further compile options specified in the
    # headers of the main program file
    #
    def get_program_specific_options(self):

        mainsrcadddir = ''

        if self.program != '':
            mainsrcadddir = 'src/'+self.program
        else:
            return None


        #
        # Add main source file
        #
        main_src = mainsrcadddir+'.cpp'


        fad_dict = {
            'compile_files_and_dirs': [],
            'scons_options': [],
        }

        #
        # Check for additional source files and directories which should be added
        # This can be specified in the main program/test source file via e.g.
        #
        # MULE_COMPILE_DIRS: [file1] [dir2] [file2] [file3]
        #
        # The files and directories need to be specified to the software's root directory
        #

        sw_root = os.environ['MULE_SOFTWARE_ROOT']+'/'
        try:
            f = open(sw_root+'/'+main_src, 'r')
        except IOError:
            # Ignore 
            return fad_dict


        lines = f.readlines()

        tags_ = [
                    ['compile_files_and_dirs', 'MULE_COMPILE_FILES_AND_DIRS: '],
                    ['scons_options', 'MULE_SCONS_OPTIONS: '],
                ]

        # Add main program file to compile files
        fad_dict['compile_files_and_dirs'] += [main_src]

        for l in lines:
            for tags in tags_:
                tag_id = tags[0]
                tag = tags[1]
                if tag in l:
                    fad = l[l.find(tag)+len(tag):]
                    fad = fad.replace('\r', '')
                    fad = fad.replace('\n', '')

                    fad_dict[tag_id] += fad.split(' ')

        return fad_dict
        


    def process_scons_options(self, options_):
        for option in options_:
            name, value = option.split('=', 1)

            if name[0:2] != '--':
                raise Exception("Option doesn't start with '--': "+name)

            name = name[2:]

            varname = name.replace('-', '_')
            if not varname in self.__dict__:
                raise Exception(f"Can't use option '{name}'")

            self.__dict__[varname] = value


    def getProgramPath(self, ignore_errors = False):
        return os.environ['MULE_SOFTWARE_ROOT']+'/build/'+self.getProgramExec(ignore_errors)



    def getUniqueID(self, i_filter_list = []):
        """
        Return a unique ID representing the compile parameters 
        """
        self.postprocessOptions()

        retval = ''

        if not 'compile_misc' in i_filter_list:

            if not 'compile_plane' in i_filter_list:
                if self.plane_spectral_space == 'enable':
                    retval+='_plspec'

                    if self.plane_spectral_dealiasing == 'enable':
                        retval+='_pldeal'

            if not 'compile_sphere' in i_filter_list:
                if self.sphere_spectral_space == 'enable':
                    retval+='_spspec'

                    if self.sphere_spectral_dealiasing == 'enable':
                        retval+='_spdeal'

            if self.gui == 'enable':
                retval+='_gui'
                
            if self.parallel_sdc_par_model == 'omp':
                retval+='_psdcpmomp'

            if self.quadmath == 'enable':
                retval+='_quadmath'

            if self.libfft == 'enable':
                retval+='_fft'


        if self.benchmark_timings == 'enable':
            retval+='_benchtime'

        if not 'compile.parallelization' in i_filter_list:
            if self.sweet_mpi == 'enable':
                retval+='_mpi'

            if self.threading in ['omp']:
                retval+='_th'+self.threading
                
            if self.rexi_thread_parallel_sum == 'enable':
                retval+='_rxthpar'

            if self.rexi_timings_additional_barriers == 'enable':
                retval+='_rxtbar'

            if self.rexi_allreduce == 'enable':
                retval+='_redall'

        retval += '_'+self.mode

        if retval != '':
            retval = 'COMP'+retval

        return retval




    def getOptionList(self):
        return self.__dict__




if __name__ == "__main__":

    p = JobCompileOptions()

    s = p.getSConsParams()
    p.info(s)

    s = p.getProgramExec(True)
    p.info(s)

    p.info("FIN")
