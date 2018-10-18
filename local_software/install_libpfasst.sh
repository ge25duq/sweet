#! /bin/bash

source ./install_helpers.sh ""

PKG_NAME="libpfasst"
PKG_INSTALLED_FILE="$SWEET_LOCAL_SOFTWARE_DST_DIR/lib/libpfasst.a"
PKG_URL_SRC="libpfasst_sweet_2018_09_27.tar.bz2"

config_setup

config_package $@

if [ -z "$SWEET_MPIF90" ]; then
	echo_error_exit "SWEET_MPIF90 environment variable must be set for libpfasst compilation, see platform configuration"
fi

sed -i "s/ftn/${SWEET_MPIF90}/" Makefile.defaults || echo_error_exit "Replacing compiler failed"

echo_info "Executing 'make clean'..."
config_exec make clean

config_exec make FC=${SWEET_MPIF90}

echo_info "Installing..."

# Copy modules
mkdir -p "$SWEET_LOCAL_SOFTWARE_DST_DIR/include/"
echo_info cp -v -f ./include/*mod "$SWEET_LOCAL_SOFTWARE_DST_DIR/include/"
cp -v -f ./include/*mod "$SWEET_LOCAL_SOFTWARE_DST_DIR/include/" || echo_error_exit "Failed to install .mod files"

# Copy static library
mkdir -p "$SWEET_LOCAL_SOFTWARE_DST_DIR/lib/"
echo_info cp -v -f "./lib/libpfasst.a" "$SWEET_LOCAL_SOFTWARE_DST_DIR/lib/"
cp -v -f "./lib/libpfasst.a" "$SWEET_LOCAL_SOFTWARE_DST_DIR/lib/" || echo_error_exit "Failed to install libpfasst.a files"


config_success
