#pragma once
#include <iostream>
#include <Windows.h>
#include <fileapi.h>
#include <string>
#define  _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

struct ObjFS {
	std::string paths[1000];
	int elements = 0;
	bool type = 1; //0 - file, 1 - folder

	ObjFS() {};

	ObjFS(std::string paths_[1000], int elements_, bool type_) {
		std::copy(paths_, paths_ + elements_, paths);
		elements = elements_;
		type = type_;
	};
};

ObjFS getDir(std::string path, bool disksOrFolders);

