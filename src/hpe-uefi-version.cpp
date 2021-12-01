/*
// Copyright (c) 2021 Hewlett-Packard Development Company, L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream, std::stringbuf

#include "uefi.h"

std::string get_hpe_uefi_firmware_version(void *hpe_uefi_build_manifest_buffer, size_t file_size)
{
	std::string version = "null";	// default on not found
	char *cbuf = reinterpret_cast<char *>(hpe_uefi_build_manifest_buffer); // operate on this as text
	cbuf[file_size-1] = 0;		// safety backstop for string ops

	// Safety:  terminate string at 1st character >= 128 ASCII (typically 0xFF padding)
	// We are expecting only ASCII followed by 0xFF padding
	unsigned int offset = 0;
	for (offset = 0; offset < file_size && cbuf[offset] < 127; offset++);
	cbuf[offset ? offset-1 : 0] = 0;	// could be empty string if malformed

	// convert to std::string and parse out the ROM_VER_STR value (= to end of line)
	std::string manifest = cbuf;
	size_t offsetRomVerStr = manifest.find("ROM_VER_STR=");
	if (offsetRomVerStr != std::string::npos)
	{
		std::string stmp1 = manifest.substr(offsetRomVerStr);
		size_t offsetEndOfLine = stmp1.find('\r');
		if (offsetEndOfLine != std::string::npos)
		{
			const size_t sizeROMVERKEY = 12;	// size of "ROM_VER_STR="
			version = stmp1.substr(sizeROMVERKEY, offsetEndOfLine-sizeROMVERKEY);	// assign trimmed value
		}
	}
	return version;
}

//
// Usage:  1st argument is the path name to the UEFI BIOS image file (e.g. could be /dev/mtdx, but could also be just a file)
//
// This outputs the version string to stdout, e.g.
// 	root@dl360poc-08f1ea71586c:/usr/bin# hpe-uefi-version /dev/mtd2
//	Z02 v1.00 (07/20/2920)
// If you set the output to an env var:
//   foo="$(hpe-uefi-version /dev/mtd2)"
// You can then use busctl to set the BIOS version property:
//   busctl set-property xyz.openbmc_project.Software.BMC.Updater /xyz/openbmc_project/software/bios_active xyz.openbmc_project.Software.Version Version s "$foo"
//
int main(int argc, char **argv)
{
	size_t size;
	std::string mtd_filepath;
	std::string version;

	// 1st arg is the path to the MTD file
	if (argc != 2 || argv == NULL)
	{
		printf("USAGE:  dump_rom <mtd filepath>\n");
		return (-1);
	}

	mtd_filepath = argv[1];
	//printf("Parsing %s\n", mtd_filepath.c_str());

	UEFI::UEFIFirmwareImage hpe_uefi_firmware(mtd_filepath);
	//hpe_uefi_firmware.dumpRomFV();
	//printf("================================================= \n");

	void *hpe_uefi_build_manifest_buffer = hpe_uefi_firmware.getFileContent(HPE_UEFI_BUILD_MANIFEST_FV);
	if (hpe_uefi_build_manifest_buffer != NULL)
	{
		size = hpe_uefi_firmware.getFileSize(HPE_UEFI_BUILD_MANIFEST_FV);
		if (size > 0)
			//printf("Version: %s\n", get_hpe_uefi_firmware_version(hpe_uefi_build_manifest_buffer, size).c_str());
			printf("%s", get_hpe_uefi_firmware_version(hpe_uefi_build_manifest_buffer, size).c_str());
	}
}
