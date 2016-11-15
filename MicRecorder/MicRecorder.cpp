#include "stdafx.h"
#include "MicRecorder.h"
#include "WaveHeader.h"
#include "RuntimeException.h"

#pragma comment(lib,"winmm.lib")
class AsyncWriter;
class WriterMessage:public AsyncMessage{
public:
    WriterMessage() {};
    ~WriterMessage() {};
    friend class MicRecorder;
    friend class AsyncWriter;
private:
    WokerSingle single;
    std::shared_ptr<char> buf;
    int dwLength;
};

class AsyncWriter :public AsyncTask
{
public:
    AsyncWriter(size_t BuferLength, HANDLE h_file,const WAVEFORMATEX& format) 
    :wavformat(format), m_hfile(h_file), buf(new char[BuferLength]),bufferwritren(0), len_recorded(0),BufferSize(BuferLength)
    {

    }
    ~AsyncWriter(){};
protected:
    virtual void Process(const AsyncMessage &Message) override {
        const WriterMessage &message = *(const WriterMessage *)&Message;
        if (WokerSingle::Stop == message.single) {
            WriteWavFile(buf.get(), bufferwritren);
            WriteEnd(len_recorded);
            return;
        }
        else if (WokerSingle::Open == message.single) {
            WriteHead();
        }
        else if (WokerSingle::Write == message.single) {
            len_recorded += message.dwLength;
            if (bufferwritren + message.dwLength < BufferSize) {
                memcpy(buf.get() + bufferwritren, message.buf.get(), message.dwLength);
                bufferwritren += message.dwLength;
            }
            else {
                auto len_copy = BufferSize - bufferwritren;
                memcpy(buf.get() + bufferwritren, message.buf.get(), len_copy);
                WriteWavFile(buf.get(), BufferSize);
                if (len_copy != message.dwLength) {
                    bufferwritren = message.dwLength - len_copy;
                    auto source = message.buf.get() + len_copy;
                    memcpy(buf.get(), source, bufferwritren);
                }
                else {
                    bufferwritren = 0;
                }
            }
        }
    }
private:
    size_t BufferSize;
    HANDLE m_hfile;
    std::shared_ptr<char> buf;
    const WAVEFORMATEX &wavformat;
    DWORD bufferwritren;
    DWORD len_recorded ;
    void WriteWavFile(void * buf, DWORD data_len)
    {
        DWORD written = 0;
        while (written < data_len)
        {
            DWORD _written;
            if (!::WriteFile(m_hfile, buf, data_len, &_written, NULL)) {
                break;
            }
            written += _written;
        }
    }
    void WriteEnd(DWORD TotalBytes)
    {
        DWORD data = 4 + sizeof(FMT_BLOCK) + sizeof(DATA_BLOCK) + TotalBytes;
        SetFilePointer(m_hfile, sizeof(char[4]), NULL, FILE_BEGIN);
        WriteWavFile(&data, 4);

        SetFilePointer(m_hfile, sizeof(RIFF_HEADER) + sizeof(FMT_BLOCK) + sizeof(char[4]), NULL, FILE_BEGIN);
        WriteWavFile(&TotalBytes, 4);
    }
    void WriteHead()
    {
        //init RIFF_HEADER
        RIFF_HEADER m_riff = { 0 };;
        memcpy(m_riff.szRiffID, "RIFF", 4);
        memcpy(m_riff.szRiffFormat, "WAVE", 4);

        //init FMT_BLOCK
        FMT_BLOCK m_fmt = { 0 };
        memcpy(m_fmt.szFmtID, "fmt ", 4);
        m_fmt.dwFmtSize = sizeof(WAVE_FORMAT);
        m_fmt.wavFormat = *(WAVE_FORMAT*)&wavformat;

        //init Data
        DATA_BLOCK m_data = { 0 };
        memcpy(m_data.szDataID, "data", 4);

        WriteWavFile(&m_riff, sizeof(RIFF_HEADER));
        WriteWavFile(&m_fmt, sizeof(FMT_BLOCK));
        WriteWavFile(&m_data, sizeof(DATA_BLOCK));
    }
};

MicRecorder::MicRecorder(const WavFormat & format, int DeviceIndex):
format(format)                  ,
wavformat(wavformatInit(format)),
DeviceIndex(DeviceIndex)        ,
MainHDR({0})                    ,
SecondHDR({0})                  ,
status(recorderStaus::Stoped)   ,
WaveDataLen(0)                  ,
stopping(false)					,
BufferInQueue(0)
{
    mainbuf = std::shared_ptr<char>(new char[BufferSize()]);
    seconebuf = std::shared_ptr<char>(new char[BufferSize()]);
}

MicRecorder::~MicRecorder()
{
    std::unique_lock<std::mutex> lock(StausLock);
    if (recorderStaus::Recording == status) {
        stop();
    }
}
void MicRecorder::start(HANDLE h_file) {
    Writer = std::make_shared<AsyncWriter>(1 << 20, h_file, wavformat);
	::SetFilePointer(h_file, 0, 0, FILE_BEGIN);
	RuntimeException::Win32ErrorThrow();
	::SetEndOfFile(h_file);
	m_hfile = h_file;

	//open_input_device
	auto r = waveInOpen(&hwi,
						DeviceIndex,
						&wavformat,
						(DWORD_PTR)waveInProc,
						(DWORD_PTR)this,
						CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);

	if (MMSYSERR_NOERROR != r) {
		if (stop_close) {
			CloseHandle(m_hfile);
		}
		RuntimeException::mmErrorThrow(r);
	}

	//initRecordBuffer
	SecondHDR.lpData = seconebuf.get();
	MainHDR.lpData = mainbuf.get();
	MainHDR.dwBufferLength = SecondHDR.dwBufferLength = BufferSize();
	waveInPrepareHeader(hwi, &MainHDR, sizeof(WAVEHDR));
	if (MMSYSERR_NOERROR != r) {
		waveInClose(hwi);
		if (stop_close) {
			CloseHandle(m_hfile);
		}
		RuntimeException::mmErrorThrow(r);
	}
	r = waveInPrepareHeader(hwi, &SecondHDR, sizeof(WAVEHDR));
	if (MMSYSERR_NOERROR != r) {
		waveInUnprepareHeader(hwi, &MainHDR, sizeof(WAVEHDR));
		waveInClose(hwi);
		if (stop_close) {
			CloseHandle(m_hfile);
		}
		RuntimeException::mmErrorThrow(r);
	}

	//Begin Record
	for (auto &i : std::vector<std::function<MMRESULT(void)>>
	{
		[this]()->MMRESULT {return waveInAddBuffer(hwi, &MainHDR, sizeof(WAVEHDR)); },
		[this]()->MMRESULT {return waveInAddBuffer(hwi, &SecondHDR, sizeof(WAVEHDR)); },
		[this]()->MMRESULT {return waveInStart(hwi); }
	}) 
	{
		if (r = i(), MMSYSERR_NOERROR != r) {
			waveInUnprepareHeader(hwi, &MainHDR, sizeof(WAVEHDR));
			waveInUnprepareHeader(hwi, &SecondHDR, sizeof(WAVEHDR));
			waveInClose(hwi);
			if (stop_close) {
				CloseHandle(m_hfile);
			}
			RuntimeException::mmErrorThrow(r);
		}
	}

	status = recorderStaus::Recording;
	return;
}

void MicRecorder::Start(HANDLE h_file) {
	std::unique_lock<std::mutex> lock(StausLock);
	if (recorderStaus::Stoped != status) {
		throw std::runtime_error("MicRecorder is running");
	}
	stop_close = false;
	start(h_file);
}

void MicRecorder::StartByPath(const std::wstring & FilePath)
{
    std::unique_lock<std::mutex> lock(StausLock);
    if (recorderStaus::Stoped != status) {
        throw std::runtime_error("MicRecorder is running");
    }
	//CreateFile
	auto h_file = ::CreateFileW(FilePath.c_str()			,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ				,
								NULL						,
								CREATE_NEW					,
								FILE_ATTRIBUTE_NORMAL		,
								NULL);
	if (INVALID_HANDLE_VALUE == h_file) {
		RuntimeException::Win32ErrorThrow();
	}
	stop_close = true;
	start(h_file);
}

void MicRecorder::Stop()
{
    std::unique_lock<std::mutex> lock(StausLock);
    stop();
    status = recorderStaus::Stoped;
}

bool MicRecorder::EnumDevices(std::vector<std::wstring>& devs)
{
    if (auto count = ::waveInGetNumDevs()) {
        while (count--) {
            WAVEINCAPS cap = { 0 };
            ::waveInGetDevCapsW(count, &cap, sizeof(WAVEINCAPS));
            std::wstring name = cap.szPname;
            for (auto &i : name) {
                if (L'(' == i) {
                    i = 0;
                    break;
                }
            }
            devs.emplace_back(std::move(name));
        }
        return true;
    }
    return false;
}

void MicRecorder::stop()
{
    if (recorderStaus::Recording != status) {
        throw std::runtime_error("recoder is not running");
    }
    stopping = true;

    Writer->WaitForExit();

	auto r = waveInStop(hwi);
    r = waveInUnprepareHeader(hwi, &SecondHDR, sizeof(WAVEHDR));
    r = waveInUnprepareHeader(hwi, &MainHDR, sizeof(WAVEHDR));
    r = waveInClose(hwi);
    
	if (stop_close) {
		CloseHandle(m_hfile);
	}
    stopping = false;
}

WAVEFORMATEX MicRecorder::wavformatInit(const WavFormat & format)
{
	WAVEFORMATEX wavformat;
	wavformat.cbSize = sizeof(WAVEFORMATEX);
	wavformat.nAvgBytesPerSec = format.BytesPerSecond();
	wavformat.nBlockAlign = format.Channels * format.BytesPerSample();
	wavformat.nChannels = format.Channels;
	wavformat.nSamplesPerSec = format.SampleRate;
	wavformat.wBitsPerSample = format.BitsPerSample;
    wavformat.wFormatTag = WAVE_FORMAT_PCM;
	return wavformat;
}

void MicRecorder::waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    auto self = (MicRecorder*)dwInstance;
    auto message = std::make_shared<WriterMessage>();
    if (self->stopping) {
		if (!(self->BufferInQueue -= 1)) {
			message->single = WokerSingle::Stop;
			self->Writer->Post_Message(message);
			self->Writer->Exit();
		}
        return;
    } else if (WIM_OPEN == uMsg) {
		self->BufferInQueue = 2;
        message->single = WokerSingle::Open;

    } else if (WIM_DATA == uMsg) {
        auto pWaveHeader = (WAVEHDR*)dwParam1;
        if (!pWaveHeader->dwBytesRecorded) {
            return;
        }
        
        message->single = WokerSingle::Write;
        message->dwLength = pWaveHeader->dwBytesRecorded;
        message->buf.reset(new char[pWaveHeader->dwBytesRecorded]);
        memcpy(message->buf.get(), pWaveHeader->lpData, pWaveHeader->dwBytesRecorded);
        waveInAddBuffer(hwi, pWaveHeader, sizeof(WAVEHDR));
    } else {
        return;
    }
    self->Writer->Post_Message(message);
}