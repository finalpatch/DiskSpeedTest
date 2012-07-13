//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <ppltasks.h>
#include <random>

using namespace DiskSpeedTest;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;

const static int kTestFrameSize = 1024*1024*5;

MainPage::MainPage()
{
	InitializeComponent();

	m_testing = false;

	m_totalWriteBytes = 0;
	m_totalWriteTime = 0;
	m_totalReadBytes = 0;
	m_totalReadTime = 0;

	StorageFolder^ tempFolder = Windows::Storage::ApplicationData::Current->TemporaryFolder;
	CREATEFILE2_EXTENDED_PARAMETERS cfp = {
		sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
		FILE_ATTRIBUTE_NORMAL,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_DELETE_ON_CLOSE |  FILE_FLAG_WRITE_THROUGH,
		0, 0 };
	String^ path = String::Concat(tempFolder->Path, L"\\DISKSPEEDTEST");
	m_hfile = CreateFile2(path->Data(), GENERIC_READ | GENERIC_WRITE, 0, CREATE_ALWAYS, &cfp);
}

MainPage::~MainPage()
{
	if(m_hfile)
		CloseHandle(m_hfile);
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

long long v210framesize(int w, int h)
{
	return ((w + 23)/24*24)*h *16/6;
}
long long r210framesize(int w, int h)
{
	return w * h * 4;
}
long long r212framesize(int w, int h)
{
	return w * h * 2 * 3;
}

String^ format_fps(long long fps)
{
	wchar_t buf[20];
	swprintf(buf, 20, L"%lld fps", fps);
	return ref new String(buf);
}

const static long long v210_data_rates[15] = {
	v210framesize(720, 576) * 25,
	v210framesize(720, 486) * 30000 / 1001,
	v210framesize(1280, 720) * 60,
	v210framesize(1280, 720) * 60000 / 1001,
	v210framesize(1920, 1080) * 24000 / 1001,
	v210framesize(1920, 1080) * 24,
	v210framesize(1920, 1080) * 30000 / 1001,
	v210framesize(1920, 1080) * 30,
	v210framesize(1920, 1080) * 25,
	v210framesize(1920, 1080) * 30000 / 1001,
	v210framesize(1920, 1080) * 50,
	v210framesize(1920, 1080) * 60000 / 1001,
	v210framesize(2048, 1556) * 24000 / 1001,
	v210framesize(2048, 1556) * 24,
	v210framesize(2048, 1556) * 25,
};
const static long long r210_data_rates[15] = {
	r210framesize(720, 576) * 25,
	r210framesize(720, 486) * 30000 / 1001,
	r210framesize(1280, 720) * 60,
	r210framesize(1280, 720) * 60000 / 1001,
	r210framesize(1920, 1080) * 24000 / 1001,
	r210framesize(1920, 1080) * 24,
	r210framesize(1920, 1080) * 30000 / 1001,
	r210framesize(1920, 1080) * 30,
	r210framesize(1920, 1080) * 25,
	r210framesize(1920, 1080) * 30000 / 1001,
	r210framesize(1920, 1080) * 50,
	r210framesize(1920, 1080) * 60000 / 1001,
	r210framesize(2048, 1556) * 24000 / 1001,
	r210framesize(2048, 1556) * 24,
	r210framesize(2048, 1556) * 25,
};
const static long long r212_data_rates[15] = {
	r212framesize(720, 576) * 25,
	r212framesize(720, 486) * 30000 / 1001,
	r212framesize(1280, 720) * 60,
	r212framesize(1280, 720) * 60000 / 1001,
	r212framesize(1920, 1080) * 24000 / 1001,
	r212framesize(1920, 1080) * 24,
	r212framesize(1920, 1080) * 30000 / 1001,
	r212framesize(1920, 1080) * 30,
	r212framesize(1920, 1080) * 25,
	r212framesize(1920, 1080) * 30000 / 1001,
	r212framesize(1920, 1080) * 50,
	r212framesize(1920, 1080) * 60000 / 1001,
	r212framesize(2048, 1556) * 24000 / 1001,
	r212framesize(2048, 1556) * 24,
	r212framesize(2048, 1556) * 25,
};

long long computeSpeed(long long bytes, long long ms)
{
	if(ms == 0)
		return 0;
	return bytes * 1000 / ms;
}

void DiskSpeedTest::MainPage::updateResult()
{
	long long writeSpeed = computeSpeed(m_totalWriteBytes, m_totalWriteTime);
	long long readSpeed = computeSpeed(m_totalReadBytes, m_totalReadTime);

	wchar_t buf[20];

	if(writeSpeed > 0)
	{
		int mb = static_cast<int>(writeSpeed / (1024 * 1024));
		swprintf(buf, 20, L"WRITE: %d Mb/s", mb);
		this->wrlbl->Text = ref new String(buf);
		this->wrbar->Value = mb;

		v210_pal_wr_ok->Text = writeSpeed >= v210_data_rates[0] ? "Y" : "N";
		v210_ntsc_wr_ok->Text = writeSpeed >= v210_data_rates[1] ? "Y" : "N";
		v210_720p60_wr_ok->Text = writeSpeed >= v210_data_rates[2] ? "Y" : "N";
		v210_720p59_wr_ok->Text = writeSpeed >= v210_data_rates[3] ? "Y" : "N";
		v210_1080p23_wr_ok->Text = writeSpeed >= v210_data_rates[4] ? "Y" : "N";
		v210_1080p24_wr_ok->Text = writeSpeed >= v210_data_rates[5] ? "Y" : "N";
		v210_1080p29_wr_ok->Text = writeSpeed >= v210_data_rates[6] ? "Y" : "N";
		v210_1080p30_wr_ok->Text = writeSpeed >= v210_data_rates[7] ? "Y" : "N";
		v210_1080i50_wr_ok->Text = writeSpeed >= v210_data_rates[8] ? "Y" : "N";
		v210_1080i59_wr_ok->Text = writeSpeed >= v210_data_rates[9] ? "Y" : "N";
		v210_1080p50_wr_ok->Text = writeSpeed >= v210_data_rates[10] ? "Y" : "N";
		v210_1080p59_wr_ok->Text = writeSpeed >= v210_data_rates[11] ? "Y" : "N";
		v210_2k23_wr_ok->Text = writeSpeed >= v210_data_rates[12] ? "Y" : "N";
		v210_2k24_wr_ok->Text = writeSpeed >= v210_data_rates[13] ? "Y" : "N";
		v210_2k25_wr_ok->Text = writeSpeed >= v210_data_rates[14] ? "Y" : "N";

		r210_pal_wr_ok->Text = writeSpeed >= r210_data_rates[0] ? "Y" : "N";
		r210_ntsc_wr_ok->Text = writeSpeed >= r210_data_rates[1] ? "Y" : "N";
		r210_720p60_wr_ok->Text = writeSpeed >= r210_data_rates[2] ? "Y" : "N";
		r210_720p59_wr_ok->Text = writeSpeed >= r210_data_rates[3] ? "Y" : "N";
		r210_1080p23_wr_ok->Text = writeSpeed >= r210_data_rates[4] ? "Y" : "N";
		r210_1080p24_wr_ok->Text = writeSpeed >= r210_data_rates[5] ? "Y" : "N";
		r210_1080p29_wr_ok->Text = writeSpeed >= r210_data_rates[6] ? "Y" : "N";
		r210_1080p30_wr_ok->Text = writeSpeed >= r210_data_rates[7] ? "Y" : "N";
		r210_1080i50_wr_ok->Text = writeSpeed >= r210_data_rates[8] ? "Y" : "N";
		r210_1080i59_wr_ok->Text = writeSpeed >= r210_data_rates[9] ? "Y" : "N";
		r210_1080p50_wr_ok->Text = writeSpeed >= r210_data_rates[10] ? "Y" : "N";
		r210_1080p59_wr_ok->Text = writeSpeed >= r210_data_rates[11] ? "Y" : "N";
		r210_2k23_wr_ok->Text = writeSpeed >= r210_data_rates[12] ? "Y" : "N";
		r210_2k24_wr_ok->Text = writeSpeed >= r210_data_rates[13] ? "Y" : "N";
		r210_2k25_wr_ok->Text = writeSpeed >= r210_data_rates[14] ? "Y" : "N";

		r212_pal_wr_ok->Text = writeSpeed >= r212_data_rates[0] ? "Y" : "N";
		r212_ntsc_wr_ok->Text = writeSpeed >= r212_data_rates[1] ? "Y" : "N";
		r212_720p60_wr_ok->Text = writeSpeed >= r212_data_rates[2] ? "Y" : "N";
		r212_720p59_wr_ok->Text = writeSpeed >= r212_data_rates[3] ? "Y" : "N";
		r212_1080p23_wr_ok->Text = writeSpeed >= r212_data_rates[4] ? "Y" : "N";
		r212_1080p24_wr_ok->Text = writeSpeed >= r212_data_rates[5] ? "Y" : "N";
		r212_1080p29_wr_ok->Text = writeSpeed >= r212_data_rates[6] ? "Y" : "N";
		r212_1080p30_wr_ok->Text = writeSpeed >= r212_data_rates[7] ? "Y" : "N";
		r212_1080i50_wr_ok->Text = writeSpeed >= r212_data_rates[8] ? "Y" : "N";
		r212_1080i59_wr_ok->Text = writeSpeed >= r212_data_rates[9] ? "Y" : "N";
		r212_1080p50_wr_ok->Text = writeSpeed >= r212_data_rates[10] ? "Y" : "N";
		r212_1080p59_wr_ok->Text = writeSpeed >= r212_data_rates[11] ? "Y" : "N";
		r212_2k23_wr_ok->Text = writeSpeed >= r212_data_rates[12] ? "Y" : "N";
		r212_2k24_wr_ok->Text = writeSpeed >= r212_data_rates[13] ? "Y" : "N";
		r212_2k25_wr_ok->Text = writeSpeed >= r212_data_rates[14] ? "Y" : "N";

		v210_pal_wr_fps->Text = format_fps(writeSpeed / v210framesize(720,576));
		v210_ntsc_wr_fps->Text = format_fps(writeSpeed / v210framesize(720,486));
		v210_720_wr_fps->Text = format_fps(writeSpeed / v210framesize(1280,720));
		v210_1080_wr_fps->Text = format_fps(writeSpeed / v210framesize(1920,1080));
		v210_2k_wr_fps->Text = format_fps(writeSpeed / v210framesize(2048,1556));

		r210_pal_wr_fps->Text = format_fps(writeSpeed / r210framesize(720,576));
		r210_ntsc_wr_fps->Text = format_fps(writeSpeed / r210framesize(720,486));
		r210_720_wr_fps->Text = format_fps(writeSpeed / r210framesize(1280,720));
		r210_1080_wr_fps->Text = format_fps(writeSpeed / r210framesize(1920,1080));
		r210_2k_wr_fps->Text = format_fps(writeSpeed / r210framesize(2048,1556));

		r212_pal_wr_fps->Text = format_fps(writeSpeed / r212framesize(720,576));
		r212_ntsc_wr_fps->Text = format_fps(writeSpeed / r212framesize(720,486));
		r212_720_wr_fps->Text = format_fps(writeSpeed / r212framesize(1280,720));
		r212_1080_wr_fps->Text = format_fps(writeSpeed / r212framesize(1920,1080));
		r212_2k_wr_fps->Text = format_fps(writeSpeed / r212framesize(2048,1556));
	}

	if(readSpeed > 0)
	{
		int mb = static_cast<int>(readSpeed / (1024 * 1024));
		swprintf(buf, 20, L"READ: %d Mb/s", mb);
		this->rdlbl->Text = ref new String(buf);
		this->rdbar->Value = mb;

		v210_pal_rd_ok->Text = readSpeed >= v210_data_rates[0] ? "Y" : "N";
		v210_ntsc_rd_ok->Text = readSpeed >= v210_data_rates[1] ? "Y" : "N";
		v210_720p60_rd_ok->Text = readSpeed >= v210_data_rates[2] ? "Y" : "N";
		v210_720p59_rd_ok->Text = readSpeed >= v210_data_rates[3] ? "Y" : "N";
		v210_1080p23_rd_ok->Text = readSpeed >= v210_data_rates[4] ? "Y" : "N";
		v210_1080p24_rd_ok->Text = readSpeed >= v210_data_rates[5] ? "Y" : "N";
		v210_1080p29_rd_ok->Text = readSpeed >= v210_data_rates[6] ? "Y" : "N";
		v210_1080p30_rd_ok->Text = readSpeed >= v210_data_rates[7] ? "Y" : "N";
		v210_1080i50_rd_ok->Text = readSpeed >= v210_data_rates[8] ? "Y" : "N";
		v210_1080i59_rd_ok->Text = readSpeed >= v210_data_rates[9] ? "Y" : "N";
		v210_1080p50_rd_ok->Text = readSpeed >= v210_data_rates[10] ? "Y" : "N";
		v210_1080p59_rd_ok->Text = readSpeed >= v210_data_rates[11] ? "Y" : "N";
		v210_2k23_rd_ok->Text = readSpeed >= v210_data_rates[12] ? "Y" : "N";
		v210_2k24_rd_ok->Text = readSpeed >= v210_data_rates[13] ? "Y" : "N";
		v210_2k25_rd_ok->Text = readSpeed >= v210_data_rates[14] ? "Y" : "N";

		r210_pal_rd_ok->Text = readSpeed >= r210_data_rates[0] ? "Y" : "N";
		r210_ntsc_rd_ok->Text = readSpeed >= r210_data_rates[1] ? "Y" : "N";
		r210_720p60_rd_ok->Text = readSpeed >= r210_data_rates[2] ? "Y" : "N";
		r210_720p59_rd_ok->Text = readSpeed >= r210_data_rates[3] ? "Y" : "N";
		r210_1080p23_rd_ok->Text = readSpeed >= r210_data_rates[4] ? "Y" : "N";
		r210_1080p24_rd_ok->Text = readSpeed >= r210_data_rates[5] ? "Y" : "N";
		r210_1080p29_rd_ok->Text = readSpeed >= r210_data_rates[6] ? "Y" : "N";
		r210_1080p30_rd_ok->Text = readSpeed >= r210_data_rates[7] ? "Y" : "N";
		r210_1080i50_rd_ok->Text = readSpeed >= r210_data_rates[8] ? "Y" : "N";
		r210_1080i59_rd_ok->Text = readSpeed >= r210_data_rates[9] ? "Y" : "N";
		r210_1080p50_rd_ok->Text = readSpeed >= r210_data_rates[10] ? "Y" : "N";
		r210_1080p59_rd_ok->Text = readSpeed >= r210_data_rates[11] ? "Y" : "N";
		r210_2k23_rd_ok->Text = readSpeed >= r210_data_rates[12] ? "Y" : "N";
		r210_2k24_rd_ok->Text = readSpeed >= r210_data_rates[13] ? "Y" : "N";
		r210_2k25_rd_ok->Text = readSpeed >= r210_data_rates[14] ? "Y" : "N";

		r212_pal_rd_ok->Text = readSpeed >= r212_data_rates[0] ? "Y" : "N";
		r212_ntsc_rd_ok->Text = readSpeed >= r212_data_rates[1] ? "Y" : "N";
		r212_720p60_rd_ok->Text = readSpeed >= r212_data_rates[2] ? "Y" : "N";
		r212_720p59_rd_ok->Text = readSpeed >= r212_data_rates[3] ? "Y" : "N";
		r212_1080p23_rd_ok->Text = readSpeed >= r212_data_rates[4] ? "Y" : "N";
		r212_1080p24_rd_ok->Text = readSpeed >= r212_data_rates[5] ? "Y" : "N";
		r212_1080p29_rd_ok->Text = readSpeed >= r212_data_rates[6] ? "Y" : "N";
		r212_1080p30_rd_ok->Text = readSpeed >= r212_data_rates[7] ? "Y" : "N";
		r212_1080i50_rd_ok->Text = readSpeed >= r212_data_rates[8] ? "Y" : "N";
		r212_1080i59_rd_ok->Text = readSpeed >= r212_data_rates[9] ? "Y" : "N";
		r212_1080p50_rd_ok->Text = readSpeed >= r212_data_rates[10] ? "Y" : "N";
		r212_1080p59_rd_ok->Text = readSpeed >= r212_data_rates[11] ? "Y" : "N";
		r212_2k23_rd_ok->Text = readSpeed >= r212_data_rates[12] ? "Y" : "N";
		r212_2k24_rd_ok->Text = readSpeed >= r212_data_rates[13] ? "Y" : "N";
		r212_2k25_rd_ok->Text = readSpeed >= r212_data_rates[14] ? "Y" : "N";

		v210_pal_rd_fps->Text = format_fps(readSpeed / v210framesize(720,576));
		v210_ntsc_rd_fps->Text = format_fps(readSpeed / v210framesize(720,486));
		v210_720_rd_fps->Text = format_fps(readSpeed / v210framesize(1280,720));
		v210_1080_rd_fps->Text = format_fps(readSpeed / v210framesize(1920,1080));
		v210_2k_rd_fps->Text = format_fps(readSpeed / v210framesize(2048,1556));

		r210_pal_rd_fps->Text = format_fps(readSpeed / r210framesize(720,576));
		r210_ntsc_rd_fps->Text = format_fps(readSpeed / r210framesize(720,486));
		r210_720_rd_fps->Text = format_fps(readSpeed / r210framesize(1280,720));
		r210_1080_rd_fps->Text = format_fps(readSpeed / r210framesize(1920,1080));
		r210_2k_rd_fps->Text = format_fps(readSpeed / r210framesize(2048,1556));

		r212_pal_rd_fps->Text = format_fps(readSpeed / r212framesize(720,576));
		r212_ntsc_rd_fps->Text = format_fps(readSpeed / r212framesize(720,486));
		r212_720_rd_fps->Text = format_fps(readSpeed / r212framesize(1280,720));
		r212_1080_rd_fps->Text = format_fps(readSpeed / r212framesize(1920,1080));
		r212_2k_rd_fps->Text = format_fps(readSpeed / r212framesize(2048,1556));
	}
}

void DiskSpeedTest::MainPage::startTest()
{
	// capture the UI thread context
	auto ctx = concurrency::task_continuation_context::use_current();

	// initiate an async write test
	auto asyncTestWrite = concurrency::create_task([=] () { return this->testWrite(); });

	// on complete, update the UI in the UI thread
	asyncTestWrite.then([=](bool success)
	{
		if(success)
		{
			this->updateResult();
		}
	}, ctx);

	asyncTestWrite.then([=](bool success)
	{
		if(success)
		{
			// initiate an async read test after the write test
			auto asyncTestRead = concurrency::create_task([=] () { return this->testRead(); } );

			// when read test completes, update the UI, and restart the whole test.
			asyncTestRead.then([=](bool success)
			{
				if(success)
				{
					this->updateResult();
					this->startTest(); // restart! Note this is not a recursion.
				}
			}, ctx);
		}
	});
}

bool DiskSpeedTest::MainPage::testWrite()
{
	std::vector<byte> buf(kTestFrameSize);
	auto r = std::bind(std::uniform_int_distribution<int>(), std::default_random_engine());

	LARGE_INTEGER offset;
	offset.QuadPart = 0;
	auto startTime = GetTickCount64();

	if (!m_testing)
		return false;

	std::fill(buf.begin(), buf.end(), r());
	SetFilePointerEx(m_hfile, offset, NULL, FILE_BEGIN);
	DWORD written;
	if(!WriteFile(m_hfile, &buf[0], buf.size(), &written, NULL))
		return false;

	m_totalWriteBytes += written;
	m_totalWriteTime += GetTickCount64() - startTime;
	return true;
}

bool DiskSpeedTest::MainPage::testRead()
{
	std::vector<byte> buf(kTestFrameSize);
	LARGE_INTEGER offset;
	offset.QuadPart = 0;
	auto startTime = GetTickCount64();

	if (!m_testing)
		return false;

	SetFilePointerEx(m_hfile, offset, NULL, FILE_BEGIN);
	DWORD read;
	if(!ReadFile(m_hfile, &buf[0], buf.size(), &read, NULL))
		return false;

	m_totalReadBytes += read;
	m_totalReadTime += GetTickCount64() - startTime;
	return true;
}

void DiskSpeedTest::MainPage::startButton_Click_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!m_testing)
	{
		m_testing = true;
		m_totalWriteBytes = 0;
		m_totalWriteTime = 0;
		m_totalReadBytes = 0;
		m_totalReadTime = 0;
		this->startButton->Content = "Stop";
		startTest();
	}
	else
	{
		m_testing = false;
		this->startButton->Content = "Start";
	}
}
