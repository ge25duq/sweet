#! /bin/bash

#if false; then
if true; then
	./pp_plot_csv.py output_prog_vort_t00000000*csv
	./pp_create_mp4.sh output_prog_vort_ output_prog_vort.mp4
fi


if false; then
#if true; then
	./pp_plot_csv.py output_prog_h_t00000000*csv
	./pp_create_mp4.sh output_prog_h_ output_prog_h.mp4
fi
