#pragma once
#include "stdafx.h"
#include <windef.h>

extern "C" {
	typedef struct {
		char szRiffID[4];
		DWORD dwRiffSize;
		char szRiffFormat[4];
	}RIFF_HEADER;
	typedef struct {
		WORD    wFormatTag;        /* format type */
		WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
		DWORD   nSamplesPerSec;    /* sample rate */
		DWORD   nAvgBytesPerSec;   /* for buffer estimation */
		WORD    nBlockAlign;       /* block size of data */
		WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
	}WAVE_FORMAT;
	typedef struct {
		char  szFmtID[4];
		DWORD  dwFmtSize;
		WAVE_FORMAT wavFormat;
	}FMT_BLOCK;
	typedef struct {
		char  szDataID[4];
		DWORD  dwDataSize;
	}DATA_BLOCK;
}