#include "config.h"

struct cfg_tab 
{
	int unit;
	int addr;
	int irq;
	int dma;
};

struct cfg_tab snd_cfg_tab[] = 
{

	{OSS_0, OSS_0_SIOA, OSS_0_VECT, OSS_0_CHAN},
#ifdef OSS_1_VECT
	{OSS_1, OSS_1_SIOA, OSS_1_VECT, OSS_1_CHAN},
#endif
#ifdef OSS_2_VECT
	{OSS_2, OSS_2_SIOA, OSS_2_VECT, OSS_2_CHAN},
#endif
#ifdef OSS_3_VECT
	{OSS_3, OSS_3_SIOA, OSS_3_VECT, OSS_3_CHAN},
#endif
#ifdef OSS_4_VECT
	{OSS_4, OSS_4_SIOA, OSS_4_VECT, OSS_4_CHAN},
#endif
#ifdef OSS_5_VECT
	{OSS_5, OSS_5_SIOA, OSS_5_VECT, OSS_5_CHAN},
#endif
#ifdef OSS_6_VECT
	{OSS_6, OSS_6_SIOA, OSS_6_VECT, OSS_6_CHAN},
#endif
#ifdef OSS_7_VECT
	{OSS_7, OSS_7_SIOA, OSS_7_VECT, OSS_7_CHAN},
#endif
	{-1, 0, 0}
};
