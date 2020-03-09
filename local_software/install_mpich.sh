#! /bin/bash

source ./install_helpers.sh ""

PKG_NAME="MPICH"
PKG_INSTALLED_FILE="$SWEET_LOCAL_SOFTWARE_DST_DIR/bin/mpicc"
PKG_URL_SRC="mpich-3.3.2.tar.gz"


export FC=$F90
export FCFLAGS=$F90FLAGS

unset F90
unset F90FLAGS

config_setup

config_package $@

config_configure

config_make_default_install

config_success
