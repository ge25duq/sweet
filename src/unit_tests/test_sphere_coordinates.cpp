/*
 * test_sphere_coordinates.cpp
 *
 *  Created on: 17 Apr 2018
 *      Author: Martin Schreiber <schreiberx@gmail.com>
 */

#include <iostream>
#include <sweet/ScalarDataArray.hpp>
#include <sweet/FatalError.hpp>
#include <sweet/sphere/SphereTimestepping_SemiLagrangian.hpp>




int main(int i_argc, char *i_argv[])
{
	//double N = 16;
	double N = 256;
	double dlon = 1.0/(N+1);
	double dlat = 1.0/(N+1);
	double eps = 1e-10;

	ScalarDataArray a_lon(1);
	ScalarDataArray a_lat(1);
	ScalarDataArray a_x(1);
	ScalarDataArray a_y(1);
	ScalarDataArray a_z(1);

	for (double lon = dlon*0.5; lon < 2.0*M_PI; lon += dlon)
	{
		std::cout << "Testing for longitude " << lon << std::endl;
		a_lon.scalar_data[0] = lon;

		for (double lat = -M_PI*0.5+dlat*0.5; lat <= M_PI*0.5; lat += dlat)
//		double lat = 0;
		{
			a_lat.scalar_data[0] = lat;

			SphereTimestepping_SemiLagrangian::math_angleToCartCoord(a_lon, a_lat, a_x, a_y, a_z);

			ScalarDataArray o_lon(1);
			ScalarDataArray o_lat(1);
			SphereTimestepping_SemiLagrangian::math_cartToAngleCoord(a_x, a_y, a_z, o_lon, o_lat);

			double error = std::max(std::abs(o_lon.scalar_data[0]-lon), std::abs(o_lat.scalar_data[0]-lat));

			if (error > eps)
			{
				std::cout << std::endl;
				std::cout << "input: " << lon << "\t" << lat << std::endl;
				std::cout << "cart: " << a_x.scalar_data[0] << "\t" << a_y.scalar_data[1] << "\t" << a_z.scalar_data[2] << std::endl;
				std::cout << "output: " << o_lon.scalar_data[0] << "\t" << o_lat.scalar_data[0] << std::endl;
				FatalError("Error too large!");
			}
		}
	}

	std::cout << "Tests passed" << std::endl;

	return 0;
}
