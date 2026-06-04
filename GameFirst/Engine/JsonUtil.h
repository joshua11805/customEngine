#pragma once
#include "EngineMath.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

// These are helper functions to make it easy to load something from a json file

bool GetFloatFromJSON(const rapidjson::Value& inObject, const char* inProperty, float& outFloat);
bool GetIntFromJSON(const rapidjson::Value& inObject, const char* inProperty, int& outInt);
bool GetUintFromJSON(const rapidjson::Value& inObject, const char* inProperty, uint32_t& outInt);
bool GetStringFromJSON(const rapidjson::Value& inObject, const char* inProperty, std::string& outStr);
bool GetBoolFromJSON(const rapidjson::Value& inObject, const char* inProperty, bool& outBool);
bool GetVectorFromJSON(const rapidjson::Value& inObject, const char* inProperty, Vector3& outVector);
bool GetQuaternionFromJSON(const rapidjson::Value& inObject, const char* inProperty, Quaternion& outQuat);
