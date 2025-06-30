#include <iostream>
using namespace std;
void convert16to8(unsigned char* src, unsigned char* dst);
void convert16to12(unsigned char* src, unsigned char* dst);
void convert16to12_seqx(unsigned char* src,unsigned int comb_values[]);
void convert16to12_seq1(unsigned char* src, unsigned char* dst);
void convert12to16(unsigned char* src, unsigned char* dst);