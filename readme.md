# CAN filter/mask generator for MCP2517FD

## Introduction

The goal is to put different CAN IDs into different groups, such that within each group, the number of different bits in the binary representations of the CAN IDs within a group is upper-bounded by a fixed integer. The current implemenation assumes the maximum number of different bits is 11. Extending these functions to support 29-bit messages is trivial. 

The implementation tries with 0 all the way til 11. Zero means the number of bits different across all CAN IDs in the same group is zero. This can happen in two cases: 

* The group contains zero or one CAN ID. 
* The group contains multiple CAN IDs, but the bit positions that these CAN IDs differ are ignored in the filter MASK. Setting corresponding bit as zero in the MASK ignores that bit when applying CAN message filtering.

## Usage

This project expose the API written in C++ to the C language. To use, just call the function `generate_canfd_filter_content`. 

* The function takes the CAN IDs as an array as its first parameter. 
* The second parameter indicates the number of CAN IDs in the array.
* The third parameter captures the generated  bash script content. 
* The fourth parameter tells how long the bash script content is. *If the third parameter is allocated with enough length/storage, the fourth parameter is not useful*. If the returned content length is greater than the allocated storage, the returned contents cannot be used. 
* The last parameter indicates the network interface index we need to use. The naming convention for these network interface should be "can0", "can1", etc. If the name is different, e.g., "vcan0", the source code needs to change (this is naive for now). Check the code of `gen_bash_file_content` for details.

The generated output can be pasted into a bash file, if the bash file has the CiFLTCONm set properly, e.g., 

* Disable FLTENm
* Paste the content returned by the `generate_canfd_filter_content` here. 
* Enable FLTENm

## 11-bit only 

As of now, the implementation assumes the underlying CAN bus only has 11-bit CAN IDs.

## CiFLTCONm are not generated

The content for these registers are still a myth to me. Will add support later.

## Compile the C code with -lstdc++, and link the library generated by this repo.
