#include "tools.h"
#include <random>

using namespace std;


int tools::rand_number(int max, int min, bool fixed) {
	/*
	std::random_device rd; // obtain a random number from hardwar
	default_random_engine eng{ rd() };

	// We can rewrite the random engine if no fixed seed is wished
	if (!fixed) {
		std::mt19937 eng(rd()); // seed the generator
	}

	std::uniform_int_distribution<> distr(min, max); // define the range

	return distr(eng);*/

	return int(round(UNI*(max - min) + min));
}

int tools::rand_number_normal(double mean, double std) {
	std::random_device rd{};
	std::mt19937 gen{ rd() };

	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> d{ mean, std };
	return int(std::round(d(gen)));
}