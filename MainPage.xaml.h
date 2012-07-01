//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include <windows.h>

namespace DiskSpeedTest
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		~MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		volatile bool m_testing;
		HANDLE m_hfile;

		long long m_totalWriteBytes;
		long long m_totalWriteTime;
		long long m_totalReadBytes;
		long long m_totalReadTime;

		void startTest();
		bool testRead();
		bool testWrite();

		void updateResult();

		void startButton_Click_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
