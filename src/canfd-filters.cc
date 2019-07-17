/*
 * CANFD-filter.cc
 *
 *  Created on: Jul 17, 2019
 *      Author: Chuan Li (chuan.li@akka-na.com)
 *		Copyright: Proprietary
 */


#include "canfd-filters.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
using namespace std;

void generate_canfd_filter_contents (int* canids_c, int id_cnt, int pairs, char** bash_content, int* content_cnt, int ifindex) {
	vector<int> canids(id_cnt);
	for (int i = 0; i < id_cnt; ++ i) {
		canids[i] = canids_c[i];
	}
	CANFDFilterGenerator gen{};
	auto content = gen.gen_filters(canids, pairs, ifindex);
	*content_cnt = content.size();
	strcpy (*bash_content, content.c_str());
}

string CANFDFilterGenerator::gen_filters (vector<int>& canids, const size_t pairs, int ifindex) {

	// This will be a knapsack problem in which we put CAN IDs into a fixed amount of knapsack, and
	// we stop searching if we find one valid placement.
	// The bit_budget indicates at maximum, how many bits are different for all CAN IDs in all knapsacks.
	// The only case when the algorithm reaches a certain bit_budget is because it can't have a valid placement
	// if only using bit_budget-1.
	for (int bit_budget = 0; bit_budget < 12; ++ bit_budget) {
		unordered_set<int> unique_canids {canids.begin(), canids.end()};
			//cout<<" unique.size " << unique_canids.size() <<endl;
		vector<KS> knapsacks (pairs);
		for (auto& p : knapsacks) {
			p = gen_single_pair (unique_canids, bit_budget);
			for (auto& id : p.canids) {
				unique_canids.erase(id);
			}
			if ( unique_canids.size()  == 0) break; //
		}
		if ( unique_canids.size()  == 0) { //
			//print_all_mask_filter_pairs (knapsacks);
			return gen_bash_file_content (knapsacks, ifindex);
		}
	}
	return "";
}

void CANFDFilterGenerator::print_all_mask_filter_pairs(vector<KS> &knapsacks) {
	cout<<"There are in total " <<knapsacks.size() << " kanpsacks" <<endl;
	for (auto k : knapsacks) {
		cout<<" diff_cnt " <<k.diff_cnt <<" common " << k.common.to_string() << " state " << k.state.to_string() << " ids {";
		for (auto id : k.canids) {
			cout<<hex<<id<<", ";
		}
		cout<<"}"<<dec<<endl;
	}
	cout<<"Print completed."<<endl<<endl;
}


KS CANFDFilterGenerator::gen_single_pair (unordered_set<int>& canids, int budget) {
	KS ks{};
	using ull = unsigned long long;
	int budget_min = -1;
	size_t candidate_size{};

	vector<int> ids {canids.begin(), canids.end()};
	unordered_map<ull, KS> dp{};
	dp[0] = KS{};
	dp[0].common = bitset<11>{(ull)ids[0]};
	dp[0].canids.insert(ids[0]);
	for (const auto& id : ids) {
		for (const auto& d : dp) {
			bitset<11> state {d.second.state};
			bitset<11> common {d.second.common};
			int diff {d.second.diff_cnt};

			bitset<11> canid_bits {(ull)id};
			for (int i = 0; i < 11; ++ i) {
				if (state.test(i) == false && common.test(i) != canid_bits.test(i)) {
					diff ++;
					state.set(i);
					common.reset(i);
				}
			}
			ull key = state.to_ullong();
			set<int> _canids (d.second.canids.begin(), d.second.canids.end());
			_canids.insert (id);
			if (diff <= budget && dp.count(key) && dp[key].canids.size() < _canids.size()) {
				dp[key].state = state;
				dp[key].common = common;
				dp[key].diff_cnt = diff;
				dp[key].canids.clear();
				dp[key].canids.insert(_canids.begin(), _canids.end());
			}
			else if (diff <= budget ){
				KS _ks{};
				_ks.common = common;
				_ks.state = state;
				_ks.diff_cnt = diff;
				_ks.canids.insert(_canids.begin(), _canids.end());
				dp[key] = _ks;
			}

			if (diff<=budget && diff >= budget_min && dp[key].canids.size() > candidate_size) {
				ks = dp[key];
			}
		}
	}
	return ks;

}


string CANFDFilterGenerator::gen_bash_file_content (vector<KS> &knapsacks, int ifindex) {
	ostringstream oss;
	for (size_t i = 0; i < knapsacks.size();++ i) {
		oss<<"echo  0x400007ff > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"mask"<<i<<"\n";
		oss<<"echo  0x"<<hex<<(int)knapsacks[i].common.to_ullong() <<dec <<" > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltobj"<<i<<"\n\n";
	}
	for (int i = knapsacks.size(); i < 32; ++ i) {
		oss<<"echo  0x400007ff > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"mask"<<i<<"\n";
		oss<<"echo  0x00000000 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltobj"<<i<<"\n\n";
	}
	return oss.str();
}
