#pragma once
#include <vector>
#include "solution.h"

namespace route_evaluate {
	// Finalize solution representation
	void update_route_chromosome(
		std::vector<int> & route_chromosome,
		std::vector<int> & route,
		int route_id,
		int start_pos=0);

	// ROUTE BASIS
	// [[Start -> End]]
	// The update functions take an object and update it
	// All update functions operate on route basis
	void update_load_levels(
		std::vector<double> &loads,
		std::vector<int> &load_levels,
		const std::vector<int> &route,
		const unsigned int start_pos,
		const std::vector<double> &demand,
		const double load_bucket_size);

	void update_visit_times(
		double &driving_time,
		std::vector<double> &arrival_times,
		std::vector<double> &departure_times,
		const double starting_time,
		const std::vector<int> &route,
		const std::vector<int> &load_levels,
		const std::vector<double> &start_window,
		const std::vector<std::vector<std::vector<double>>> &time_cube,
		const std::vector<double> &service_times);

	// [[Complete Route]]
	double get_starting_time(
		std::vector<int> &route,
		std::vector<int> &load_levels,
		const std::vector<double> &start_windows,
		const std::vector<std::vector<std::vector<double>>> &time_cube);

	double get_quality(
		const double time,
		const double capa_error,
		const double frame_error,
		const double capa_error_weight,
		const double frame_error_weight
	);

	/*
	DEPRECATED! (consider waiting times!)

	double get_total_time(
		const std::vector<int> &route,
		const double start_time,
		const std::vector<double> &departure_times,
		const std::vector<std::vector<std::vector<double>>> &time_cube);
	*/

	double get_capa_error(
		const std::vector<int> &route,
		const unsigned int vehicle_capacity,
		const std::vector<double> &loads);

	double get_frame_error(
		const std::vector<int> &route,
		const std::vector<double> &end_window,
		const std::vector<double> &arrival_times);

	bool is_feasible(double capa_error, double time_error);

	int get_customer_pos(
		const std::vector<int> &route,
		const int customer_id);
}