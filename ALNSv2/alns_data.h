/**
Data framework for the VRPLDTT

Can be reduced and simultaneously used for VRP and VRPTW
*/
#pragma once
#include "tools.h"
#include <iostream>
#include <vector>
#include <algorithm> // max element

struct ALNSData {
private:
	void general_preprocessing();
	void vrpldtt_preprocessing();

public:
	// DATA
	// vehicle attributes
	int const vehicle_weight = 140;
	int const vehicle_cap = 150;
	int add_pseudo_capacity; // This value is set to the max of customer demands
	double load_bucket_size;

	// network attributes
	const int nr_depots = 1;
	const int nr_vehicles = 0;
	const int nr_nodes = 0;
	const int nr_customer = 0;
	std::vector<double> demand;
	std::vector<double> service_times;
	std::vector<double> start_window;
	std::vector<double> end_window;

	// preprocessing relevant
	std::vector<std::vector<double>> elevation_matrix; // in m
	std::vector<std::vector<double>> distance_matrix; // in km

	// derived by preprocessing
	std::vector<std::vector<double>> slope_matrix;
	std::vector<std::vector<std::vector<double>>> time_cube;
	std::vector<std::vector<double>> norm_distance_matrix;
	std::vector<std::vector<double>> norm_start_window_matrix;
	std::vector<std::vector<double>> norm_end_window_matrix;
	std::vector<std::vector<double>> norm_demand_matrix;

	// FUNCTIONS
	// Default constructor (used for Solution pickling)
	ALNSData(): load_bucket_size() {};

	/**
	VRPLDTT constructor
	The time cube is computed in preprocessing
	*/
	ALNSData(int nr_veh,
		int nr_nodes,
		int nr_cust,
		std::vector<double> demand,
		std::vector<double> service_times,
		std::vector<double> start_window,
		std::vector<double> end_window,
		std::vector<std::vector<double>> elevation_m,
		std::vector<std::vector<double>> distance_m,
		double load_bucket_size=-1,
		double nr_load_buckets=-1,
		int vehicle_weight=140,
		int vehicle_cap=150) :
		nr_vehicles(nr_veh),
		nr_nodes(nr_nodes),
		nr_customer(nr_cust),
		demand(demand),
		service_times(service_times),
		start_window(start_window),
		end_window(end_window),
		elevation_matrix(elevation_m),
		distance_matrix(distance_m),
		vehicle_weight(vehicle_weight),
		vehicle_cap(vehicle_cap)
	{
		if (nr_load_buckets > 0) {
			this->load_bucket_size = vehicle_cap / nr_load_buckets;
		}
		else if (load_bucket_size > 0) {
			this->load_bucket_size = load_bucket_size;
		}
		else {
			throw std::exception("Unexpected error: Neither interval size nor number of intervalls given.");
		}


		// perform preprocessing
		std::cout << "INFO:c++: preprocessing (START)" << std::endl;

		this->add_pseudo_capacity = int(std::ceil(*std::max_element(demand.begin(), demand.end())));
		this->general_preprocessing();
		this->vrpldtt_preprocessing();

		std::cout << "INFO:c++: preprocessing (DONE)" << std::endl;
	}

	/**
	VRPTW constructor
	time_cube explicitly, should be a two dimensional cube
	*/
	ALNSData(int nr_veh,
		int nr_nodes,
		int nr_cust,
		std::vector<double> demand,
		std::vector<double> service_times,
		std::vector<double> start_window,
		std::vector<double> end_window,
		std::vector<std::vector<std::vector<double>>> time_c,
		int vehicle_cap = 200) :
		nr_vehicles(nr_veh),
		nr_nodes(nr_nodes),
		nr_customer(nr_cust),
		demand(demand),
		service_times(service_times),
		start_window(start_window),
		end_window(end_window),
		distance_matrix(time_c[0]), // Distance matrix is relevant for shaw and equivalent to the time_cube
		time_cube(time_c),
		load_bucket_size(vehicle_cap*2), // Infeas Upper bound. A smarter upper bound has no efficiency gain
		vehicle_weight(0),
		vehicle_cap(vehicle_cap)
	{
		// perform preprocessing
		std::cout << "INFO:c++: preprocessing (START)" << std::endl;

		this->add_pseudo_capacity = int(std::ceil(*std::max_element(demand.begin(), demand.end())));
		this->general_preprocessing();

		std::cout << "INFO:c++: preprocessing (DONE)" << std::endl;
	}

	/**
	Pickel constructor
	*/
	ALNSData(int nr_veh,
		int nr_nodes,
		int nr_cust,
		std::vector<double> demand,
		std::vector<double> service_times,
		std::vector<double> start_window,
		std::vector<double> end_window,
		std::vector<std::vector<double>> slope_matrix,
		std::vector<std::vector<double>> distance_m,
		std::vector<std::vector<std::vector<double>>> time_c,
		double load_bucket_size,
		int vehicle_weight = 140,
		int vehicle_cap = 150) :
		nr_vehicles(nr_veh),
		nr_nodes(nr_nodes),
		nr_customer(nr_cust),
		demand(demand),
		service_times(service_times),
		start_window(start_window),
		end_window(end_window),
		slope_matrix(slope_matrix),
		distance_matrix(distance_m),
		time_cube(time_c),
		load_bucket_size(load_bucket_size),
		vehicle_weight(vehicle_weight),
		vehicle_cap(vehicle_cap)
	{
		// perform preprocessing
		std::cout << "INFO:c++: recration data object done (START)" << std::endl;

		this->add_pseudo_capacity = int(std::ceil(*std::max_element(demand.begin(), demand.end())));
		this->general_preprocessing();

		std::cout << "INFO:c++: recration data object done (DONE)" << std::endl;
	}
};