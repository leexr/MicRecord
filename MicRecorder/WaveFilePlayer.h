#pragma once
#include <Windows.h>
#include <string>
#include <dsound.h>
#include <thread>
#include <atomic>
#include <mutex>

#define WaveFilePlayerBufferCount 2
enum PlayerStatus {
    Normal,
    LoadFile,
    Playing,
	Pause,
};
class WaveFilePlayer {
public:
	WaveFilePlayer(HWND hwnd);
	virtual ~WaveFilePlayer();
	void Load_fileByPath(const std::wstring &FilePath);
	void Load_file(HANDLE h_file);
	void Detach_file();
    bool Play();
	bool Stop();
	bool Pause();
	bool SetVolume(short volume);
	PlayerStatus Status();
	long WaveDuration() const;
private:
	HWND m_hwnd;
	HANDLE m_hfile;
	IDirectSound8 *m_Player;
	DSBUFFERDESC m_dsbd;
	WAVEFORMATEX m_format;
	IDirectSoundBuffer8 *m_pDSBuffer8;
	IDirectSoundBuffer *m_pDSBuffer;
    DSBPOSITIONNOTIFY m_pDSPosNotify[WaveFilePlayerBufferCount];
    std::thread BufferTransfer;
    std::atomic<bool> waiting;
	DWORD pcm_offset;
    std::mutex lock_status;
    PlayerStatus status;
	DWORD Data_length;
	DWORD Data_Begin;
	bool Detach_Close;
	short _volume;
private:
	LONG volume();
	void load_file(HANDLE h_file);
	void detach_File();
    void ParseWavformat();
    void TransferProc(int BufferSize);
	void ExitTransfer(int BufferSize);
	void FillMainBuf(int BufferSize);
};

