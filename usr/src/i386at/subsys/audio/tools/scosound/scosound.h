/*-------------------------------------------------------------------------
  Copyright (c) 1994-1996      		The Santa Cruz Operation, Inc.
  -------------------------------------------------------------------------
  All rights reserved.  No part of this  program or publication may be
  reproduced, transmitted, transcribed, stored  in a retrieval system,
  or translated into any language or computer language, in any form or
  by any  means,  electronic, mechanical, magnetic, optical, chemical,
  biological, or otherwise, without the  prior written  permission of:

           The Santa Cruz Operation, Inc.  (408) 425-7222
           400 Encinal St, Santa Cruz, CA  95060 USA
  -------------------------------------------------------------------------

  SCCS  : @(#)scosound.h	7.1	97/10/22
  Author: Shawn McMurdo (shawnm@sco.com)
  Date  : 29-Apr-94, rewritten 28-Aug-96
  File  : scosound.h

  Description:
	Application header file for scosound.

  Modification History:
  S004,	11-Nov-96, shawnm
	add option structs
  S003,	28-Aug-96, shawnm
	rewritten
  S002,	10-Mar-95, shawnm
	don't use localhost as default server
  S001,	04-Jul-94, shawnm
	change default volume and gain to 100%
  S000,	28-Apr-94, shawnm
	created
  -----------------------------------------------------------------------*/

#ifndef SCOSOUND_H
#define SCOSOUND_H

#define XtRSoundFileFormat	"SoundFileFormat"
#define XtRSoundDataFormat	"SoundDataFormat"

typedef struct {
	String audio_device;
	String file_name;
	String short_file_name;
	String null_file_name;
	int file_format;
	int data_format;
	int sample_rate;
	int channels;
	Boolean auto_play;
} ScoSoundResources;

typedef struct {
	int	value;
	char	*name;
} SampleRate;

typedef struct {
	int	value;
	char	*name;
} FileFormat;

#define wav			0
#define voc			1
#define au			2
#define snd			3
#define aiff			4

typedef struct {
	int	value;
	char	*name;
} DataFormat;

#define pcm8			0
#define u8			0
#define s8			1
#define pcm16			2
#define s16l			2
#define s16b			3
#define u16l			4
#define u16b			5
#define mulaw			6
#define ulaw			6
#define alaw			7
#define ima_adpcm		8
#define mpeg			9

#ifdef SCOSOUND_C

SampleRate sampleRate[] = {
	{8000, "8000"},
	{11025, "11025"},
	{16000, "16000"},
	{22050, "22050"},
	{32000, "32000"},
	{44100, "44100"},
	{48000, "48000"},
};

FileFormat fileFormat[] = {
	{wav, "wav"},
	{voc, "voc"},
	{au, "au"},
	{snd, "snd"},
	{aiff, "aiff"},
};

DataFormat dataFormat[] = {
	{AFMT_U8, "pcm8"},
	{AFMT_S8, "s8"},
	{AFMT_S16_LE, "pcm16"},
	{AFMT_S16_BE, "s16b"},
	{AFMT_U16_LE, "u16l"},
	{AFMT_U16_BE, "u16b"},
	{AFMT_MU_LAW, "ulaw"},
	{AFMT_A_LAW, "alaw"},
	{AFMT_IMA_ADPCM, "ima_adpcm"},
	{AFMT_MPEG, "mpeg"},
};

#else

extern SampleRate sampleRate[];
extern FileFormat fileFormat[];
extern DataFormat dataFormat[];

#endif /* SCOSOUND_C */

#define NUM_SAMPLE_RATES 7	/* XtNumber(sampleRate) */
#define NUM_FILE_FORMATS 5	/* XtNumber(fileFormat) */
#define NUM_DATA_FORMATS 10	/* XtNumber(dataFormat) */

#define DEFAULT_AUDIO_DEVICE	"/dev/dsp"
#define DEFAULT_FILE_FORMAT	wav
#define DEFAULT_DATA_FORMAT	pcm8
#define DEFAULT_SAMPLE_RATE	11025
#define DEFAULT_CHANNELS	1
#define DEFAULT_AUTO_PLAY	False
#define DEFAULT_FILE_NAME	NULL
#define DEFAULT_NULL_FILE_NAME	"None"

#endif /* SCOSOUND_H */
