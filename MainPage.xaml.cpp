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

int computeSpeed(long long bytes, long long ms)
{
	if(ms == 0)
		return 0;
	return static_cast<int>(bytes * 1000 / ms / (1024 * 1024));
}

void DiskSpeedTest::MainPage::updateResult()
{
	int writeSpeed = computeSpeed(m_totalWriteBytes, m_totalWriteTime);
	int readSpeed = computeSpeed(m_totalReadBytes, m_totalReadTime);

	wchar_t buf[20];
	swprintf(buf, 20, L"WRITE: %d Mb/s", writeSpeed);
	this->wrlbl->Text = ref new String(buf);
	this->wrbar->Value = writeSpeed;

	swprintf(buf, 20, L"READ: %d Mb/s", readSpeed);
	this->rdlbl->Text = ref new String(buf);
	this->rdbar->Value = readSpeed;
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
