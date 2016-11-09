#pragma once
#include "stdafx.h"
#include <windef.h>

extern "C" {
	struct RIFF_HEADER {
		char szRiffID[4];
		DWORD dwRiffSize;
		char szRiffFormat[4];
	};
	struct WAVE_FORMAT {
		WORD    wFormatTag;        /* format type */
		WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
		DWORD   nSamplesPerSec;    /* sample rate */
		DWORD   nAvgBytesPerSec;   /* for buffer estimation */
		WORD    nBlockAlign;       /* block size of data */
		WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
	};
	struct FMT_BLOCK {
		char  szFmtID[4];
		DWORD  dwFmtSize;
		WAVE_FORMAT wavFormat;
	};
	struct DATA_BLOCK {
		char  szDataID[4];
		DWORD  dwDataSize;
	};
}