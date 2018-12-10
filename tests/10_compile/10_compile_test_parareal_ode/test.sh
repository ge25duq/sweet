#! /bin/bash

cd "$MULE_SOFTWARE_ROOT"

echo
echo "PARAREAL ODE"
SCONS="scons --program=parareal_ode --parareal=serial --gui=disable --plane-spectral-space=disable --mode=debug "
echo "$SCONS"
$SCONS || exit

