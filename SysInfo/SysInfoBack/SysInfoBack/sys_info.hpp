#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <locale>
#include <codecvt>
#include <exception>

#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

#include <Iphlpapi.h>
#pragma comment(lib, "Iphlpapi.lib")

#include <Setupapi.h>
#pragma comment(lib, "Setupapi.lib")

#include <nlohmann/json.hpp>
#include <grpcpp/grpcpp.h>
#include "channel.grpc.pb.h"


using std::string;
using std::wstring;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using nlohmann::json;


class SysInfo final : public channel::ChannelService::Service
{
public:
	int run();

private:
	string wstringToString(const wstring& wstr);
	string ConvertToMultibyte(WCHAR* array);


	// Методы собирающие информацию
	int GetProcessList(string& data);
	int GetDiskInfo(string& data);
	int GetNetworkActivity(string& data);
	int GetDeviceInfo(string& data);

	grpc::Status ProcessList(grpc::ServerContext* context, const google::protobuf::Empty* request, channel::Response* response) override;
	grpc::Status DiskInfo(grpc::ServerContext* context, const google::protobuf::Empty* request, channel::Response* response) override;
	grpc::Status NetworkActivity(grpc::ServerContext* context, const google::protobuf::Empty* request, channel::Response* response) override;
	grpc::Status DeviceInfo(grpc::ServerContext* context, const google::protobuf::Empty* request, channel::Response* response) override;
};
