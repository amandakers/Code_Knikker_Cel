#include <iostream>
#include "modbus.h"
#include "GalaxyIncludes.h"
#include "stdafx.h"
#include <fstream>

/*
int main() {
    modbus_t* ctx = modbus_new_tcp("127.0.0.1", 502); // lokale verbinding
    if (!ctx) return -1;

    if (modbus_connect(ctx) == -1) {
        std::cerr << "Verbinden mislukt: " << modbus_strerror(errno) << "\n";
        modbus_free(ctx);
        return -1;
    }

    int reg_address = 0; // %MW0
    int value = 333;

    if (modbus_write_register(ctx, reg_address, value) == -1)
        std::cerr << "Schrijven mislukt: " << modbus_strerror(errno) << "\n";
    else
        std::cout << "Geschreven waarde " << value << " naar register " << reg_address << "\n";

    uint16_t tab_reg[1];
    if (modbus_read_registers(ctx, reg_address, 1, tab_reg) == -1)
        std::cerr << "Lezen mislukt: " << modbus_strerror(errno) << "\n";
    else
        std::cout << "Gelezen waarde: " << tab_reg[0] << "\n";

    modbus_close(ctx);
    modbus_free(ctx);
    //test
    return 0;
}
*/

using namespace std;

struct THREAD_PARAM
{
	bool bRun;
	CGXStreamPointer pStream;
};

// Thread Function
DWORD WINAPI GrabImgThread(LPVOID lpParam)
{
	THREAD_PARAM* pThreadParam = (THREAD_PARAM*)lpParam;
	if (NULL == pThreadParam)
	{
		cout << "lpParam is NULL!" << endl;
		system("pause");
		return 0;
	}
	modbus_t* ctx = modbus_new_tcp("127.0.0.1", 502); // lokale verbinding
	if (!ctx) return -1;

	if (modbus_connect(ctx) == -1) {
		std::cerr << "Verbinden mislukt: " << modbus_strerror(errno) << "\n";
		modbus_free(ctx);
		std::cin.get();
		return -1;
	}

	while (pThreadParam->bRun)
	{
		try
		{
			CImageDataPointer pImgData = pThreadParam->pStream->GetImage(2000);
			if (GX_FRAME_STATUS_SUCCESS == pImgData->GetStatus())
			{
				//After acquiring the image, obtain the frame attribute control object to obtain the frame data
				IGXFeatureControl* pChunkDataFeatureControl = pImgData->GetChunkDataFeatureControl();
				int64_t i64ChunkFID = pChunkDataFeatureControl->GetIntFeature("ChunkFrameID")->GetValue();

				cout << "<Successful acquisition: Width: " << pImgData->GetWidth() <<
					" Height: " << pImgData->GetHeight() << " i64ChunkFID: " << i64ChunkFID <<
					">" << endl;


				void* pBuffer = pImgData->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, true);
				unsigned char* rgbData = (unsigned char*)pBuffer;

				int width = pImgData->GetWidth();
				int height = pImgData->GetHeight();

				uint64_t sumR = 0, sumG = 0, sumB = 0;
				unsigned char minR = 255, minG = 255, minB = 255;

				// Histogram arrays (256 mogelijke intensiteiten per kanaal)
				std::vector<int> histR(256, 0);
				std::vector<int> histG(256, 0);
				std::vector<int> histB(256, 0);

				//data naar tesktbestand
				//std::ofstream file("RGB-data.txt", std::ios::app);

				for (int y = 0; y < height; y++)
				{
					for (int x = 0; x < width; x++)
					{
						int index = (y * width + x) * 3;
						unsigned char R = rgbData[index + 0];
						unsigned char G = rgbData[index + 1];
						unsigned char B = rgbData[index + 2];

						sumR += R;
						sumG += G;
						sumB += B;

						if (R < minR) minR = R;
						if (G < minG) minG = G;
						if (B < minB) minB = B;

						// histogram vullen
						histR[R]++;
						histG[G]++;
						histB[B]++;
					}
				}

				int totalPixels = width * height;
				int avgR = sumR / totalPixels;
				int avgG = sumG / totalPixels;
				int avgB = sumB / totalPixels;

				// Piek (modus) zoeken
				int peakR = std::distance(histR.begin(), std::max_element(histR.begin(), histR.end()));
				int peakG = std::distance(histG.begin(), std::max_element(histG.begin(), histG.end()));
				int peakB = std::distance(histB.begin(), std::max_element(histB.begin(), histB.end()));

				int countR = histR[peakR];
				int countG = histG[peakG];
				int countB = histB[peakB];

				// percentages berekenen
				double percAtPeakR = 100.0 * countR / totalPixels;
				double percAtPeakG = 100.0 * countG / totalPixels;
				double percAtPeakB = 100.0 * countB / totalPixels;

				cout << "Average RGB = (" << avgR << ", " << avgG << ", " << avgB << ")" << std::endl;
				cout << "Minimum RGB = (" << (int)minR << ", " << (int)minG << ", " << (int)minB << ")" << std::endl;

			//	cout << "Peak RGB values = (" << peakR << ", " << peakG << ", " << peakB << ")" << std::endl;

				//file << "test average: (" << avgR << ", " << avgG << ", " << avgB << ")";
				//file << "test minimum = (" << (int)minR << ", " << (int)minG << ", " << (int)minB << ")";
				//file << "test R%:" << percAtPeakR << "%";
				//file.close();

				cout << "Percentage pixels bij piek: "
					<< "R=" << percAtPeakR << "%, "
					<< "G=" << percAtPeakG << "%, "
					<< "B=" << percAtPeakB << "%" << std::endl;

				
				//int reg_address = 0; // %MW0
			//	int value = avgR;

				int reg_address = 0; 
				uint16_t values[9];  

				
				values[0] = avgR;
				values[1] = avgG;
				values[2] = avgB;
				values[3] = minR;
				values[4] = minG;
				values[5] = minB;
				values[6] = percAtPeakR;
				values[7] = percAtPeakG;
				values[8] = percAtPeakB;

				int num_registers = 9;


				if (modbus_write_registers(ctx, reg_address, num_registers, values) == -1)
					std::cerr << "Schrijven mislukt: " << modbus_strerror(errno) << "\n";
			//else
			//	std::cout << "Geschreven waarde " << values << " naar register " << reg_address << "\n";
			}
			else
			{
				cout << "<Abnormal Acquisition: Exception code: " << pImgData->GetStatus() << ">" << endl;
			}

			//Sleep(100);
		}
		catch (CGalaxyException& e)
		{
			cout << "<" << e.GetErrorCode() << ">" << "<" << e.what() << ">" << endl;
		}
		catch (std::exception& e)
		{
			cout << "<" << e.what() << ">" << endl;
		}

	}

	cout << "<Acquisition thread Exit!>" << endl;
	return 0;
}

int main(int argc, char* argv[])
{
	
	try
	{
		//Initialize the device library
		IGXFactory::GetInstance().Init();

		//Enumerate camera devices
		GxIAPICPP::gxdeviceinfo_vector vectorDeviceInfo;
		IGXFactory::GetInstance().UpdateDeviceList(1000, vectorDeviceInfo);

		//Determine the number of current device connections
		if (vectorDeviceInfo.size() <= 0)
		{
			cout << "No device!" << endl;
			system("pause");
			return 0;
		}

		//Open the camera device via SN
		CGXDevicePointer pDevice = IGXFactory::GetInstance().OpenDeviceBySN(vectorDeviceInfo[0].GetSN(), GX_ACCESS_EXCLUSIVE);
		//Get the camera property control object
		CGXFeatureControlPointer pRemoteControl = pDevice->GetRemoteFeatureControl();
		//Stream layer objects
		CGXStreamPointer pStream;
		if (pDevice->GetStreamCount() > 0)
		{
			pStream = pDevice->OpenStream(0);
		}
		else
		{
			cout << "Not find stream!";
			system("pause");
			return 0;
		}

		//Select the default parameter group
		//pRemoteControl->GetEnumFeature("UserSetSelector")->SetValue("Default");
		//Load parameter group
		//pRemoteControl->GetCommandFeature("UserSetLoad")->Execute();
		pRemoteControl->GetIntFeature("Width")->SetValue(640);
		pRemoteControl->GetIntFeature("Height")->SetValue(480);
		pRemoteControl->GetIntFeature("OffsetX")->SetValue(80);
		pRemoteControl->GetIntFeature("OffsetY")->SetValue(50);

		//Cameras that do not support frame information do not have the ChunkModeActive function, so they cannot output frame information, so the program ends;
		if (!pRemoteControl->IsImplemented("ChunkModeActive") || !pRemoteControl->IsWritable("ChunkModeActive"))
		{
			cout << "ChunkData is not supported, App exit!\n";
			system("pause");
			return 0;
		}

		pRemoteControl->GetBoolFeature("ChunkModeActive")->SetValue(true);

		//USB interface camera, each frame information item has its own switch, which can be turned on as needed. This sample program only displays the real FrameID;
		//Gev interface camera, currently there is no item switch, only the CHunkModeActive master switch;
		if (pRemoteControl->IsImplemented("ChunkSelector") && pRemoteControl->IsWritable("ChunkSelector"))
		{
			pRemoteControl->GetEnumFeature("ChunkSelector")->SetValue("FrameID");
			if (pRemoteControl->IsImplemented("ChunkEnable") && pRemoteControl->IsWritable("ChunkEnable"))
			{
				pRemoteControl->GetBoolFeature("ChunkEnable")->SetValue(true);
			}
		}


		cout << "***********************************************" << endl;
		cout << "<Vendor Name:   " << pDevice->GetDeviceInfo().GetVendorName() << ">" << endl;
		cout << "<Model Name:    " << pDevice->GetDeviceInfo().GetModelName() << ">" << endl;
		cout << "<Serial Number: " << pDevice->GetDeviceInfo().GetSN() << ">" << endl;
		cout << "***********************************************" << endl << endl;

		cout << "Press [a] or [A] and then press [Enter] to start acquisition" << endl;
		cout << "Press [x] or [X] and then press [Enter] to Exit the Program" << endl << endl;

		char startKey = 0;
		bool bWaitStart = true;
		bool bGrab = false;
		while (bWaitStart)
		{
			startKey = getchar();
			switch (startKey)
			{
				//press 'a' and [Enter] to start acquisition
				//press 'x' and [Enter] to exit
			case 'a':
			case 'A':
				//Start to acquisition
				bWaitStart = false;
				bGrab = true;
				break;
			case 'x':
			case 'X':
				//App exit
				cout << "<App exit!>" << endl;
				system("pause");
				return 0;
			default:
				break;
			}
		}

		//Start flow layer acquisition
		pStream->StartGrab();
		//Start camera acquisition
		pRemoteControl->GetCommandFeature("AcquisitionStart")->Execute();

		THREAD_PARAM threadParam;
		threadParam.bRun = true;
		threadParam.pStream = pStream;

		// Create a collection thread
		HANDLE hThread = CreateThread(NULL, 0, GrabImgThread, &threadParam, 0, NULL);
		if (hThread == NULL)
		{
			cerr << "Failed to create thread." << std::endl;
			system("pause");
			return 0;
		}

		//Judgment Exit
		bWaitStart = true;
		while (bWaitStart)
		{
			char strKey = getchar();
			//press 'x' and [Enter] for exit
			switch (strKey)
			{
			case 'X':
			case 'x':
				bWaitStart = false;
				break;
			default:
				break;
			}
		}

		//Wait for the collection thread to exit
		threadParam.bRun = false;
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);

		if (bGrab)
		{
			//The camera stops collecting
			pRemoteControl->GetCommandFeature("AcquisitionStart")->Execute();
			//Flow layer stops collecting
			pStream->StopGrab();
		}

		//Closing a stream
		pStream->Close();
		//Turn off the camera device
		pDevice->Close();
		//Close the device library
		IGXFactory::GetInstance().Uninit();
	}
	catch (CGalaxyException& e)
	{
		cout << "<" << e.GetErrorCode() << ">" << "<" << e.what() << ">" << endl;
	}
	catch (std::exception& e)
	{
		cout << "<" << e.what() << ">" << endl;
	}

	cout << "<App exit!>" << endl;
	system("pause");

	return 0;
}
