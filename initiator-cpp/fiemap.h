#pragma once

#include <string>
#include <vector>

#include "extent.h"

int getExtents(std::string &file_path, std::vector<Extent *> &vec);
