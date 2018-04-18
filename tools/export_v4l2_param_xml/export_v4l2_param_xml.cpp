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

/**

@brief  This is a tool for generating camera definition file by querying a camera device.

*/

#include <algorithm>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <iostream>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <tinyxml.h>
#include <unistd.h>

struct v4l2_querymenu querymenu;
struct v4l2_capability cap;
struct v4l2_query_ext_ctrl query_ext_ctrl;
struct v4l2_queryctrl queryctrl;

struct options {
    const char *filename;
    const char *devicename;
};

std::string str_to_upper(std::string s);
bool find_substr(std::string s1, std::string s2);
static void enumerate_menu(int file_descp, TiXmlElement *parameter);
void set_type(v4l2_ctrl_type type, TiXmlElement *parameter);
static int parse_argv(int argc, char *argv[], struct options *opt);

std::string str_to_upper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });

    return s;
}

bool find_substr(std::string s1, std::string s2)
{
    s1 = str_to_upper(s1);
    s2 = str_to_upper(s2);
    if (strstr(s1.c_str(), s2.c_str()))
        return 1;
    else
        return 0;
}

static void enumerate_menu(int file_descp, TiXmlElement *parameter)
{
    TiXmlElement *options = new TiXmlElement("options");
    parameter->LinkEndChild(options);
    //   printf("Menu items:\n");
    memset(&querymenu, 0, sizeof(querymenu));
    querymenu.id = query_ext_ctrl.id;

    for (querymenu.index = query_ext_ctrl.minimum; querymenu.index <= query_ext_ctrl.maximum;
         querymenu.index++) {
        if (0 == ioctl(file_descp, VIDIOC_QUERYMENU, &querymenu)) {
            //         printf("%s\n",querymenu.name);
            TiXmlElement *option = new TiXmlElement("option");
            options->LinkEndChild(option);
            option->SetAttribute("name", (char *)querymenu.name);
            option->SetAttribute("value", std::to_string(querymenu.index));

            if (query_ext_ctrl.id == V4L2_CID_EXPOSURE_AUTO) {
                if (find_substr((char *)querymenu.name, std::string("manual")) == 0) {
                    queryctrl.id = V4L2_CID_EXPOSURE;
                    if (0 == ioctl(file_descp, VIDIOC_QUERYCTRL, &queryctrl)) {
                        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                            ;
                        else {
                            TiXmlElement *exclusions = new TiXmlElement("exclusions");
                            option->LinkEndChild(exclusions);
                            TiXmlElement *exclude = new TiXmlElement("exclude");
                            exclude->LinkEndChild(new TiXmlText("exposure"));
                            exclusions->LinkEndChild(exclude);
                        }
                    } else {
                        queryctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
                        if (0 == ioctl(file_descp, VIDIOC_QUERYCTRL, &queryctrl)) {
                            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                                ;
                            else {
                                TiXmlElement *exclusions = new TiXmlElement("exclusions");
                                option->LinkEndChild(exclusions);
                                TiXmlElement *exclude = new TiXmlElement("exclude");
                                exclude->LinkEndChild(new TiXmlText((char *)queryctrl.name));
                                exclusions->LinkEndChild(exclude);
                            }
                        }
                    }
                }
            }

        } else {
            perror("VIDIOC_QUERYMENU");
        }
    }
}

void set_type(v4l2_ctrl_type type, TiXmlElement *parameter)
{
    switch (type) {
    case V4L2_CTRL_TYPE_INTEGER:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_BOOLEAN:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_MENU:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_INTEGER_MENU:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_BITMASK:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_BUTTON:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_INTEGER64:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_STRING:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_CTRL_CLASS:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_U8:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_U16:
        parameter->SetAttribute("type", "int32");
        break;
    case V4L2_CTRL_TYPE_U32:
        parameter->SetAttribute("type", "int32");
        break;
    }
}

static int parse_argv(int argc, char *argv[], struct options *opt)
{
    static const struct option options[] = {{"device-name with path", required_argument, NULL, 'd'},
                                            {"output file name", required_argument, NULL, 'f'}};
    int c;

    if (argc != 5) {
        printf("Usage: ./export_v4l2_param_xml -d  <device node>  - f <output "
               "camera-def-file-name>\n");
        printf("Example: ./export_v4l2_param_xml -d /dev/video0 -f camera_definition.xml\n");
        return -1;
    }
    assert(argv);

    while ((c = getopt_long(argc, argv, "hd:d:f:", options, NULL)) >= 0) {
        switch (c) {
        case 'd':
            opt->devicename = optarg;
            break;
        case 'f':
            opt->filename = optarg;
            break;
        default:
            printf("Invalid option\n");
        }
    }

    return 0;
}
int main(int argc, char **argv)
{
    struct options opt = {0};
    if (parse_argv(argc, argv, &opt) != 0) {
        return EXIT_FAILURE;
    }

    int fd = open(opt.devicename, O_RDWR);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    TiXmlDocument doc;
    TiXmlElement *parameter, *model, *vendor, *description, *options, *option, *exclusions,
        *exclude;
    TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    doc.LinkEndChild(decl);

    TiXmlElement *root = new TiXmlElement("mavlinkcamera");

    TiXmlElement *definition = new TiXmlElement("definition");
    root->LinkEndChild(definition);
    definition->SetAttribute("version", "1");

    memset(&cap, 0, sizeof(cap));
    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        perror(" VIDIOC_QUERYCAP");
        exit(EXIT_FAILURE);
    }

    model = new TiXmlElement("model");
    model->LinkEndChild(new TiXmlText((char *)cap.card));
    definition->LinkEndChild(model);

    vendor = new TiXmlElement("vendor");
    vendor->LinkEndChild(new TiXmlText((char *)cap.card));
    definition->LinkEndChild(vendor);

    TiXmlElement *parameters = new TiXmlElement("parameters");
    root->LinkEndChild(parameters);

    parameter = new TiXmlElement("parameter");
    parameters->LinkEndChild(parameter);

    parameter->SetAttribute("name", "camera-mode");
    parameter->SetAttribute("type", "uint32");
    parameter->SetAttribute("default", "1");
    parameter->SetAttribute("control", "0");

    description = new TiXmlElement("description");
    description->LinkEndChild(new TiXmlText("Camera Mode"));
    parameter->LinkEndChild(description);
    options = new TiXmlElement("options");
    parameter->LinkEndChild(options);
    option = new TiXmlElement("option");
    options->LinkEndChild(option);
    option->SetAttribute("name", "still");
    option->SetAttribute("value", "0");
    exclusions = new TiXmlElement("exclusions");
    options->LinkEndChild(exclusions);
    exclude = new TiXmlElement("exclude");
    exclude->LinkEndChild(new TiXmlText("video-size"));
    exclusions->LinkEndChild(exclude);

    option = new TiXmlElement("option");
    options->LinkEndChild(option);
    option->SetAttribute("name", "video");
    option->SetAttribute("value", "1");

    memset(&query_ext_ctrl, 0, sizeof(query_ext_ctrl));

    query_ext_ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;

    while (0 == ioctl(fd, VIDIOC_QUERY_EXT_CTRL, &query_ext_ctrl)) {

        if (query_ext_ctrl.id != V4L2_CID_CAMERA_CLASS
            && query_ext_ctrl.id != V4L2_CID_USER_CLASS) {
            if (!(query_ext_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {

                parameter = new TiXmlElement("parameter");
                parameters->LinkEndChild(parameter);
                description = new TiXmlElement("description");
                parameter->LinkEndChild(description);

                switch (query_ext_ctrl.id) {
                case V4L2_CID_BRIGHTNESS:
                    parameter->SetAttribute("name", "brightness");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Brightness"));
                    break;
                case V4L2_CID_CONTRAST:
                    parameter->SetAttribute("name", "contrast");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Contrast"));
                    break;
                case V4L2_CID_AUTO_WHITE_BALANCE:
                    parameter->SetAttribute("name", "wb-mode");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    description->LinkEndChild(new TiXmlText("White Balance Automatic"));
                    options = new TiXmlElement("options");
                    parameter->LinkEndChild(options);
                    option = new TiXmlElement("option");
                    options->LinkEndChild(option);
                    option->SetAttribute("name", "Manual mode");
                    option->SetAttribute("value", "0");
                    option = new TiXmlElement("option");
                    options->LinkEndChild(option);
                    option->SetAttribute("name", "Auto mode");
                    option->SetAttribute("value", "1");
                    queryctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                    if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
                        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                            break;
                        else {
                            exclusions = new TiXmlElement("exclusions");
                            option->LinkEndChild(exclusions);
                            exclude = new TiXmlElement("exclude");
                            exclude->LinkEndChild(new TiXmlText("wb-temp"));
                            exclusions->LinkEndChild(exclude);
                        }
                    }
                    break;
                case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
                    parameter->SetAttribute("name", "wb-temp");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("White Balance Temperature"));
                    break;
                case V4L2_CID_SATURATION:
                    parameter->SetAttribute("name", "saturation");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Saturation"));
                    break;
                case V4L2_CID_HUE:
                    parameter->SetAttribute("name", "hue");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Hue"));
                    break;
                case V4L2_CID_EXPOSURE_AUTO:
                    parameter->SetAttribute("name", "exp-mode");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    description->LinkEndChild(new TiXmlText("Exposure Auto"));
                    if (query_ext_ctrl.type == V4L2_CTRL_TYPE_MENU)
                        enumerate_menu(fd, parameter);
                    break;
                case V4L2_CID_EXPOSURE:
                    parameter->SetAttribute("name", "exposure");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Exposure"));
                    break;
                case V4L2_CID_GAIN:
                    parameter->SetAttribute("name", "gain");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Gain"));
                    break;
                case V4L2_CID_POWER_LINE_FREQUENCY:
                    parameter->SetAttribute("name", "power-mode");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    description->LinkEndChild(new TiXmlText("Power Line Frequency"));
                    enumerate_menu(fd, parameter);
                    break;
                case V4L2_CID_SHARPNESS:
                    parameter->SetAttribute("name", "sharpness");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    description->LinkEndChild(new TiXmlText("Sharpness"));
                    break;
                case V4L2_CID_AUTOGAIN:
                    parameter->SetAttribute("name", "auto_gain");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    description->LinkEndChild(new TiXmlText("Automatic gain"));
                    options = new TiXmlElement("options");
                    parameter->LinkEndChild(options);
                    option = new TiXmlElement("option");
                    options->LinkEndChild(option);
                    option->SetAttribute("name", "Manual mode");
                    option->SetAttribute("value", "0");
                    option = new TiXmlElement("option");
                    options->LinkEndChild(option);
                    option->SetAttribute("name", "Auto mode");
                    option->SetAttribute("value", "1");
                    queryctrl.id = V4L2_CID_GAIN;
                    if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
                        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                            break;
                        else {
                            exclusions = new TiXmlElement("exclusions");
                            option->LinkEndChild(exclusions);
                            exclude = new TiXmlElement("exclude");
                            exclude->LinkEndChild(new TiXmlText("gain"));
                            exclusions->LinkEndChild(exclude);
                        }
                    }
                    break;
                case V4L2_CID_HFLIP:
                    parameter->SetAttribute("name", "horizontal_flip");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    description->LinkEndChild(new TiXmlText("Horizontal Flip"));
                    break;
                case V4L2_CID_VFLIP:
                    parameter->SetAttribute("name", "vertical_flip");
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    description->LinkEndChild(new TiXmlText("Vertical Flip"));
                    break;
                default:
                    parameter->SetAttribute("name", (char *)query_ext_ctrl.name);
                    set_type((v4l2_ctrl_type)query_ext_ctrl.type, parameter);
                    parameter->SetAttribute("default",
                                            std::to_string((uint32_t)query_ext_ctrl.default_value));
                    parameter->SetAttribute("min", std::to_string(query_ext_ctrl.minimum));
                    parameter->SetAttribute("max", std::to_string(query_ext_ctrl.maximum));
                    parameter->SetAttribute("step", std::to_string(query_ext_ctrl.step));
                    if (query_ext_ctrl.type == V4L2_CTRL_TYPE_MENU)
                        enumerate_menu(fd, parameter);
                    description->LinkEndChild(new TiXmlText((char *)query_ext_ctrl.name));
                }
            }
        }
        query_ext_ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    }
    if (errno != EINVAL) {
        perror("VIDIOC_QUERY_EXT_CTRL");
        exit(EXIT_FAILURE);
    }
    doc.LinkEndChild(root);
    if (!(doc.SaveFile(opt.filename)))
        perror("doc.SaveFile");

    if (close(fd) == -1)
        perror("close");

    // check if xml file is well formed

    TiXmlDocument check_doc(argv[4]);
    if (!check_doc.LoadFile()) {
        printf("Error:%s", check_doc.ErrorDesc());
        exit(EXIT_FAILURE);
    }
}
