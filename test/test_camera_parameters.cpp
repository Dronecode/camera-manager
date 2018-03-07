/*
 * This file is part of the Camera Streaming Daemon project
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <algorithm>
#include <bitset>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "CameraParameters.h"
#include "log.h"
#include "util.h"

CameraParameters camParam;
void print_param_type(void);
void print_usage(void);
void test_convert_logic(uint32_t value);
void input_param_id(std::string &paramID);
void input_param_value(CameraParameters::cam_param_union_t &u);
int set(std::string paramID);
void get(std::string paramID, int type);

void print_param_type()
{
    // std::cout << "PARAM_TYPE_UINT8  : 1" << std::endl;
    // std::cout << "PARAM_TYPE_INT8   : 2" << std::endl;
    // std::cout << "PARAM_TYPE_UINT16 : 3" << std::endl;
    // std::cout << "PARAM_TYPE_INT16  : 4" << std::endl;
    std::cout << "PARAM_TYPE_UINT32 : 5" << std::endl;
    // std::cout << "PARAM_TYPE_INT32  : 6" << std::endl;
    // std::cout << "PARAM_TYPE_UINT64 : 7" << std::endl;
    // std::cout << "PARAM_TYPE_UINT64 : 8" << std::endl;
    std::cout << "PARAM_TYPE_REAL32 : 9" << std::endl;
    // std::cout << "PARAM_TYPE_REAL64 : 10" << std::endl;

    return;
}

void print_usage()
{
    std::cout << "0:  EXIT" << std::endl;
    std::cout << "1:  Test Set-Get" << std::endl;
    std::cout << "2:  Set Param" << std::endl;
    std::cout << "3:  Get Param" << std::endl;
    std::cout << "99: Test uint32<->string conversion" << std::endl;
    return;
}

// uint32(data type) -> byte array(to transmit) -> string(to store) -> byte array -> uint32
void test_convert_logic(uint32_t value)
{
    log_debug("Set DataType value: %d", value);

    // 1. Convert type uint32 to byte array
    CameraParameters::cam_param_union_t u;
    u.param_uint32 = value;
    log_debug("Set ByteArray value:");
    for (int i = 0; i < CAM_PARAM_VALUE_LEN; i++)
        printf("0x%.2x ", u.bytes[i]);
    printf("\n");

    // 2. Convert byte array to string
    std::string str(reinterpret_cast<char const *>(u.bytes), CAM_PARAM_VALUE_LEN);
    log_debug("Stored as String Object");

    // 3. Convert string to byte array
    // const char *arr2 = str.c_str();
    uint8_t arr[CAM_PARAM_VALUE_LEN];
    mem_cpy(arr, sizeof(arr), str.data(), str.size(), sizeof(arr));
    log_debug("Get ByteArray value:");
    for (int i = 0; i < CAM_PARAM_VALUE_LEN; i++)
        printf("0x%.2x ", arr[i]);
    printf("\n");

    // 4. Convert byte array to type uint32
    CameraParameters::cam_param_union_t u2;
    mem_cpy(u2.bytes, sizeof(u2.bytes), arr, sizeof(arr), sizeof(u2.bytes));
    log_debug("Get DataType value: %d", u2.param_uint32);
    if (u2.param_uint32 != u.param_uint32)
        log_error("Error in logic:%d", value);
}

void input_param_id(std::string &paramID)
{
    std::cout << "Enter Parameter ID String" << std::endl;
    std::cin >> paramID;
}

void input_param_value(CameraParameters::cam_param_union_t &u)
{
    std::cout << "Enter Parameter Type" << std::endl;
    print_param_type();
    scanf("%hhu", &u.type);
    std::cout << "Enter Parameter Value ";
    switch (u.type) {
    case CameraParameters::PARAM_TYPE_REAL32:
        std::cout << "Float :" << std::endl;
        std::cin >> u.param_float;
        break;
    case CameraParameters::PARAM_TYPE_UINT32:
        std::cout << "Uint32 :" << std::endl;
        std::cin >> u.param_uint32;
        break;
    default:
        std::cout << "Unsup Type:" << u.type << std::endl;
        break;
    }

    return;
}

int set(std::string paramID)
{
    CameraParameters::cam_param_union_t u;
    input_param_value(u);
    switch (u.type) {
    case CameraParameters::PARAM_TYPE_REAL32:
        log_debug("SET Param: %s Value: %f", paramID.c_str(), u.param_float);
        camParam.setParameter(paramID, u.param_float);
        break;
    case CameraParameters::PARAM_TYPE_UINT32:
        log_debug("SET Param: %s Value: %d", paramID.c_str(), u.param_uint32);
        camParam.setParameter(paramID, u.param_uint32);
        break;
    default:
        std::cout << "Unsupported Type" << std::endl;
        break;
    }

    return u.type;
}

void get(std::string paramID, int type)
{
    std::string paramValue;
    CameraParameters::cam_param_union_t u;
    paramValue = camParam.getParameter(paramID);
    if (paramValue.empty()) {
        log_error("No Parameter found");
        return;
    }
    mem_cpy(u.bytes, sizeof(u.bytes), paramValue.data(), paramValue.size(), sizeof(u.bytes));
    switch (type) {
    case CameraParameters::PARAM_TYPE_REAL32:
        log_debug("GET Param: %s Value: %f", paramID.c_str(), u.param_float);
        break;
    case CameraParameters::PARAM_TYPE_UINT32:
        log_debug("GET Param: %s Value: %d", paramID.c_str(), u.param_uint32);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    Log::open();
    Log::set_max_level(Log::Level::DEBUG);
    log_debug("Camera Parameters Client");

    // print usage
    print_usage();
    // provide options to user
    int input = 0;
    while (1) {
        std::cout << "Please enter selection or enter 0 to exit :" << std::endl;
        std::cin >> input;
        switch (input) {
        case 0:
            std::cout << "Exiting the program..." << std::endl;
            exit(0);
            break;
        case 1: {
            std::cout << "Test Set/Get Operation" << std::endl;
            std::string paramID;
            std::string paramValue;
            int type;
            input_param_id(paramID);
            type = set(paramID);
            get(paramID, type);
            break;
        }
        case 2: {
            std::cout << "Set Parameter" << std::endl;
            std::string paramID;
            input_param_id(paramID);
            set(paramID);
            break;
        }
        case 3: {
            std::cout << "Get Parameter" << std::endl;
            std::string paramID;
            input_param_id(paramID);
            std::string value = camParam.getParameter(paramID);
            if (value.empty())
                std::cout << "Parameter undefined" << std::endl;
            std::cout << paramID << " Value:" << value << std::endl;
            break;
        }
        case 99: {
            uint32_t value;
            std::cout << "Test DataType<->String Conversion Logic" << std::endl;
            std::cout << "Please enter number to use for conversion:" << std::endl;
            std::cin >> value;
            test_convert_logic(value);
        }
        default:
            break;
        }
    }

    // for(uint32_t i =0; i< 4294967295; i++)
    // test_convert_logic(i);
    // log_debug("Test Done\n");

    Log::close();
    return 0;
}
