/*
 * PararealController.hpp
 *
 *  Created on: 11 Apr 2016
 *      Author: Martin Schreiber <schreiberx@gmail.com>
 */

#ifndef SRC_INCLUDE_PARAREAL_PARAREAL_CONTROLLER_SERIAL_HPP_
#define SRC_INCLUDE_PARAREAL_PARAREAL_CONTROLLER_SERIAL_HPP_


#include <parareal/Parareal_ConsolePrefix.hpp>
#include <parareal/Parareal_SimulationInstance.hpp>
#include <parareal/Parareal_SimulationVariables.hpp>
#include <iostream>
#include <fstream>
#include <string>


/**
 * This class takes over the control and
 * calls methods offered via PararealSimulation.
 *
 * \param t_SimulationInstance	class which implements the Parareal_SimulationInstance interfaces
 */
template <class t_SimulationInstance>
class Parareal_Controller_Serial
{
	/**
	 * Array with instantiations of PararealSimulations
	 */
	t_SimulationInstance *simulationInstances = nullptr;

	/**
	 * Pointers to interfaces of simulationInstances
	 * This helps to clearly separate between the allocation of the simulation classes and the parareal interfaces.
	 */
	Parareal_SimulationInstance **parareal_simulationInstances = nullptr;

	/**
	 * Pointer to parareal simulation variables.
	 * These variables are used as a singleton
	 */
	Parareal_SimulationVariables *pVars;

	/**
	 * Class which helps prefixing console output
	 */
	Parareal_ConsolePrefix CONSOLEPREFIX;

public:
	Parareal_Controller_Serial()
	{
	}


	~Parareal_Controller_Serial()
	{
		cleanup();
	}


	inline
	void CONSOLEPREFIX_start(
			const char *i_prefix
	)
	{
//		if (pVars->verbosity > 0)
			CONSOLEPREFIX.start(i_prefix);
	}


	inline
	void CONSOLEPREFIX_start(int i_prefix)
	{
//		if (pVars->verbosity > 0)
			CONSOLEPREFIX.start(i_prefix);
	}

	inline
	void CONSOLEPREFIX_end()
	{
		if (pVars->verbosity > 0)
			CONSOLEPREFIX.end();
	}


	void cleanup()
	{
		if (simulationInstances != nullptr)
		{
			delete [] simulationInstances;
			delete [] parareal_simulationInstances;
		}
	}


	void setup(
			Parareal_SimulationVariables *i_pararealSimVars
	)
	{

		cleanup();

		pVars = i_pararealSimVars;

		if (!pVars->enabled)
			return;

		if (pVars->coarse_slices <= 0)
		{
			std::cerr << "Invalid number of coarse slices" << std::endl;
			exit(1);
		}

		if (pVars->max_simulation_time <= 0)
		{
			std::cerr << "Invalid simulation time" << std::endl;
			exit(1);
		}

		// allocate raw simulation instances
		simulationInstances = new t_SimulationInstance[pVars->coarse_slices];

		parareal_simulationInstances = new Parareal_SimulationInstance*[pVars->coarse_slices];

		CONSOLEPREFIX.start("[MAIN] ");
		std::cout << "Resetting simulation instances" << std::endl;

		// convert to pararealsimulationInstances to get Parareal interfaces
		for (int k = 0; k < pVars->coarse_slices; k++)
		{
			CONSOLEPREFIX_start(k);
			parareal_simulationInstances[k] = &(Parareal_SimulationInstance&)(simulationInstances[k]);
//			simulationInstances[k].reset();
		}


		CONSOLEPREFIX_start("[MAIN] ");
		std::cout << "Setup time frames" << std::endl;

		/*
		 * SETUP time frame
		 */
		// size of coarse time step

		double time_slice_size = pVars->max_simulation_time / pVars->coarse_slices;

		if (pVars->coarse_timestep_size < 0)
			pVars->coarse_timestep_size = time_slice_size;

		CONSOLEPREFIX_start(0);
		parareal_simulationInstances[0]->sim_set_timeframe(0, time_slice_size);

		for (int k = 1; k < pVars->coarse_slices-1; k++)
		{
			CONSOLEPREFIX_start(k);
			parareal_simulationInstances[k]->sim_set_timeframe(time_slice_size*k, time_slice_size*(k+1));
		}

		CONSOLEPREFIX_start(pVars->coarse_slices-1);
		parareal_simulationInstances[pVars->coarse_slices-1]->sim_set_timeframe(i_pararealSimVars->max_simulation_time-time_slice_size, i_pararealSimVars->max_simulation_time);


		/*
		 * Setup first simulation instance
		 */
		CONSOLEPREFIX_start(0);
		parareal_simulationInstances[0]->sim_setup_initial_data();
		CONSOLEPREFIX_end();

		CONSOLEPREFIX_start("[MAIN] ");
		std::cout << "Finished setup parareal" << std::endl;
		CONSOLEPREFIX_end();
	}

	void run()
	{

#if SWEET_DEBUG
		// Perform a full fine simulation
		CONSOLEPREFIX_start("[MAIN] ");
		std::cout << "Full fine simulation (debugging)" << std::endl;

		CONSOLEPREFIX_start(0);
		parareal_simulationInstances[0]->run_timestep_fine();
		Parareal_Data &fine_exact = parareal_simulationInstances[0]->get_reference_to_data_timestep_fine();
		parareal_simulationInstances[0]->sim_set_data_fine_exact(fine_exact);

		for (int i = 1; i < pVars->coarse_slices; i++)
		{
			CONSOLEPREFIX_start(i-1);
			Parareal_Data &tmp = parareal_simulationInstances[i-1]->get_reference_to_data_timestep_fine();
			Parareal_Data &tmp2 = parareal_simulationInstances[i-1]->get_reference_to_data_timestep_fine_previous_timestep(); // SL

			// use fine time step output data as initial data of next time slice
			CONSOLEPREFIX_start(i);
			parareal_simulationInstances[i]->sim_set_data(tmp);
			parareal_simulationInstances[i]->sim_set_data_fine_previous_time_slice(tmp2); // SL

			// run fine time step
			parareal_simulationInstances[i]->run_timestep_fine();

			//store
			Parareal_Data &fine_exact = parareal_simulationInstances[i]->get_reference_to_data_timestep_fine();
			parareal_simulationInstances[i]->sim_set_data_fine_exact(fine_exact);
		}
#endif

		CONSOLEPREFIX_start("[MAIN] ");
		std::cout << "Initial propagation" << std::endl;

		/**
		 * Initial propagation
		 */
		CONSOLEPREFIX_start(0);
		parareal_simulationInstances[0]->run_timestep_coarse();
		for (int i = 1; i < pVars->coarse_slices; i++)
		{
			CONSOLEPREFIX_start(i-1);
			Parareal_Data &tmp = parareal_simulationInstances[i-1]->get_reference_to_data_timestep_coarse();
			Parareal_Data &tmp2 = parareal_simulationInstances[i-1]->get_reference_to_data_timestep_coarse_previous_timestep(); // SL

			// use coarse time step output data as initial data of next coarse time step
			CONSOLEPREFIX_start(i);
			parareal_simulationInstances[i]->sim_set_data(tmp);
			parareal_simulationInstances[i]->sim_set_data_coarse_previous_time_slice(tmp2); // SL

			// run coarse time step
			parareal_simulationInstances[i]->run_timestep_coarse();
		}

		// Store initial propagation:
		for (int i = 0; i < pVars->coarse_slices; i++)
			parareal_simulationInstances[i]->output_data_file(
					parareal_simulationInstances[i]->get_reference_to_data_timestep_coarse(),
					0,  // 0-th iteration
					i
				);

		/**
		 * We run as much Parareal iterations as there are coarse slices
		 */
//		int start_slice = 0;

		int k = 0;
		for (; k < pVars->coarse_slices; k++)
		{
			CONSOLEPREFIX_start("[MAIN] ");
			std::cout << "Iteration Nr. " << k << std::endl;
			/*
			 * All the following loops should start with 0.
			 * For debugging reasons, we leave it here at 0
			 */

			/**
			 * Fine time stepping
			 */
			for (int i = k; i < pVars->coarse_slices; i++)
			{
				CONSOLEPREFIX_start(i);

				if (i > 0)
				{
				    // SL:
				    Parareal_Data &tmp2 = parareal_simulationInstances[i-1]->get_reference_to_data_timestep_fine_previous_timestep();
				    parareal_simulationInstances[i]->sim_set_data_fine_previous_time_slice(tmp2);
				}

				parareal_simulationInstances[i]->run_timestep_fine();
			}


			/**
			 * Compute difference between coarse and fine solution
			 */
			for (int i = k; i < pVars->coarse_slices; i++)
			{
				CONSOLEPREFIX_start(i);
				parareal_simulationInstances[i]->compute_difference();
			}


			/**
			 * 1) Coarse time stepping
			 * 2) Compute output + convergence check
			 * 3) Forward to next frame
			 */
			double max_convergence = -2;
			for (int i = k; i < pVars->coarse_slices; i++)
			{
				CONSOLEPREFIX_start(i);

				if (i > 0)
				{
				    // SL:
				    Parareal_Data &tmp2 = parareal_simulationInstances[i-1]->get_reference_to_data_timestep_coarse_previous_timestep();
				    parareal_simulationInstances[i]->sim_set_data_coarse_previous_time_slice(tmp2);
				}

				parareal_simulationInstances[i]->run_timestep_coarse();

				// compute convergence
				double convergence = parareal_simulationInstances[i]->compute_output_data(true);
				std::cout << "                        iteration " << k << ", time slice " << i << ", convergence: " << convergence << std::endl;
				if (max_convergence != -1)
					max_convergence = (convergence==-1)?(convergence):(std::max(max_convergence,convergence));

				parareal_simulationInstances[i]->output_data_file(
						parareal_simulationInstances[i]->get_reference_to_output_data(),
						k + 1,
						i
					);

				CONSOLEPREFIX.start(i);
				parareal_simulationInstances[i]->output_data_console(
						parareal_simulationInstances[i]->get_reference_to_output_data(),
						k + 1,
						i
					);

				// last coarse time step slice?
				if (i == pVars->coarse_slices-1)
				{
					// convergence check activated?
					if (pVars->convergence_error_threshold >= 0)
					{
						if (max_convergence >= 0)
						{
							// convergence given?
							if (max_convergence < pVars->convergence_error_threshold)
							{
								CONSOLEPREFIX_start("[MAIN] ");
								std::cout << "Convergence reached at iteration " << k << " with convergence value " << max_convergence << std::endl;
								goto converged;
							}
						}
					}
				}


				// forward to next time slice if it exists
				if (i < pVars->coarse_slices-1)
				{
					CONSOLEPREFIX_start(i);
					Parareal_Data &tmp = parareal_simulationInstances[i]->get_reference_to_output_data();

					CONSOLEPREFIX_start(i+1);
					parareal_simulationInstances[i+1]->sim_set_data(tmp);
				}

				parareal_simulationInstances[i]->check_for_nan_parareal();

#if SWEET_DEBUG
				// fine serial solution should be retrieved
				if (i == k)
					parareal_simulationInstances[i]->compare_to_fine_exact();
#endif

			}
///			start_slice++;
		}

converged:

		CONSOLEPREFIX_end();
	}
};





#endif /* SRC_INCLUDE_PARAREAL_PARAREAL_CONTROLLER_SERIAL_HPP_ */
