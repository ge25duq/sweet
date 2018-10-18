#
# Tag in header of job subscription files to express dependency to another job
# This is highly important for the plan generation of spectral transformations
#
export SWEET_JOB_SCHEDULER_DEPENDENCY="-W depend=afterany:%JOBID%"


MODULES="intel/18.0.1"
for m in $MODULES; do
	echo
	echo "Loading $m"
	module load $m
done


#
# Compiler environment
#


#
# DO NOT USE icc, icpc or ifort directly on Cheyenne!
# These are python scripts which mess around with the libraries!!!
#
# Use e.g.
# icpc --show
# to see what's really executed for the compiler
#
#export SWEET_CC=icc
#export SWEET_CXX=icpc
#export SWEET_F90=ifort

export SWEET_CC=/glade/u/apps/opt/intel/2018u1/compilers_and_libraries/linux/bin/intel64/icc
export SWEET_CXX=/glade/u/apps/opt/intel/2018u1/compilers_and_libraries/linux/bin/intel64/icpc
export SWEET_F90=/glade/u/apps/opt/intel/2018u1/compilers_and_libraries/linux/bin/intel64/ifort

export SWEET_MPICC=/opt/sgi/mpt/mpt-2.15/bin/mpicc
export SWEET_MPICXX=/opt/sgi/mpt/mpt-2.15/bin/mpicxx
export SWEET_MPIF90=/opt/sgi/mpt/mpt-2.15/bin/mpif90

export SWEET_LINK=$SWEET_CXX
export SWEET_MPILINK=$SWEET_MPICXX

#
# local software compile overrides
#
export F90=$SWEET_F90
export FC=$F90
export CC=$SWEET_CC
export CXX=$SWEET_CXX
export LINK=$SWEET_CXX
export LD=ld

