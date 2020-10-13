/**
Recurring tools that are needed within the ALNS script.
Templated functions must be defined directly in a header file! (otherwise linker error)
*/
#pragma once
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <tuple>

// Define random macros based on http://www.cse.yorku.ca/~oz/marsaglia-rng.html
#define znew (z=36969*(z&65535)+(z>>16))
#define wnew (w=18000*(w&65535)+(w>>16))
#define MWC ((znew<<16)+wnew )
#define SHR3 (jsr^=(jsr<<17), jsr^=(jsr>>13), jsr^=(jsr<<5))
#define CONG (jcong=69069*jcong+1234567)
#define FIB ((b=a+b),(a=b-a))
#define KISS ((MWC^CONG)+SHR3)
#define LFIB4 (c++,t[c]=t[c]+t[UC(c+58)]+t[UC(c+119)]+t[UC(c+178)])
#define SWB (c++,bro=(x<y),t[c]=(x=t[UC(c+34)])-(y=t[UC(c+19)]+bro))
#define UNI (KISS*2.328306e-10) //uniform 0-1
#define VNI ((long) KISS)*4.656613e-10 // uniform -1 to 1
#define UC (unsigned char) /*a cast operation*/

typedef unsigned long UL;
/* Global static variables: */
static UL z = 362436069, w = 521288629, jsr = 123456789, jcong = 380116160;
static UL a = 224466889, b = 7584631, t[256];
/* Use random seeds to reset z,w,jsr,jcong,a,b, and the table
t[256]*/
static UL x = 0, y = 0, bro; static unsigned char c = 0;

namespace tools {
	// Build vector that contains a value range from min to max
	std::vector<int> range(int max, int min = 0);

	// Get random number between min and max
	int rand_number(int max, int min = 0, bool fixed = true);

	// Get random number normal distributed
	int rand_number_normal(double mean, double std);

	/**
	Replace the values of v1 from v2 at positions

	In our programm mainly used to reset vectors to its old state without O(N)
	and having to copy the position vector
	*/
	template<typename T>
	void replace_vectorparts(
		std::vector<T>& v1, 
		std::vector<T>& v2, 
		std::vector<int>& positions, 
		int start_pos, 
		int end_pos) 
	{
		for (int iter = start_pos; iter <= end_pos; iter++) {
			int pos = positions[iter];
			v1[pos] = v2[pos];
		}
	}

	/**
	Get ranks of elements in vector v
	Works on all types comparable with "<" (e.g. double, strings etc.)

	Time-complexity
		O(n*log(n) + n)
	*/
	template<typename T>
	std::vector<int>  get_ranks(std::vector<T> & v) {

		// 1) Build tuple
		std::vector<std::tuple<T, int>> tup_v(v.size());

		for (unsigned int i = 0; i < v.size(); i++) {
			tup_v[i] = std::make_tuple(v[i], i);
		}

		// 2) sort
		std::sort(tup_v.begin(), tup_v.end());

		// 3) set ranks
		// init stuff
		int rank = 1;
		std::vector<int> ranks(v.size());

		// set first element
		ranks[std::get<1>(tup_v[0])] = rank;
		T prev_value = std::get<0>(tup_v[0]);

		for (unsigned int it = 1; it < v.size(); it++) {
			// Check if the neighbor is equal, if not push the rank one up
			T current_value = std::get<0>(tup_v[it]);

			if (prev_value == current_value) {
			ranks[std::get<1>(tup_v[it])] = rank;
			}
			else {
				rank++;
				ranks[std::get<1>(tup_v[it])] = rank;
			}

			prev_value = current_value;
		}

		return ranks;
	}

	/**
	Returns a vector containing the indices of the input vector in a sorted manner.
	*/
	template<typename T>
	std::vector<int> sort_indices(std::vector<T> &v) {
		// 1) Build tuple
		std::vector<std::tuple<T, int>> tup_v(v.size());

		for (unsigned int i = 0; i < v.size(); i++) {
			tup_v[i] = std::make_tuple(v[i], i);
		}

		// 2) sort
		std::sort(tup_v.begin(), tup_v.end());

		// 3) Build sorted index vector
		std::vector<int> sorted_indices(v.size());

		for (unsigned int i = 0; i < v.size(); i++) {
			sorted_indices[i] = std::get<1>(tup_v[i]);
		}
		return sorted_indices;
	}

	// Remove element at position n
	template <typename T>
	void remove_at(
		std::vector<T>& v,
		typename std::vector<T>::size_type n)
	{
		v.erase(v.begin() + n);
	}


	// Normalize a matrix
	void normalize_matrix(std::vector<std::vector<double>> &m, double min, double max);

	// Normalize a matrix and return a copy
	std::vector<std::vector<double>> normalize_matrix_copy(std::vector<std::vector<double>> &m, double min, double max);

	// Create a normalized (square) matrix based on a vector
	std::vector<std::vector<double>> get_norm_distance_matrix(std::vector<double> &v1);

	// Transform a matrix from 3 dimensions to 2
	std::vector<std::vector<double>> reduce_dim(std::vector<std::vector<std::vector<double>>> &m);
}