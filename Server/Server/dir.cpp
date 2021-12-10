#include "dir.h"
namespace fs = std::experimental::filesystem::v1;

std::vector<std::string> getListOfDrives() {
	std::vector<std::string> arrayOfDrives;
	char* szDrives = new char[MAX_PATH]();
	if (GetLogicalDriveStringsA(MAX_PATH, szDrives));
	for (int i = 0; i < 100; i += 4)
		if (szDrives[i] != (char)0)
			arrayOfDrives.push_back(std::string{ szDrives[i],szDrives[i + 1],szDrives[i + 2] });
	delete[] szDrives;
	return arrayOfDrives;
}

ObjFS getDir(std::string path, bool disksOrFolders) {
	std::string arr[1000];

	//If disks
	if (disksOrFolders) {
		std::vector<std::string> drives = getListOfDrives();
		int iter = 0;
		for (std::string currentDrive : drives) {
			arr[iter] = currentDrive;
			iter++;
		}

		return ObjFS(arr, iter, 1);
	}

	//If file or folder
	if (!(GetFileAttributesA(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) {
		return ObjFS(arr, 0, 0);
	}

	int iter = 0;
	for (const auto& entry : fs::directory_iterator(path)) {
		arr[iter] = entry.path().u8string();
		iter++;
	}

	return ObjFS(arr, iter, 1);
}