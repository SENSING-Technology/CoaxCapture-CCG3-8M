#include "fun.h"
void convert16to8(unsigned char* src, unsigned char* dst)
{
    dst[0] =  ((src[1]<<4)&0xf0) + ((src[0]>>4)&0xf);
    printf("conver %02x %02x\n",dst[0],dst[1]);
    return ;
}

void convert16to12(unsigned char* src, unsigned char* dst)
{
    dst[0] = ((src[1]<<4)&0xf0) + ((src[0]>>4)&0xf);
    dst[1] = ((src[0]<<4)&0xf0) + (src[3]&0xf);
    dst[2] =src[2];
    printf("conver %02x %02x %02x\n",dst[0],dst[1],dst[2]);
    return ;
}

void convert16to12_seq1(unsigned char* src, unsigned char* dst)
{
    //unsigned char src[6];

    dst[0] = ((src[1]<<4)&0xf0) + ((src[0]>>4)&0xf);
    dst[1] = ((src[3]<<4)&0xf0) + ((src[2]>>4)&0xf);
    dst[2] = ((src[0]&0xf) + ((src[2]<<4)&0xf0));
    printf("convert16to12_seq1 %02x %02x %02x\n",dst[0],dst[1],dst[2]);
    return ;
}

unsigned int get_width(int ia)
{
    //src0 src2 8bit is value
    if(0==(ia%2))
    {
        return 8;
    }
    else
    {
        return 4;
    }
}

void comb_byte(unsigned int &dst, unsigned char src, unsigned int offset )
{
    //printf("comb_byte dst:%08x src:%02x offset:%d\n",dst,src,offset);
    dst |= (src << offset);
}

void uncomb_byte(unsigned int &dst, unsigned int width,unsigned int offset)
{
    //printf("uncomb_byte dst:%08x width:%02x offset:%d\n",dst,width,offset);
    unsigned int mask = 0;
    if(width == 4)
    {
        mask = 0xf;
    }
    else
    {
        mask = 0xff;
    }
    mask = (mask << (24-offset));
    dst &=(~mask);
    //printf("uncomb_byte result dst:%08x width:%02x offset:%d\n",dst,width,offset);
}

void convert16to12_seqx(unsigned char* src,unsigned int comb_values[])
{
    unsigned int temp =0;
    unsigned int cursor=0;
    unsigned int four_offsets[24]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for(int ia=0; ia<4;ia++)
    {
        //con
        four_offsets[cursor] = get_width(ia);
        comb_byte(temp,src[ia],24-(four_offsets[cursor]));
        int ib =0;
        for(; ib<4; ib++)
        {
            if(ib == ia){
                continue;
            }
            //con
            four_offsets[cursor] = get_width(ia);
            four_offsets[cursor] +=  get_width(ib);
            comb_byte(temp,src[ib],24-(four_offsets[cursor]));
            int ic=0;
            for(;ic<4; ic++)
            {
                if((ic == ib) || (ic == ia))
                {
                    continue;
                }
                //con
                four_offsets[cursor] = get_width(ia);
                four_offsets[cursor] += get_width(ib);
                four_offsets[cursor] += get_width(ic);
                comb_byte(temp,src[ic],24-four_offsets[cursor]);
                int id =0;
                for(; id<4; id++)
                {
                    if((id == ia)|| (id==ib) || (id==ic))
                    {
                        continue;
                    }
                    //con
                   four_offsets[cursor] = get_width(ia);
                   four_offsets[cursor] += get_width(ib);
                   four_offsets[cursor] += get_width(ic);
                   four_offsets[cursor] += get_width(id);
                   comb_byte(temp,src[id],24-four_offsets[cursor]);

                    comb_values[cursor] = temp;
                    //printf("comb_%d(%d_%d_%d_%d): value(%08x) width(%d)\n",
                    //cursor,ia,ib,ic,id,comb_values[cursor],four_offsets[cursor]);
                    cursor++;
                    uncomb_byte(temp,get_width(id),get_width(ia)+get_width(ib)+get_width(ic)+get_width(id));
                }
            uncomb_byte(temp,get_width(ic),get_width(ia)+get_width(ib)+get_width(ic));    
            }
        uncomb_byte(temp,get_width(ib),get_width(ia)+get_width(ib));    
        } 
        uncomb_byte(temp,get_width(ia),get_width(ia));
    }
    return ;
}

void convert12to16(unsigned char* src, unsigned char* dst)
{
    unsigned short val1,val2;
    val1 = (src[0]<<4)|(src[2]&0x0f);
    val2 = (src[1]<<4)|((src[2]>>4)&0x0f);

    //printf("convert12to16 val1:%04x val2:%04x\n",val1,val2);

    val1*=16;
    val2*=16;

    dst[0]= val1&0xff;
    dst[1]= (val1>>8)&0xff;
    dst[2]= val2&0xff;
    dst[3]= (val2>>8)&0xff;
    //printf("convert12to16 :0x%02x_%02x_%02x_%02x\n",dst[0],dst[1],dst[2],dst[3]);
}