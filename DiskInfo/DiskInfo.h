#ifndef Disk_Info_h_
#define Disk_Info_h_

#include <vector>
#include <string>
using std::vector;
using std::string;

struct LogicalPartitionInfo
{
    string rootPathName, volumeName, fileSystemName;
    unsigned long sectorsPerCluster, numberOfFreeClusters, totalNumberOfClusters;
    unsigned long volumeSerialNumber;
	long long size;
    int type;
};
struct HardiskInfo 
{
    string vendorId, productId, serialNumber;
    unsigned long bytePerSector, sectorsPerTrack, tracksPerCylinder;
    long long diskSize, cylinders;
    int driverType; 
};
typedef vector<LogicalPartitionInfo *> ListLogicalPartitionInfo;
typedef vector<HardiskInfo *> ListHardiskInfo;
class DiskInfo
{
private:
	ListLogicalPartitionInfo listLogicalPartitionInfo;
    ListHardiskInfo listHardiskInfo;
    char* getString (const char * str, int pos, char * buf);    
    void DestroyListLogicalPartitions();
    void DestroyListHardisks();
public:    
    int ReadHardisksInfo();
    int ReadLogicalPartitionsInfo();
    void DestroyAll();
	ListHardiskInfo getHDisks();
	ListLogicalPartitionInfo getPartitions();
	DiskInfo();
	virtual ~DiskInfo();
};


#endif
