/*
 * CANFD-filter.cc
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
			//cout<<" p.canids " <<p.canids.size()  <<" unique_can.size " << unique_canids.size() <<" budget " << bit_budget<< endl;
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
			cout<<setfill('0') << setw(8)<<hex<<id<<", ";
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

			for (auto& item : dp) {
			if (item.first == key) continue;
			if ((item.first | key) == key) {
				//merge...
				for (auto id : item.second.canids) {
					if (dp[key].canids.count(id)) continue;
					bitset<11> id_to_add{(ull)id};
					for (int i = 0; i < 11; ++ i) {
						if (dp[key].state.test(i)) id_to_add.reset(i);
					}
					if ( (id_to_add.to_ullong() & dp[key].common.to_ullong()) == dp[key].common.to_ullong() ) {
						dp[key].canids.insert (id);
					}
				}
			}
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

	oss<<"#!/bin/sh\n";

	/*
	echo 0x03020100 > /sys/class/net/can1/c1fltcon0
	echo 0x07060504 > /sys/class/net/can1/c1fltcon1
	echo 0x0b0a0908 > /sys/class/net/can1/c1fltcon2
	echo 0x0f0e0d0c > /sys/class/net/can1/c1fltcon3
	echo 0x13121110 > /sys/class/net/can1/c1fltcon4
	echo 0x17161514 > /sys/class/net/can1/c1fltcon5
	echo 0x1b1a1918 > /sys/class/net/can1/c1fltcon6
	echo 0x1f1e1d1c > /sys/class/net/can1/c1fltcon7
	*/


	oss<<"echo 0x03020100> /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon0\n"
			<< "echo 0x07060504 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon1\n"
			<< "echo 0x0b0a0908 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon2\n"
			<< "echo 0x0f0e0d0c > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon3\n"
			<< "echo 0x13121110 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon4\n"
			<< "echo 0x17161514 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon5\n"
			<< "echo 0x1b1a1918 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon6\n"
			<< "echo 0x1f1e1d1c > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon7\n";

	for (size_t i = 0; i < knapsacks.size();++ i) {
		knapsacks[i].state.flip();
		//oss<<"echo  0x400007ff > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"mask"<<i<<"\n";
		oss<<"echo  0x"<< setfill('0') << setw(8)<<hex<< ((int)knapsacks[i].state.to_ullong()|0x40000000) <<dec <<" > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"mask"<<i<<"\n";
		oss<<"echo  0x"<< setfill('0') << setw(8)<<hex<<(int)knapsacks[i].common.to_ullong() <<dec <<" > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltobj"<<i<<"\n\n";
	}
	for (int i = knapsacks.size(); i < 32; ++ i) {
		oss<<"echo  0x400007ff > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"mask"<<i<<"\n";
		oss<<"echo  0x00000000 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltobj"<<i<<"\n\n";
	}
	oss<<"echo 0x8b8a8988 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon0\n"
			<< "echo 0x8f8e8d8c > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon1\n"
			<< "echo 0x93929190 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon2\n"
			<< "echo 0x97969594 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon3\n"
			<< "echo 0x009a9998 > /sys/class/net/can"<<ifindex<<"/c"<<ifindex<<"fltcon4\n";
	return oss.str();
}
