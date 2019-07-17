/*
 * CANFD-filter.h
 *
 *  Created on: Jul 17, 2019
 *      Author: Chuan Li (chuan.li@akka-na.com)
 *		Copyright: Proprietary
 */

#ifndef CANFD_FILTERS_H__
#define CANFD_FILTERS_H__

#ifdef __cplusplus
#include <vector>
#include <unordered_set>
#include <string>
#include <bitset>
#include <set>

using namespace std;


struct KS {
	int diff_cnt{}; //count the number of bits that are different across all CAN IDs in  canids
	bitset<11> state{}; // The positions that CAN IDs different in bits are set as one in this field
	bitset<11> common{}; // The current filter content..   its corresponding mask will always be 0x7ff for 11-bit CAN frames
	set<int> canids{}; //keep track of the CAN IDs that are added in the knapsack.
};


class CANFDFilterGenerator{
public:
	string gen_filters (vector<int>& canids, const size_t pairs, int ifindex);
private:
	/**
	 * Treat each Filter/Mask as a knapsack problem.
	 */
	KS gen_single_pair (unordered_set<int>& canids,  int budget);

	/**
	 * Utility method to print the results, good for debugging
	 */
	void print_all_mask_filter_pairs (vector<KS> &knapsacks);

	/**
	 * The final results are in knapsacks, this method generate the bash script contents
	 */
	string gen_bash_file_content (vector<KS> &knapsacks, int ifindex);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * C API ..
	 */
	void generate_canfd_filter_contents (int* canids, int id_cnt, int pairs, char** bash_content, int *content_cnt, int ifindex);
#ifdef __cplusplus
}
#endif

#endif /*CANFD_FILTERS_H__H_ */
