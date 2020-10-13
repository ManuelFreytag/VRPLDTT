#include "alns_data.h"
#include "tools.h"
#include <math.h>
#include <iostream> // used for cout
#include <algorithm> // used for min
#include <vector> // used to leverage the vector type

using namespace std;

// Define physical constants
const int max_speed_cycler = 25;
const int POWER = 350;
const double KMHTOMS = 3.6;
const double GRAVITY = 9.81; // m per s
const double DRAG_COEFFICIENT = 1.18;
const double RIDER_SURFACE = 0.83;
const double RHO = 1.18;
const double COEFFICIENT_ROLLING = 0.01;
const double AIR_RESISTANCE_CONSTANT = (RHO * DRAG_COEFFICIENT * RIDER_SURFACE) / 2;

/**
	Utility function to calculate the speed of a cyclist with fixed power
	but variable slope and mass

	Annotation1: Speed is reported in KM/H
*/
double velocity_calculation(double mass, double slope, double accuracy = 0.01) {
	// Check if negative slope
	if (slope < 0) {
		return max_speed_cycler;
	}

	// Define forces
	double rolling_resistance = (COEFFICIENT_ROLLING * mass * GRAVITY * cos(atan(slope)));
	double gravity = mass * GRAVITY * sin(atan(slope));

	// calculate velocity
	// Start with accuracy/2 to ensure correct rounding!
	// 1.9 because we want to ensure that no floating error can mess this up!
	double velocity = 0 + accuracy/1.99;
	bool x = true;

	while (x) {
		double drag = AIR_RESISTANCE_CONSTANT * pow(velocity/KMHTOMS, 2);
		double current_power = (drag + rolling_resistance + gravity)*velocity/KMHTOMS/ 0.95;
		if (current_power - POWER < 0) {
			velocity += accuracy;
		}
		else {
			x = false;
		}
	}

	if (velocity < max_speed_cycler) {
		// The initial rounding bias must be corrected again
		return velocity - accuracy/1.99;
	}
	else {
		return max_speed_cycler;
	}
}

/**
	Compute the slope matrix based on distance and elevation

	Ratio is rise over run (elevation / distance)
*/
vector<vector<double>> get_slope_matrix(const vector<vector<double>> distance_matrix, const vector<vector<double>> elevation_matrix)
{
	// initialize control flow
	const int nr_nodes = distance_matrix.size();
	vector<vector<double>> slope_matrix(nr_nodes, vector<double>(nr_nodes));

	// compute values and insert them
	for (int i = 0; i < nr_nodes; i++) {
		for (int j = 0; j < nr_nodes; j++) {
			double elevation = elevation_matrix[i][j];
			double distance = distance_matrix[i][j];
			// case 1: Distance = 0 | same node
			if (distance == 0.0) {
				slope_matrix[i][j] = 0;
			}
			// case 2: Distance > 0
			else {
				double ground_distance = sqrt(pow(distance*1000,2) - pow(elevation,2));
				slope_matrix[i][j] = (elevation / ground_distance);
			}
		}
	}
	return slope_matrix;
}


/**
	Get the time cube based on dimensions:
	load level x nodes x nodes

	Annotation: Time is reported in hours!

	@param distance_matrix
*/
vector<vector<vector<double>>> get_time_cube(const vector<vector<double>> distance_matrix,
	const vector<vector<double>> slope_matrix,
	const double vehicle_weight,
	const double vehicle_capacity,
	const double add_pseudo_capacity,
	double weight_interval_size)
{
	// get number of intervals
	double max_capacity_considered = (vehicle_capacity + add_pseudo_capacity);

	int nr_intervals = int(ceil(max_capacity_considered / weight_interval_size));
	const int nr_nodes = distance_matrix.size();

	// Build the three dimensional matrix (with 0 as default values)
	vector<vector<vector<double>>> time_cube(nr_intervals, vector<vector<double>>(nr_nodes, vector<double>(nr_nodes)));

	// Fill the values
	for (int interval = 0; interval < nr_intervals; interval++) {
		// get the mass for each interval (use min to cut the mass at the max)
		// Compute the middle of the interval!
		//double additional_mass = min(max_capacity_considered, interval*(weight_interval_size)+weight_interval_size/2);
		double additional_mass = min(max_capacity_considered, interval*(weight_interval_size)+weight_interval_size/2);

		for (int i = 0; i < nr_nodes; i++) {
			// We cannot start from i and set ij, ji at equal time
			// Because of the differing slope matrix!
			for (int j = 0; j < nr_nodes; j++) {
				// get the velocity for the current step
				double velocity = velocity_calculation(vehicle_weight + additional_mass, slope_matrix[i][j]);

				if (i == 0 & j == 7) {
					cout << "velocity: " << velocity << "additonal mass: " << additional_mass << endl;
				}

				double time = (distance_matrix[i][j] / velocity)*60;

				// set the values
				time_cube[interval][i][j] = time;
			}
		}
	}
	return time_cube;
}

/*
Create preprocessed information relevant for VRPTW and VRPLDTT
*/
void ALNSData::general_preprocessing() {
	double min_d = std::numeric_limits<double>::max();
	double max_d = -std::numeric_limits<double>::max();

	for (unsigned int i = 0; i < this->distance_matrix.size(); i++) {
		vector<double> &v2 = this->distance_matrix[i];

		for (unsigned int j = 0; j < v2.size(); j++) {
			if (distance_matrix[i][j] < min_d) {
				min_d = distance_matrix[i][j];
			}

			if (distance_matrix[i][j] > max_d) {
				max_d = distance_matrix[i][j];
			}
		}
	}

	this->norm_distance_matrix = tools::normalize_matrix_copy(distance_matrix, min_d, max_d);
	this->norm_start_window_matrix = tools::get_norm_distance_matrix(this->start_window);
	this->norm_end_window_matrix = tools::get_norm_distance_matrix(this->end_window);
	this->norm_demand_matrix = tools::get_norm_distance_matrix(this->demand);
}

void ALNSData::vrpldtt_preprocessing() {
	// 1) get slope matrix
	this->slope_matrix =  get_slope_matrix(this->distance_matrix, this->elevation_matrix);

	// 2) get time cube
	this->time_cube = get_time_cube(this->distance_matrix,
		this->slope_matrix,
		this->vehicle_weight,
		this->vehicle_cap,
		this->add_pseudo_capacity,
		this->load_bucket_size);
}
