#include "roulette_wheel.h"
#include "tools.h"
#include <vector>

using namespace std;

/**
	Utility function to return a random functor based on their weights
	Idea: Sum all weights and generate a real number with max sum(weights)

	Iterate over the sum of weights and select the functor that is just below that

	Binary search could also be used but it worse in a setting with constant
	weight change!

	Funfacts:
		Worst case complexity is O(2n)
		Medium complexity is O(1.5n)
*/
int RouletteWheel::get_random_functor_id() {
	// 1) Get sum of weights! (this changes often!)
	// Only time O(n_operators) ez.
	double sum_of_weight = 0;
	for (double weight : this->weights) {
		sum_of_weight += weight;
	}
	double rnd = UNI*sum_of_weight;

	// 2) Get random functor based on weights
	double current_weight = 0;
	for (int functor_id = 0; functor_id < this->nr_functors; functor_id++) {
		current_weight += weights[functor_id];

		// check if selected value is below current threshold -> return
		if (rnd <= current_weight) {
			this->last_functor_id = functor_id;
			return functor_id;
		}
	}

	throw std::exception("Unexpected error! No functor selected.");
}

/**
	Utility function that is used to update the weights of the wheel
	Must be done after each full iteration

	Annotation:
	New score has to be between 0 and 1
*/
void RouletteWheel::update_stats(double new_score) {
	// SETUP
	int new_functor_id = this->last_functor_id;

	// INCREASE
	// 1) Update functor specifics
	this->nr_uses[new_functor_id]++;
	this->scores[new_functor_id] += new_score;
}

/**
Update the weights of the roulette wheel based on the last interaction
*/
void RouletteWheel::update_weights() {
	for (int functor_id = 0; functor_id < this->nr_functors; functor_id++) {
		if (nr_uses[functor_id] > 0) {
			double weight = wheel_parameter * (this->scores[functor_id] / this->nr_uses[functor_id]) + (1 - wheel_parameter)*weights[functor_id];

			if (weight < this->min_weight) {
				// Ensure min weight even with negative values
				this->weights[functor_id] = this->min_weight;
			}
			else {
				this->weights[functor_id] = weight;
			}
		}
		else {
			// Avoid infinite values because of 0 division!
			this->weights[functor_id] = this->min_weight;
		}

		// Reset the scores after an update!
		this->scores[functor_id] = 0;
		this->nr_uses[functor_id] = 0;
	}
}

/**
	Utility function to get a random functor
	For more documentation see the [get_random_functor_id]
*/
function<vector<int>()>& DestroyRouletteWheel::get_random_operator() {
	int functor_id = this->get_random_functor_id();
	return this->destroy_functors[functor_id];
}

/**
	Utility function to get a random functor
	For more documentation see the [get_random_functor_id]
*/
function<void(vector<int>)>& InsertionRouletteWheel::get_random_operator() {
	int functor_id = this->get_random_functor_id();
	return this->repair_functors[functor_id];
}


