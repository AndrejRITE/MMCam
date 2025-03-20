#pragma once
#ifndef XERYONMOTOR_H
#define XERYONMOTOR_H

#include <memory>
#include <algorithm>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>

#include "IMotor.h"
#include "IMotorArray.h"

class XeryonMotor final 
{
};

class XeryonMotorArray final : public IMotorArray
{
public:
	XeryonMotorArray() {};
	~XeryonMotorArray() {};

};
#endif
