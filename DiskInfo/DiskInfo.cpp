// DiskInfo.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <Windows.h>
#include <iostream>
#include "DiskInfo.h"
using namespace std;
#define  MAX_IDE_DRIVES  16
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

DiskInfo diskInfo;

DiskInfo::DiskInfo(void){	    
	this->ReadLogicalPartitionsInfo();
	this->ReadHardisksInfo();
}
int DiskInfo::ReadHardisksInfo (){    
	this->DestroyListHardisks();
	int drive = 0;
	HardiskInfo *hardiskInfo;    
	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDrive = 0;                
		char driveName [256];
		sprintf_s(driveName, "\\\\.\\PhysicalDrive%d", drive);		
		//Google xem CreateFile       
		WCHAR name[256];
		MultiByteToWideChar( 0,0, driveName, 255, name, 256);
			
		hPhysicalDrive = CreateFile ((LPCWSTR)name, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);		

		if (hPhysicalDrive != INVALID_HANDLE_VALUE)
		{
			////google xem struct STORAGE_PROPERTY_QUERY
			STORAGE_PROPERTY_QUERY query;
			DWORD cbBytesReturned = 0;
			char buffer[10000];
			memset ((void *) &query, 0, sizeof(query));
			query.PropertyId = StorageDeviceProperty;
			query.QueryType = PropertyStandardQuery;
			memset (buffer, 0, sizeof (buffer));
			//Google xem DeviceIoControl
			if ( DeviceIoControl (hPhysicalDrive, IOCTL_STORAGE_QUERY_PROPERTY,
				&query,
				sizeof(query),
				&buffer,
				sizeof(buffer),
				&cbBytesReturned, NULL) )
			{         
				//google xem struct STORAGE_DEVICE_DESCRIPTOR
				STORAGE_DEVICE_DESCRIPTOR * descrip = (STORAGE_DEVICE_DESCRIPTOR *) & buffer;
				char serialNumber[1000];
				char modelNumber[1000];
				char vendorId[1000];                
				//Lay cac thong tin mo ta cua o cung
				getString (buffer, descrip -> VendorIdOffset, vendorId );
				getString (buffer, descrip -> ProductIdOffset, modelNumber );                
				getString (buffer, descrip -> SerialNumberOffset, serialNumber );                
				//Them mot doi tuong o cung vao danh sach voi cac thong tin mo ta
				hardiskInfo = new HardiskInfo;
				hardiskInfo->vendorId = string(vendorId);
				hardiskInfo->productId = string(modelNumber);                
				hardiskInfo->serialNumber = string(serialNumber);
				hardiskInfo->bytePerSector = 0;
				hardiskInfo->cylinders = 0;
				hardiskInfo->diskSize = 0;
				hardiskInfo->tracksPerCylinder = 0;
				hardiskInfo->sectorsPerTrack = 0;
				hardiskInfo->driverType = -1;
				listHardiskInfo.push_back(hardiskInfo);

				// Lay thong tin chi tiet cua o cung
				memset (buffer, 0, sizeof(buffer));
				if ( DeviceIoControl (hPhysicalDrive,
					IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
					NULL,
					0,
					&buffer,
					sizeof(buffer),
					&cbBytesReturned,
					NULL))
				{
					//google xem struct DISK_GEOMETRY_EX
					DISK_GEOMETRY_EX* geom = (DISK_GEOMETRY_EX*) &buffer;
					int fixed = (geom->Geometry.MediaType == FixedMedia);
					__int64 size = geom->DiskSize.QuadPart;				     
					//Cap nhap thong tin chi tiet
					hardiskInfo->bytePerSector = geom->Geometry.BytesPerSector;                    
					hardiskInfo->diskSize = geom->DiskSize.QuadPart;
					hardiskInfo->tracksPerCylinder = geom->Geometry.TracksPerCylinder;
					hardiskInfo->sectorsPerTrack = geom->Geometry.SectorsPerTrack;
					hardiskInfo->driverType = geom->Geometry.MediaType;
					hardiskInfo->cylinders = geom->Geometry.Cylinders.QuadPart;                    
				}
			}            
			CloseHandle (hPhysicalDrive);
		}        
	}    
	return drive;
}
int DiskInfo::ReadLogicalPartitionsInfo(){
    this->DestroyListLogicalPartitions();
    const int lenght = 255;
    char buffer[lenght + 1];
	WCHAR names[256];	
    //Lay danh sach cac o dia logical
::GetLogicalDriveStrings(lenght, names);	
    //Convert WCHAR* to char*
	for (int i = 0; i < 256; i++){
		buffer[i] = names[i];
	}
	char *s = buffer;
    //Khoi tao danh sach cac o dia logical
    while (*s)
    {
        LogicalPartitionInfo *volumeInfo = new LogicalPartitionInfo;
        volumeInfo->rootPathName = s;
        listLogicalPartitionInfo.push_back(volumeInfo);
        s += strlen(s) + 1;
    }    		
    //Lay thong tin chi tiet cua moi o dia logical
    WCHAR VolumeNameBuffer[256], FileSystemBuffer[256];
    for (unsigned int i = 0; i < listLogicalPartitionInfo.size(); i++)
    {        
        memset( VolumeNameBuffer, 0, 256*sizeof(WCHAR));
        memset( FileSystemBuffer, 0, 256*sizeof(WCHAR));		
		unsigned long bytesPerSector = 0;
		unsigned long sectorsPerCluster = 0;
		unsigned long numberOfFreeClusters = 0;
		unsigned long totalNumberOfClusters = 0;
		unsigned long volumeSerialNumber = 0;
		unsigned long maximumComponentLength = 0;
		unsigned long fileSystemFlags = 0;
		int driverType;

		WCHAR rootPath[5];
		MultiByteToWideChar( 0,0, listLogicalPartitionInfo[i]->rootPathName.data(), 5, rootPath, 5);
				
		driverType =::GetDriveType((LPCWSTR)rootPath);        
		//Google xem GetDiskFreeSpace
    ::GetDiskFreeSpace(
            (LPCWSTR)rootPath, 
            &sectorsPerCluster, 
            &bytesPerSector, 
            &numberOfFreeClusters, 
            &totalNumberOfClusters
            );
            //Google xem VolumeInfomation
    ::GetVolumeInformation(
            (LPCWSTR)rootPath,
            (LPWSTR)VolumeNameBuffer,
            256,
            &volumeSerialNumber,
            &maximumComponentLength,
            &fileSystemFlags,
            (LPWSTR)FileSystemBuffer,
            256                
            );		
		//Convert WCHAR * to char *
		char volumName[256], fileSystem[256];
		for(int i = 0; i < 256; i++){
			volumName[i] = VolumeNameBuffer[i];
			fileSystem[i] = FileSystemBuffer[i];
		}
		listLogicalPartitionInfo[i]->type = driverType;
		listLogicalPartitionInfo[i]->numberOfFreeClusters = numberOfFreeClusters;
		listLogicalPartitionInfo[i]->sectorsPerCluster = sectorsPerCluster;
		listLogicalPartitionInfo[i]->totalNumberOfClusters = totalNumberOfClusters;
		listLogicalPartitionInfo[i]->volumeSerialNumber = volumeSerialNumber;
		listLogicalPartitionInfo[i]->rootPathName.data();
        listLogicalPartitionInfo[i]->volumeName = string(volumName);
        listLogicalPartitionInfo[i]->fileSystemName = string(fileSystem);    
		listLogicalPartitionInfo[i]->size = bytesPerSector * sectorsPerCluster * (long long) totalNumberOfClusters;

		
    }
    return this->listLogicalPartitionInfo.size();
}


char * DiskInfo::getString (const char * str, int pos, char * buf)
{
	buf [0] = 0;
	if (pos <= 0)
		return buf;

	int i = pos;
	int j = 1;
	int k = 0;    
	//Tach xau
	while (j && str[i] != 0)
	{
		char c = str[i];
		if ( ! isprint(c))
		{
			j = 0;
			break;
		}
		buf[k] = c;
		++k;
		++i;
	}        

	// Neu khong co ky tu nao in duoc
	if ( ! j )            
		k = 0;    

	buf[k] = 0;

	// Trim: Loai bo khoang trang dau va cuoi xau    
	while (isspace(buf[0]) && buf[0] != 0)
	{   
		i = 0;
		while ( buf[i] != 0)      
		{
			buf[i] = buf[i+1];
			i++;
		}
	}
	if (strlen((buf)) > 1)
		while (isspace(buf[strlen(buf) - 1]))
		{   
			buf[strlen(buf) - 1] = 0;
		}

		return buf;
}

void DiskInfo::DestroyListLogicalPartitions()
{
    while (!listLogicalPartitionInfo.empty())
    {
        delete listLogicalPartitionInfo.back();
        listLogicalPartitionInfo.pop_back();
    }
}

void DiskInfo::DestroyListHardisks()
{
    while (!listHardiskInfo.empty())
    {
        delete listHardiskInfo.back();
        listHardiskInfo.pop_back();
    }
}

void DiskInfo::DestroyAll()
{
    this->DestroyListHardisks();
    this->DestroyListLogicalPartitions();
}

DiskInfo::~DiskInfo()
{
   this->DestroyAll();
}

ListHardiskInfo DiskInfo::getHDisks()
{
	return this->listHardiskInfo;	
}

ListLogicalPartitionInfo DiskInfo::getPartitions()
{
	return this->listLogicalPartitionInfo;
}
int _tmain(int argc, _TCHAR* argv[])
{	
	DiskInfo d;
	ListLogicalPartitionInfo l = d.getPartitions();
	for (int i=0;i<l.size();i++){  
		cout << "Loai: " << l[i] ->type << endl;
		cout << "So cluster trong: " <<l[i] ->numberOfFreeClusters << endl;
		cout << "So sector tren moi cluster: " <<l[i] ->sectorsPerCluster << endl;
		cout << "Tong so cluster : " <<l[i] ->totalNumberOfClusters << endl;
		cout << "volumeSerialNumber : " <<l[i] ->volumeSerialNumber << endl;
		cout << "Thu muc goc : " <<l[i] ->rootPathName.data() << endl;
		cout << "Ten nhan : " <<l[i] ->volumeName << endl;
		cout << "Dinh dang : " <<l[i] ->fileSystemName << endl;
		cout << "Tong dung luong : " << (l[i] ->size)/1073741824 << "GB" << endl;
		cout << "================================" << endl;
	}
	cout << "=============================================================================" << endl;
	ListHardiskInfo ld = d.getHDisks();
	for (int i=0;i<ld.size();i++){  
		cout << "Nha san xuat: " << ld[i] ->productId << endl;
		cout << "So serial: " << ld[i] ->serialNumber << endl;
		cout << "So byte tren moi sector: " << ld[i] ->bytePerSector << endl;
		cout << "Kich thuoc: " <<(ld[i] ->diskSize)/1073741824 << "GB" << endl;
		cout << "So track tren moi clinder: " <<ld[i] ->tracksPerCylinder << endl;
		cout << "So sector tren moi track : " <<ld[i] ->sectorsPerTrack << endl;
		cout << "Loai dia : " << ld[i] ->driverType << endl;
		cout << "So clinder : " <<ld[i] ->cylinders << endl;
		cout << "================================" << endl;
	}    
}
