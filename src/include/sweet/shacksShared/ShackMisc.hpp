/*
 * ShackMisc.hpp
 *
 *  Created on: Feb 21, 2023
 *      Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_INCLUDE_SWEET_SHACKS_SHACKMISC_HPP_
#define SRC_INCLUDE_SWEET_SHACKS_SHACKMISC_HPP_

#include <string>
#include <iostream>
#include <sweet/ProgramArguments.hpp>
#include <sweet/shacks/ShackInterface.hpp>



/**
 * Miscellaneous variables
 */
class ShackMisc	:
		public sweet::ShackInterface
{
public:
	/// do instability checks for simulation
	int instability_checks = 1;

	/// Diffusion applied only on nonlinear divergence
	int use_nonlinear_only_visc = 0;

	/*
	 * Do a normal mode analysis, see
	 * Hillary Weller, John Thuburn, Collin J. Cotter,
	 * "Computational Modes and Grid Imprinting on Five Quasi-Uniform Spherical C Grids"
	 */
	int normal_mode_analysis_generation = 0;

	/*
	 * Some flexible variable where one can just add options like
	 * --comma-separated-tags=galewsky_analytical_geostrophic_setup
	 */
	std::string comma_separated_tags = "";


	void printProgramArguments(const std::string& i_prefix = "")
	{
		std::cout << "" << std::endl;
		std::cout << "Misc options:" << std::endl;
		std::cout << "	-d [int]			accuracy of floating point output" << std::endl;
		std::cout << "	-i [file0][;file1][;file3]...	string with filenames for initial conditions" << std::endl;
		std::cout << "					specify BINARY; as first file name to read files as binary raw data" << std::endl;
		std::cout << "	--use-local-visc [0/1]	Viscosity will be applied only on nonlinear divergence, default:0" << std::endl;
		std::cout << "	--reuse-plans [0/1]	Save plans for fftw transformations and SH transformations" << std::endl;
		std::cout << "					-1: use only estimated plans (no wisdom)" << std::endl;
		std::cout << "					0: compute optimized plans (no wisdom)" << std::endl;
		std::cout << "					1: compute optimized plans, use wisdom if available and store wisdom" << std::endl;
		std::cout << "					2: use wisdom if available if not, trigger error if wisdom doesn't exist (not yet working for SHTNS)" << std::endl;
		std::cout << "					default: -1 (quick mode)" << std::endl;
		std::cout << "" << std::endl;
	}

	bool processProgramArguments(sweet::ProgramArguments &i_pa)
	{
		i_pa.getArgumentValueByKey("--instability-checks", instability_checks);
		i_pa.getArgumentValueByKey("--use-nonlinear-only-visc", use_nonlinear_only_visc);
		i_pa.getArgumentValueByKey("--normal-mode-analysis-generation", normal_mode_analysis_generation);
		i_pa.getArgumentValueByKey("--comma-separated-tags", comma_separated_tags);

		return error.forwardWithPositiveReturn(i_pa.error);
	}


	virtual void printShack(
		const std::string& i_prefix = ""
	)
	{
		std::cout << std::endl;
		std::cout << "MISC:" << std::endl;
		std::cout << " + instability_checks: " << instability_checks << std::endl;
		std::cout << " + use_nonlinear_only_visc: " << use_nonlinear_only_visc << std::endl;
		std::cout << std::endl;
		std::cout << " + normal_mode_analysis_generation: " << normal_mode_analysis_generation << std::endl;
		std::cout << " + comma_separated_tags: " << comma_separated_tags << std::endl;
		std::cout << std::endl;
	}
};




#endif /* SRC_INCLUDE_SWEET_SHACKS_SHACKMISC_HPP_ */
