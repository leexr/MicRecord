#include "stdafx.h"
#include "WaveFilePlayer.h"
#include "WaveHeader.h"
#include "RuntimeException.h"
#include "WaveHeader.h"
#include <math.h>
#include <sstream>

#pragma comment(lib,"Dsound.lib")
#pragma comment(lib,"dxguid.lib")

#ifndef TRACE
	#ifdef _DEBUG  
		#define TRACE printf  
	#else  
		#define TRACE  
	#endif
#endif

WaveFilePlayer::WaveFilePlayer(HWND hwnd):
m_hwnd(hwnd)  ,
waiting(false),
_volume(100)  ,
status(PlayerStatus::Normal)
{
	if (DS_OK != DirectSoundCreate8(NULL, &m_Player, NULL)) {
		throw std::runtime_error("Create DirectSound instance error");
	}

	if (DS_OK != m_Player->SetCooperativeLevel(hwnd, DSSCL_NORMAL)) {
		m_Player->Release();
		throw std::runtime_error("SetCooperativeLevel error");
	}

    for (auto i = 0; i != WaveFilePlayerBufferCount; ++i) {
        m_pDSPosNotify[i].hEventNotify = CreateEventW(NULL, false, false, NULL);
        if (INVALID_HANDLE_VALUE == m_pDSPosNotify[i].hEventNotify) {
            while (--i > 0) {
                CloseHandle(m_pDSPosNotify[i].hEventNotify);
            }
            m_Player->Release();
            RuntimeException::Win32ErrorThrow();
        }
    }
}

LONG WaveFilePlayer::volume() {
	if (_volume <= 0) {
		return -10000;
	}
	if (_volume >= 100) {
		return 0;
	}
	return (LONG)(20.0 * log10((double)_volume / 100.0) * 100);
}

WaveFilePlayer::~WaveFilePlayer() {
	std::unique_lock<std::mutex>(lock_status);
	if (PlayerStatus::Playing == status || PlayerStatus::Pause == status) {
		m_pDSBuffer8->Stop();
		status = PlayerStatus::LoadFile;
	}
    if (PlayerStatus::LoadFile == status) {
        detach_File();
    }
    for (auto i : m_pDSPosNotify) {
        CloseHandle(i.hEventNotify);
    }
	m_Player->Release();
}

void ReadWavFile(HANDLE file, void *buf, DWORD len) {
	DWORD readed = 0, _readed = 0;
	while (readed != len) {
		if (!::ReadFile(file, (char *)buf + readed, len - readed, &_readed, NULL)) {
			RuntimeException::Win32ErrorThrow();
		}
		readed += _readed;
	}
}

void WaveFilePlayer::Load_fileByPath(const std::wstring &FilePath) {
    std::unique_lock<std::mutex> lock(lock_status);
    if (PlayerStatus::Normal != status) {
        throw std::runtime_error("palyer has already loaded file,must detch frist");
    }
	HANDLE h_file = CreateFileW(FilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    RuntimeException::Win32ErrorThrow();
	Detach_Close = true;
	load_file(h_file);

}

void WaveFilePlayer::Load_file(HANDLE h_file) {
	std::unique_lock<std::mutex> lock(lock_status);
	if (PlayerStatus::Normal != status) {
		throw std::runtime_error("palyer has already loaded file,must detch frist");
	}
	Detach_Close = false;
	load_file(h_file);
}

void WaveFilePlayer::load_file(HANDLE h_file) {
	m_hfile = h_file;
	ParseWavformat();
	DWORD BufferSize = m_format.nSamplesPerSec * 0.5 * m_format.wBitsPerSample / 8;

	m_dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
	m_dsbd.dwBufferBytes = WaveFilePlayerBufferCount * BufferSize;
	m_dsbd.dwSize = sizeof(DSBUFFERDESC);
	m_dsbd.lpwfxFormat = &m_format;


	if (FAILED(m_Player->CreateSoundBuffer(&m_dsbd, &m_pDSBuffer, NULL))) {
		if (Detach_Close) { ::CloseHandle(m_hfile); }
		throw std::runtime_error("Create SoundBuffer Error");
	}
	if (FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8))) {
		::CloseHandle(m_hfile);
		m_pDSBuffer->Release();
		throw std::runtime_error("Create SoundBuffer Manager Error");
	}
	if (FAILED(m_pDSBuffer8->SetVolume(volume()))) {
		//todo
	}

	//Get IDirectSoundNotify8  
	IDirectSoundNotify8 *m_pDSNotify;
	if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify))) {
		if (Detach_Close) { ::CloseHandle(m_hfile); }
		m_pDSBuffer->Release();
		m_pDSBuffer8->Release();
		throw std::runtime_error("Create SoundNotify Error");
	}
	for (int i = 0; i != WaveFilePlayerBufferCount; ++i) {
		m_pDSPosNotify[i].dwOffset = i * BufferSize;
	}

	if (FAILED(m_pDSNotify->SetNotificationPositions(WaveFilePlayerBufferCount, m_pDSPosNotify))) {
		m_pDSNotify->Release();
		if (Detach_Close) { ::CloseHandle(m_hfile); }
		m_pDSBuffer->Release();
		m_pDSBuffer8->Release();
		throw std::runtime_error("SetNotificationPositions Error");
	}
	waiting = true;
	BufferTransfer = std::thread([this, BufferSize] {TransferProc(BufferSize); });
	m_pDSNotify->Release();
	status = PlayerStatus::LoadFile;
}

void WaveFilePlayer::Detach_file() {
	std::unique_lock<std::mutex>(lock_status);
	if (status == PlayerStatus::Playing || status == PlayerStatus::Pause) {
		m_pDSBuffer8->Stop();
		status = PlayerStatus::LoadFile;
	}
	if (status == PlayerStatus::LoadFile) {
		detach_File();
	}
	if (status != PlayerStatus::Normal) {
		throw std::runtime_error("must load file frist");
	}
}

bool WaveFilePlayer::Play()
{
	std::unique_lock<std::mutex>(lock_status);
	if (status == PlayerStatus::Pause || status == PlayerStatus::LoadFile) {
		m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
		status = PlayerStatus::Playing;
		return true;
	}
    return false;
}
PlayerStatus WaveFilePlayer::Status() {
	std::unique_lock<std::mutex>(lock_status);
	return status;
}

bool WaveFilePlayer::SetVolume(short volume) {
	std::unique_lock<std::mutex>(lock_status);
	_volume = volume;
	if (status != PlayerStatus::Normal) {
		auto v = -this->volume();
		if (FAILED(m_pDSBuffer8->SetVolume(-v))) {
			return false;
		}
	}
	return true;
}

long WaveFilePlayer::WaveDuration() const {
	std::unique_lock<std::mutex>(lock_status);
	if (status == PlayerStatus::Normal) {
		throw std::runtime_error("must load wav file frist");
	}
	return Data_length / m_format.nAvgBytesPerSec * 1000;
}

bool WaveFilePlayer::Pause() {
	std::unique_lock<std::mutex>(lock_status);
	if (status == PlayerStatus::Playing) {
		m_pDSBuffer8->Stop();
		status = PlayerStatus::Pause;
		return true;
	}
	return false;
}

bool WaveFilePlayer::Stop() {
	std::unique_lock<std::mutex>(lock_status);
	if (status == PlayerStatus::Playing || status == PlayerStatus::Pause) {
		m_pDSBuffer8->Stop();
		pcm_offset = 0;
		SetFilePointer(m_hfile, Data_Begin, 0, FILE_BEGIN);
		status = PlayerStatus::LoadFile;
		return true;
	}
	return false;
}

void WaveFilePlayer::detach_File() {
	
    for (auto &i : m_pDSPosNotify) {
        SetEvent(i.hEventNotify);
    }
	waiting = false;

    BufferTransfer.join();
	if (Detach_Close) {
		::CloseHandle(m_hfile);
	}
	
	
	m_pDSBuffer8->Release();
	m_pDSBuffer->Release();

    status = PlayerStatus::Normal;
}

void WaveFilePlayer::ParseWavformat() {
	if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hfile, 0, 0, FILE_BEGIN)) {
		auto error = ::GetLastError();
		if (Detach_Close) {
			::CloseHandle(m_hfile);
		}
		RuntimeException::Win32ErrorThrow(error);
	}

	Data_Begin = 0;

	RIFF_HEADER m_riff = {0};
	ReadWavFile(m_hfile, &m_riff, sizeof(m_riff));
	Data_Begin += sizeof(m_riff);
	if (memcmp(m_riff.szRiffID, "RIFF", 4)) {
		CloseHandle(m_hfile);
		throw std::runtime_error("not a RIFF file");
	}
	if (memcmp(m_riff.szRiffFormat, "WAVE", 4)) {
		CloseHandle(m_hfile);
		throw std::runtime_error("not a wave format");
	}
	

	FMT_BLOCK m_fmt = { 0 };
	ReadWavFile(m_hfile, &m_fmt, sizeof(FMT_BLOCK));
	Data_Begin += sizeof(FMT_BLOCK);
	if (memcmp(m_fmt.szFmtID, "fmt ", 4)) {
		CloseHandle(m_hfile);
		throw std::runtime_error("invalid fmt block");
	}
	if (WAVE_FORMAT_PCM != m_fmt.wavFormat.wFormatTag) {
		std::ostringstream oss;
		oss << "unsupport WAVE Format :"
			<< m_fmt.wavFormat.wFormatTag
			<< " ,player support WAVE_FORMAT_PCM only";
		throw std::runtime_error(oss.str());
	}
	

	DATA_BLOCK m_data = { 0 };
	ReadWavFile(m_hfile, &m_data, sizeof(DATA_BLOCK));
	Data_Begin += sizeof(DATA_BLOCK);

	if (!memcmp(m_data.szDataID, "LIST", 4)) {
		if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hfile, m_data.dwDataSize, 0, FILE_CURRENT)) {
			auto error = GetLastError();
			if (Detach_Close) {
				::CloseHandle(m_hfile);
			}
			RuntimeException::Win32ErrorThrow(error);
		}
		Data_Begin += m_data.dwDataSize;
		ReadWavFile(m_hfile, &m_data, sizeof(DATA_BLOCK));
		Data_Begin += sizeof(DATA_BLOCK);
	}
	if (memcmp(m_data.szDataID, "data", 4)) { //ignore list
		throw std::runtime_error("invalid data block");
	} 
	Data_length = m_data.dwDataSize;

	memcpy(&m_format, &m_fmt.wavFormat, sizeof(WAVEFORMATEX) - sizeof(WORD));
}

void WaveFilePlayer::TransferProc(int BufferSize) {
    HANDLE m_events[WaveFilePlayerBufferCount];
	for (auto i = 0; i != WaveFilePlayerBufferCount; ++i) {
		m_events[i] = m_pDSPosNotify[i].hEventNotify;
	}

	//seek to pcm data
	SetFilePointer(m_hfile, Data_Begin, 0, FILE_BEGIN);
	auto buffer_offset = 0;
	pcm_offset = 0;

	LPVOID buf = NULL;
	DWORD  buf_len = 0;
	DWORD readed = 0;
	bool shuoldstop = false;

	while (waiting) {
		auto res = WaitForMultipleObjects(WaveFilePlayerBufferCount, m_events, false, INFINITE);
		
		buffer_offset = res == WAIT_OBJECT_0 ? BufferSize : 0;

		if (FAILED(m_pDSBuffer8->Lock(buffer_offset, BufferSize, &buf, &buf_len, NULL, NULL, 0))) {
			shuoldstop = true;
		}
		if (shuoldstop || !ReadFile(m_hfile, buf, buf_len, &readed, NULL)) {
			shuoldstop = true;
		}
		if (shuoldstop || FAILED(m_pDSBuffer8->Unlock(buf, buf_len, NULL, 0))) {
			shuoldstop = true;
		}
		
		if (!shuoldstop) {
			pcm_offset += buf_len;
			shuoldstop = pcm_offset >= Data_length;
		}

		if (shuoldstop) {
			pcm_offset = 0;
            Stop();
			shuoldstop = false;
		}
	}
}