/**
This file contains the low level evaluation functions for routes

The evaluation of insertion operators are almost always done
on route basis / route part basis.


Annotation:
	These routines are used frequently and form the basis
	of higher level evaluations.

	## Efficiency is therefore of high importance.##
*/
#include "evaluate.h"
#include "solution.h"

using namespace std;

/**
Update the route chromosome for all given nodes
*/
void route_evaluate::update_route_chromosome(
	vector<int> & route_chromosome,
	vector<int> &route,
	int route_id,
	int start_pos)
{
	for (unsigned int route_pos = start_pos; route_pos < route.size(); route_pos++) {
		int customer_id = route[route_pos];
		route_chromosome[customer_id] = route_id;
	}
}



/**
	Utility function to compute the load bucket / load level of a certain cummulated demand

	Annotation: We assume that the upper bound of a load interval must also be included!

	@param:	customer_demand	The total customer_demand that should be put into a bucket
*/
int get_load_bucket(double customer_demand, const double load_bucket_size) {
	// obviously the index of the bucketing starts at 0 
	// -> use floor
	// min double difference! to include upper bound aswell!
	return int((customer_demand - 0.3) / load_bucket_size);
}


/**
	Updates the load levels of an route till a certain position
	We know that the start must always be the route start.

	Annotation:
		- Update the load levels directly on the object
		- Updates all loads and load levels (including) till end_pos
		- Route basis for computational efficiency
*/
void route_evaluate::update_load_levels(
	vector<double> &loads,
	vector<int> &load_levels,
	const vector<int> &route,
	const unsigned int end_pos,
	const vector<double> &demand,
	const double load_bucket_size) 
{
	double new_load = 0;
	unsigned int r_size = route.size();

	if (r_size > 0) {
		if (end_pos < r_size - 1) {
			// Not the last position! (else null pointer)
			new_load += loads[route[end_pos + 1]];
		}

		// perform this in reverse!
		for (int route_pos = end_pos; route_pos >= 0; route_pos--) {

			// get new load
			int customer_id = route[route_pos];
			new_load += demand[customer_id]; // route[route_pos] -> customer_id!
			loads[customer_id] = new_load;
			load_levels[customer_id] = get_load_bucket(new_load, load_bucket_size);
		}
	}
}


/**
	Updates the load levels of an route starting from a certain position
	We know that the end must always be the route end.

	Annotation:
		- Update the route times directly on the object
		- Updates all times (including) starting from start_pos
		- Route basis for computational efficiency
*/
void route_evaluate::update_visit_times(
	double &route_driving_time,
	vector<double> &arrival_times,
	vector<double> &departure_times,
	const double starting_time,
	const vector<int> &route,
	const vector<int> &load_levels,
	const vector<double> &start_windows,
	const vector<vector<vector<double>>> &time_cube,
	const vector<double> &service_times)
{
	route_driving_time = 0;
	int prev_node_id = 0; // we measure it as 
	double current_time = starting_time;

	for (unsigned int route_pos = 0; route_pos < route.size(); route_pos++) {
		int customer_id = route[route_pos];
		int node_id = customer_id+1;// +1 because of its node ids!

		// update arrival time
		double ctoctime = time_cube[load_levels[customer_id]][prev_node_id][node_id];
		current_time += ctoctime;
		route_driving_time += ctoctime;

		// wait if we did not reach the beginning of the start window
		current_time = max(current_time, start_windows[customer_id]);

		arrival_times[customer_id] = current_time;

		// update departure time
		current_time += service_times[customer_id];
		departure_times[customer_id] = current_time;

		// cleanup (prepare for next iteration)
		prev_node_id = node_id;
	}

	// Finish to depot!
	route_driving_time += time_cube[0][prev_node_id][0];
}

/**
Get the starting time of a route
(Either 0 or expected arrival at first customer -> travel time)
*/
double route_evaluate::get_starting_time(
	vector<int> &route,
	vector<int> &loads_levels,
	const vector<double> &start_windows,
	const vector<vector<vector<double>>> &time_cube) 
{
	if (route.size() > 0) {
		int first_customer_id = route[0];

		// We start the route the latest we have to
		// Annotation: Depot -> first customer
		double starting_time = start_windows[first_customer_id] - time_cube[loads_levels[first_customer_id]][0][first_customer_id + 1];
		return max(0.0, starting_time);
	}
	return 0.0;
}

/**
Get the total quality of a route
*/
double route_evaluate::get_quality(
	double time,
	double capa_error, 
	double frame_error,
	double capa_error_weight,
	double frame_error_weight)
{
	return 	time + capa_error_weight * capa_error + frame_error * frame_error_weight;
}

/**
DEPRECATED!
	Get the total time of a route
	Last departure + time to depot (load level must always be 0)

double route_evaluate::get_total_time(
	const vector<int> &route,
	const double start_time,
	const vector<double> &departure_times,
	const vector<vector<vector<double>>> &time_cube)
{
	if (route.size() > 0) {
		int customer_id = route.back();
		return (departure_times[customer_id] + time_cube[0][customer_id + 1][0]) - start_time; // last_customer + depot
	}	
	return 0;
}
*/


/**
	Utiltiy function to get the capacity induced error
*/
double route_evaluate::get_capa_error(
	const vector<int> &route,
	const unsigned int vehicle_capacity,
	const vector<double> &loads) 
{
	if (route.size() > 0) {
		int customer_id = route[0]; // first customer is has the highest load
		double capacity_difference = loads[customer_id] - vehicle_capacity;
		if (capacity_difference > 0) {
			return capacity_difference;
		}
	}
	return 0.0;
}

/**
Utility function to evaluate the error by not conforming 
to the time windows

The only possibility is late arrival
Early arrival is not possible -> we wait
*/
double route_evaluate::get_frame_error(
	const vector<int> &route,
	const vector<double> &end_window,
	const vector<double> &arrival_times) 
{
	double frame_error = 0.0;
	for (int customer_id : route) {
		// 1) Get the service error for serving a customer later than his preference
		// -> arrival must be earlier otherwise its an error
		double late_diff = arrival_times[customer_id] - end_window[customer_id];

		// 2) Add it to the total difference
		if (late_diff > 0) {
			frame_error += late_diff;
		}
	}
	return frame_error;
}


/**
Simple route feasibility check
Can be used for solution feasibility as well

@param capa_error:	Capacity error of existing route
@param time_error:	Time error of existing route
*/
bool route_evaluate::is_feasible(double capa_error, double time_error) {
	if ((capa_error > 0) || (time_error > 0)) {
		return false;
	}
	else {
		return true;
	}
}
/**
Get the customer position in a route

@param &route:	Route where to look for customer position
@param customer_id: Id that should be looked up
*/
int route_evaluate::get_customer_pos(const vector<int> &route, const int customer_id) {
	int customer_pos = -1;
	for (unsigned int route_pos = 0; route_pos < route.size(); route_pos++) {
		if (route[route_pos] == customer_id) {
			customer_pos = route_pos;
			break;
		}
	}

	if (customer_pos < 0) {
		throw logic_error("The customer was not found in the route");
	}

	return customer_pos;
}

