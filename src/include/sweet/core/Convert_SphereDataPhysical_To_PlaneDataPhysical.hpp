/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_INCLUDE_SWEET_SPHERE_CONVERT_SPHEREDATA_PHYSICAL_TO_PLANEDATAPHYSICAL_HPP_
#define SRC_INCLUDE_SWEET_SPHERE_CONVERT_SPHEREDATA_PHYSICAL_TO_PLANEDATAPHYSICAL_HPP_

#include <sweet/core/plane/PlaneData_Physical.hpp>
#include <sweet/core/sphere/SphereData_Physical.hpp>



namespace sweet
{

class Convert_SphereDataPhysical_To_PlaneDataPhysical
{
public:
	static
	PlaneData_Physical physical_convert(
			const SphereData_Physical &i_sphereData,
			PlaneData_Config &i_planeDataConfig
	)
	{
		return physical_convert(i_sphereData, &i_planeDataConfig);
	}

public:
	static
	PlaneData_Physical physical_convert(
			const SphereData_Physical &i_sphereData,
			PlaneData_Config *i_planeDataConfig
	)
	{
		assert(i_sphereData.sphereDataConfig->physical_num_lon == (int)i_planeDataConfig->physical_res[0]);
		assert(i_sphereData.sphereDataConfig->physical_num_lat == (int)i_planeDataConfig->physical_res[1]);
		assert(i_planeDataConfig->physical_array_data_number_of_elements == i_sphereData.sphereDataConfig->physical_array_data_number_of_elements);

		PlaneData_Physical out(i_planeDataConfig);


#if SPHERE_DATA_GRID_LAYOUT	== SPHERE_DATA_LAT_CONTINUOUS

		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int i = 0; i < i_sphereData.sphereDataConfig->physical_num_lon; i++)
			for (int j = 0; j < i_sphereData.sphereDataConfig->physical_num_lat; j++)
				out.physical_space_data[(i_sphereData.sphereDataConfig->physical_num_lat-1-j)*i_sphereData.sphereDataConfig->physical_num_lon + i] = i_sphereData.physical_space_data[i*i_sphereData.sphereDataConfig->physical_num_lat + j];
#else

		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int j = 0; j < i_sphereData.sphereDataConfig->physical_num_lat; j++)
			for (int i = 0; i < i_sphereData.sphereDataConfig->physical_num_lon; i++)
				out.physical_space_data[(i_sphereData.sphereDataConfig->physical_num_lat-1-j)*i_sphereData.sphereDataConfig->physical_num_lon + i] = i_sphereData.physical_space_data[j*i_sphereData.sphereDataConfig->physical_num_lon + i];
#endif

		return out;
	}
};

}

#endif
