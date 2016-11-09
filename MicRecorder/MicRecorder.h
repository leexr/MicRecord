#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include<mutex>
#include <deque>
#include <atomic>
#include <memory>
#include <thread>
#include <condition_variable>

class WavFormat{
public:
	int BitsPerSample;
	int Channels;
	int SampleRate;
	inline int BytesPerSample() const{ return BitsPerSample / 8; }
	inline int BytesPerSecond() const { return SampleRate * Channels * BytesPerSample(); }
	static WavFormat &Default() {
		static WavFormat default;
		static std::once_flag flag;
		std::call_once(flag, [] {
			default.BitsPerSample = 16;
			default.Channels = 2;
			default.SampleRate = 44100;
		});
		return default;
	}
};
enum recorderStaus
{
    Recording,
    Stoped,
};
enum WokerSingle
{
    Open,
    Write,
    Stop
};
typedef struct {
    WokerSingle single;
    std::shared_ptr<char> buf;
    int dwLength;
}WorkerMessage,*pWorkerMessage;
class MicRecorder
{
public:
	MicRecorder(const WavFormat &format,int DeviceIndex);
	MicRecorder(const WavFormat &format) : MicRecorder(format, WAVE_MAPPER) {}
	MicRecorder() : MicRecorder(WavFormat::Default()) {};
    MicRecorder(int DeviceIndex) : MicRecorder(WavFormat::Default(), DeviceIndex) {};
	MicRecorder(const MicRecorder&) = delete;
    ~MicRecorder();
    void Start(const std::wstring &FilePath);
    void Stop();
public:
	static bool EnumDevices(std::vector<std::wstring> &devs);
private:
	WavFormat format;
	WAVEFORMATEX wavformat;
    MicRecorder::recorderStaus status;
	std::shared_ptr<char> mainbuf;
	std::shared_ptr<char> seconebuf;
    int DeviceIndex;
    HANDLE m_hfile;
    HWAVEIN hwi;
    std::mutex StausLock;
    WAVEHDR MainHDR;
    WAVEHDR SecondHDR;
    long WaveDataLen;

    std::atomic<bool> stopping;
    std::deque<WorkerMessage> singles;
    std::mutex wokerqueuelock;
    std::thread fileWriter;
    std::condition_variable writer_condition;
private:
    void stop();
    void WriterProc();
    inline int BufferSize() { return (int)format.SampleRate * 0.5 * format.BytesPerSample(); }
    inline void PostSingle(WorkerMessage message) {
        std::unique_lock<std::mutex>(wokerqueuelock);
        singles.push_back(message);
        writer_condition.notify_all();
    }
    void WriteWavFile(void *buf, DWORD data_len);
    void WriteHead();
    void WriteEnd(DWORD TotalBytes);
private:	
    static void CALLBACK waveInProc(
        HWAVEIN   hwi,
        UINT      uMsg,
        DWORD_PTR dwInstance,
        DWORD_PTR dwParam1,
        DWORD_PTR dwParam2
    );
    static WAVEFORMATEX wavformatInit(const WavFormat &format);
};