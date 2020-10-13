/**
This file contains all functions defined for a solution object
Most of the evaluation functions are only used in the constructor.

This constructor is used sparsely as we try to optimize on the same solution object.
The efficiency of these evaluation functions is therefore of secundary importance
*/
#include "alns_data.h"
#include "evaluate.h"
#include "solution.h"
#include "tools.h"
#include <vector>
#include <math.h>

using namespace std;

/**
Standard constructor

@param data_obj:			Data object containing the problem isntance
@param solution_rep:		Solution reprensetation (must be at least randomly generated)
@param capa_error_weight:	Weight for the capacity error (for quality calculation)
@param frame_error_weight:	Weight for the frame error (for quality calculation)

*/
Solution::Solution(
	ALNSData &data_obj,
	const vector<vector<int>> solution_rep,
	const double capa_error_weight,
	const double frame_error_weight):
	data_obj(data_obj)
{
	// setup
	this->solution_representation = solution_rep;

	// set customer based info
	this->evaluate_solution(capa_error_weight, frame_error_weight);
}

/**
Pickle constructor

Allows for deserialization of the Solution object
in the python interface.

ATTENTION: NEVER USE IT OTHERWISE (inefficient)
*/
Solution::Solution(
	ALNSData &data_obj,
	std::vector<std::vector<int>> solution_representation,
	std::vector<double> loads,
	std::vector<double> arrival_times,
	std::vector<double> departure_times,
	double driving_time,
	double solution_quality,
	double capa_error,
	double frame_error,
	bool is_feasible,
	std::vector<double> start_times,
	std::vector<double> route_driving_times) :
	data_obj(data_obj)
{
	this->solution_representation = solution_representation;
	this->loads = loads;
	this->arrival_times = arrival_times;
	this->departure_times = departure_times;
	this->driving_time = driving_time;
	this->solution_quality = solution_quality;
	this->capa_error = capa_error;
	this->frame_error = frame_error;
	this->is_feasible = is_feasible;
	this->start_times = start_times;
	this->route_driving_times = route_driving_times;
};

/**
Deep copy instructions

@param other:	Object of type solution that should be copied
*/
Solution& Solution::operator=(const Solution &obj) {
	if (&obj != this) {
		this->data_obj = obj.data_obj; // This is only possible with actuall reference wrappers
		// If not identical (reference is equal)
		this->solution_representation = obj.solution_representation;
		this->route_chromosome = obj.route_chromosome;

		this->loads = obj.loads;
		this->load_levels = obj.load_levels;
		this->arrival_times = obj.arrival_times;
		this->departure_times = obj.departure_times;

		this->driving_time = obj.driving_time;
		this->capa_error = obj.capa_error;
		this->frame_error = obj.frame_error;
		this->is_feasible = obj.is_feasible;
		this->solution_quality = obj.solution_quality;

		this->start_times = obj.start_times;
		this->route_driving_times = obj.route_driving_times;
		this->route_capa_errors = obj.route_capa_errors;
		this->route_frame_errors = obj.route_frame_errors;
		this->route_qualities = obj.route_qualities;
	}
	return *this;
}

/**
Base function to reevaluate the solution object from scratch
Is used in heavy destroy operators or initialization
*/
void Solution::evaluate_solution(double capa_error_weight, double frame_error_weight) {
	this->set_chromosomes();
	this->set_load_levels();
	this->set_solution_time();
	this->set_capa_error();
	this->set_frame_error();
	this->set_quality(capa_error_weight, frame_error_weight);
	this->set_is_feasible();
}


/**
Set chromosomes

Some operators require the new information description
for a significantly better time complexity
*/
void Solution::set_chromosomes() {
	ALNSData &data = this->data_obj.get();

	vector<int> route_chromosome(data.nr_customer);

	int route_id = 0;
	for (vector<int> &route : this->solution_representation) {
		route_evaluate::update_route_chromosome(
			route_chromosome,
			route,
			route_id);

		route_id++;
	}
	this->route_chromosome = route_chromosome;
}


/**
Set loads and load levels.
*/
void Solution::set_load_levels() {
	ALNSData &data = this->data_obj.get();
	// 1) Get the loads of each custom
	vector<double> loads(data.nr_customer);
	vector<int> loads_lvls(data.nr_customer); // init with 0s

	// for each route compute the value!
	for (vector<int> &route : this->solution_representation) {
		route_evaluate::update_load_levels(loads,
			loads_lvls,
			route, 
			route.size()-1, 
			data.demand, 
			data.load_bucket_size);
	}

	this->loads = loads;
	this->load_levels = loads_lvls;
}

/**
Utility function to set the following attributes:
	1) arrival_times:			Time point for each customer at which he is served
	2) departure_times:			Time point for each customer at which he the vehicle leaves
	3) driving_time:			Absolute time needed for all customers to be served
	4) route_driving_times:		Vector containing all route times

*/
void Solution::set_solution_time() {
	ALNSData &data = this->data_obj.get();

	// Initialize the solution objects
	double total_time = 0.0;
	vector<double> start_times(solution_representation.size());
	vector<double> route_driving_times(solution_representation.size(), 0);
	vector<double> arr_times(data.nr_customer);
	vector<double> dep_times(data.nr_customer);

	// we must consider all routes
	int route_id = 0;
	for (vector<int> &route : this->solution_representation) {

		double start_time = route_evaluate::get_starting_time(
			route,
			this->load_levels,
			data.start_window,
			data.time_cube);

		start_times[route_id] = start_time;

		route_evaluate::update_visit_times(
			route_driving_times[route_id],
			arr_times, 
			dep_times, 
			start_time,
			route, 
			this->load_levels, 
			data.start_window,
			data.time_cube,
			data.service_times);

		total_time += route_driving_times[route_id];
		route_id++;
	}

	this->start_times = start_times;
	this->driving_time = total_time ;
	this->route_driving_times = route_driving_times;
	this->arrival_times = arr_times;
	this->departure_times = dep_times;
}

/**
Sets capa_error and route_capa_errors.
*/
void Solution::set_capa_error() {
	ALNSData &data = this->data_obj.get();

	double capa_error = 0;
	vector<double> route_capa_errors(data.nr_vehicles);

	int route_id = 0;
	for (vector<int> &route : this->solution_representation) {

		double route_capa_error = route_evaluate::get_capa_error(route, data.vehicle_cap, this->loads);
		capa_error += route_capa_error;
		route_capa_errors[route_id] = route_capa_error;
		route_id++;
	}

	this->capa_error = capa_error;
	this->route_capa_errors = route_capa_errors;
}

/**
Utility function to get the overall infeasibility error based on customer service windows
*/
void Solution::set_frame_error() {
	ALNSData &data = this->data_obj.get();

	double frame_error = 0;
	vector<double> route_frame_errors(this->solution_representation.size());

	int route_id = 0;
	for (vector<int> &route : this->solution_representation) {

		double route_frame_error = route_evaluate::get_frame_error(route,
			data.end_window, 
			this->arrival_times);

		frame_error += route_frame_error;
		route_frame_errors[route_id] = route_frame_error;
		route_id++;
	}
	this->frame_error = frame_error;
	this->route_frame_errors = route_frame_errors;
}

/**
Set solution is_feasible of the solution object

Trivialy based on the existing errors values
*/
void Solution::set_is_feasible() {
	if ((this->capa_error > 0) || (this->frame_error > 0)) {
		this->is_feasible = false;
	}
	else {
		this->is_feasible = true;
	}
}

/**
Set solution_quality and route_qualities

@param capa_error_weight:	Weight for the capacity error (for quality calculation)
@param frame_error_weight:	Weight for the frame error (for quality calculation)
*/
void Solution::set_quality(const double capa_error_weight, const double frame_error_weight) {
	ALNSData &data = this->data_obj.get();

	double solution_quality = 0;
	vector<double> route_qualities(data.nr_vehicles);

	for (int route_id = 0; route_id < data.nr_vehicles; route_id++) {

		double route_quality = route_evaluate::get_quality(route_driving_times[route_id],
			this->route_capa_errors[route_id],
			this->route_frame_errors[route_id],
			capa_error_weight,
			frame_error_weight);
		
		solution_quality += route_quality;
		route_qualities[route_id] = route_quality;
	}

	this->solution_quality = solution_quality;
	this->route_qualities = route_qualities;
}

/**
Reevaluate the solution after a change to a route.

Indicating where the change happened gives a slight performance boost.
If not sure indicate that its the last position of the route.

@param route_id					Route ID where the customer id should be inserted
@param customer_id				Customer id to be inserted
@param ins_pos					Insertion position at route [route_id]
@param capa_error_weight:		Weight for the capacity error (for quality calculation)
@param frame_error_weight:		Weight for the frame error (for quality calculation)

@throws InfeasibilityException:	The max capa error is exceeded after insertion.

								Annotation: The applied changes to the object are not
								(capacity evaluation) are not reverted for efficiency reasons.

								Annotaiton: The potential changes to the times are not performed
								(because out of bounds exception)

								!!! Either compute the values completely from scratch !!!
								!!! or reset to previous state!!!
*/
void Solution::evaluate_change(
	const int route_id,
	const int ins_pos,
	const double capa_error_weight,
	const double frame_error_weight)
{
	ALNSData &data = this->data_obj.get();
	vector<int> &route = this->solution_representation[route_id];

	// 1) Check if we are within the computational limits! (load levels)
	// Update load levels and compute if its still within its limits
	this->capa_error -= route_capa_errors[route_id];

	route_evaluate::update_load_levels(this->loads,
		this->load_levels,
		route,
		ins_pos,
		data.demand,
		data.load_bucket_size);

	double route_capa_error = route_evaluate::get_capa_error(route, data.vehicle_cap, this->loads);
	this->capa_error += route_capa_error;

	// Check if insertion exceeds max capa infeasibility
	if (route_capa_error >= data.add_pseudo_capacity) {
		throw InfeasibilityException();
	}

	// 2) Update all necessary travel time KPIs
	// 2.1) Reduce route rest KPIs
	this->driving_time -= route_driving_times[route_id];
	this->frame_error -= route_frame_errors[route_id];
	this->solution_quality -= route_qualities[route_id];

	// 2.2) Update time impact (consideres the depot distance at the beginning)
	double start_time = route_evaluate::get_starting_time(
		route,
		this->load_levels,
		data.start_window,
		data.time_cube);

	route_evaluate::update_visit_times(
		this->route_driving_times[route_id],
		this->arrival_times,
		this->departure_times,
		start_time,
		route,
		this->load_levels,
		data.start_window,
		data.time_cube,
		data.service_times);

	double route_frame_error = route_evaluate::get_frame_error(route,
		data.end_window,
		this->arrival_times);

	double route_quality = route_evaluate::get_quality(route_driving_times[route_id], route_capa_error, route_frame_error, capa_error_weight, frame_error_weight);

	// 2.3) Update absolute KPIs
	this->driving_time += route_driving_times[route_id];
	this->frame_error += route_frame_error;
	this->solution_quality += route_quality;

	// 2.44) Update route based KPIs
	this->route_capa_errors[route_id] = route_capa_error;
	this->route_frame_errors[route_id] = route_frame_error;
	this->route_qualities[route_id] = route_quality;

	this->set_is_feasible();
}


double Solution::get_diversity(std::vector<std::vector<int>> &node_pair_useage_matrix, int iteration) {
	ALNSData &data = this->data_obj;

	int new_iter = iteration + 1;
	int diversity_norm = data.nr_customer;
	double diversity = 0;

	for (vector<int> &route : this->solution_representation) {
		// each none empty route adds one more edge
		if (route.size() > 0) {
			diversity_norm++;

			int prev_node_id = 0;
			for (int customer_id : route) {
				diversity += (1 - (node_pair_useage_matrix[prev_node_id][customer_id + 1] / new_iter));
				prev_node_id = customer_id + 1;
			}

			// back to depot
			diversity += (1 - (node_pair_useage_matrix[prev_node_id][0] / new_iter));
		}
	}

	return diversity / diversity_norm;
}
