#include<iostream>
#include<fcntl.h>
#include<stdio.h>
#include<cstring>
#include<errno.h>
#include<linux/videodev2.h>
#include <sys/ioctl.h>
#include <tinyxml.h>

struct v4l2_querymenu querymenu;
struct v4l2_capability cap;
struct v4l2_query_ext_ctrl query_ext_ctrl;

static void enumerate_menu (int fd,TiXmlElement * parameter)
{
        parameter->SetAttribute("type1","junk"); 
        TiXmlElement * abc=new TiXmlElement( "options" );
        parameter->LinkEndChild(abc);

        printf ("  Menu items:\n");

        memset (&querymenu, 0, sizeof (querymenu));
        querymenu.id = query_ext_ctrl.id;

        for (querymenu.index = query_ext_ctrl.minimum;
             querymenu.index <= query_ext_ctrl.maximum;
              querymenu.index++) {
                if (0 == ioctl (fd, VIDIOC_QUERYMENU, &querymenu)) {
                        printf ("  %s\n", querymenu.name);
                        TiXmlElement * msg3=new TiXmlElement( "option" );
                        abc->LinkEndChild(msg3);
                        msg3->SetAttribute("name",(char*)querymenu.name);
                        msg3->SetAttribute("value",std::to_string(querymenu.index));
                } else {
                        perror ("VIDIOC_QUERYMENU");
                        exit (EXIT_FAILURE);
                }
        }
}

void set_type(int a, TiXmlElement * parameter)
{
switch(a){
case 1: parameter->SetAttribute("type","int");break;
case 2: parameter->SetAttribute("type","bool");break;
case 3: parameter->SetAttribute("type","enum");break;
case 4: parameter->SetAttribute("type","integer_menu");break;
case 5: parameter->SetAttribute("type","bitmask");break;
case 6: parameter->SetAttribute("type","button");break;
case 7: parameter->SetAttribute("type","int64");break;
case 8: parameter->SetAttribute("type","string");break;
case 9: parameter->SetAttribute("type","ctrl_class");break;
case 10: parameter->SetAttribute("type","uint8");break;
case 11: parameter->SetAttribute("type","uint16");break;
case 12 :parameter->SetAttribute("type","uint32");break;
}
}


int main()
{

std::string paramName;
int fd=open("/dev/video0",O_RDWR);
//int fd1 = v4l2_open("/dev/video0");

TiXmlDocument doc;
TiXmlElement * parameter,*msg, *msg1, *msg2, *msg3, *msg4;
TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0","UTF-8","" );
doc.LinkEndChild(decl);

TiXmlElement * root = new TiXmlElement( "mavlinkcamera" );  

TiXmlElement * definition = new TiXmlElement( "definition" );
root->LinkEndChild( definition );  
definition->SetAttribute("version", "1");

memset (&cap, 0, sizeof (cap));
if (-1 == ioctl (fd, VIDIOC_QUERYCAP, &cap)) {
        perror (" VIDIOC_QUERYCAP");
        exit (EXIT_FAILURE);
}

msg = new TiXmlElement( "model" );  
msg->LinkEndChild( new TiXmlText( (char*) cap.card  ));  
definition->LinkEndChild( msg );

msg = new TiXmlElement( "vendor" );
msg->LinkEndChild( new TiXmlText( (char*) cap.card  ));
definition->LinkEndChild( msg );


TiXmlElement * parameters = new TiXmlElement( "parameters" );
root->LinkEndChild( parameters );

parameter = new TiXmlElement( "parameter" );
parameters->LinkEndChild( parameter );

parameter->SetAttribute("name","camera-mode");
parameter->SetAttribute("type","uint32");
parameter->SetAttribute("default","1");
parameter->SetAttribute("control","0");

msg1 = new TiXmlElement( "description" );
msg1->LinkEndChild( new TiXmlText( "Camera Mode" ));
parameter->LinkEndChild( msg1 );
msg1 = new TiXmlElement( "options" );
parameter->LinkEndChild( msg1 );
msg2 = new TiXmlElement( "option" );
msg1->LinkEndChild( msg2 );
msg2->SetAttribute("name","still");
msg2->SetAttribute("value","0");
msg3 = new TiXmlElement( "exclusions" );
msg2->LinkEndChild( msg3 );
msg4 = new TiXmlElement( "exclude" );
msg4->LinkEndChild( new TiXmlText( "video-size" ));
msg3->LinkEndChild( msg4 );


msg2 = new TiXmlElement( "option" );
msg1->LinkEndChild( msg2 );
msg2->SetAttribute("name","video");
msg2->SetAttribute("value","1");


memset(&query_ext_ctrl, 0, sizeof(query_ext_ctrl));

query_ext_ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL| V4L2_CTRL_FLAG_NEXT_COMPOUND;

while (0 == ioctl(fd, VIDIOC_QUERY_EXT_CTRL, &query_ext_ctrl)) {
    if(query_ext_ctrl.id!=V4L2_CID_CAMERA_CLASS || query_ext_ctrl.id!=V4L2_CID_USER_CLASS){
             if (!(query_ext_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)) 
              {

parameter= new TiXmlElement( "parameter" );
parameters->LinkEndChild( parameter );
msg1 = new TiXmlElement( "description" );
parameter->LinkEndChild( msg1 );

switch (query_ext_ctrl.id) {
        case V4L2_CID_BRIGHTNESS:
            parameter->SetAttribute("name","brightness");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Brightness" ));
            break;
        case V4L2_CID_CONTRAST:
            parameter->SetAttribute("name","contrast");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Contrast" ));
            break;
        case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
            parameter->SetAttribute("name","wb-temp");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "White Balance Temperature" ));
            break;
        case V4L2_CID_SATURATION:
            parameter->SetAttribute("name","saturation");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Saturation" ));
            break;
        case V4L2_CID_HUE:
            parameter->SetAttribute("name","hue");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Hue" ));
            break;
        case V4L2_CID_EXPOSURE_AUTO:
            parameter->SetAttribute("name","exp-mode");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            if(query_ext_ctrl.type==3)
              enumerate_menu (fd,parameter);
            msg1->LinkEndChild( new TiXmlText( "Exposure Auto" )); 
        case V4L2_CID_EXPOSURE:
            parameter->SetAttribute("name","exposure");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            msg1->LinkEndChild( new TiXmlText( "Exposure" ));
            break;
        case V4L2_CID_GAIN:
            parameter->SetAttribute("name","gain");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Gain" ));
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY:
            parameter->SetAttribute("name","power-mode");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            msg1->LinkEndChild( new TiXmlText( "Power Line Frequency" ));
            parameter->LinkEndChild( msg1 );
            enumerate_menu (fd,parameter);
            break;
        case V4L2_CID_SHARPNESS:
            parameter->SetAttribute("name","sharpness");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Sharpness" ));
            break;
        case V4L2_CID_AUTOGAIN:
            parameter->SetAttribute("name","auto_gain");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Automatic gain" ));
            if(query_ext_ctrl.default_value==1){
            msg3 = new TiXmlElement( "exclusions" );
            parameter->LinkEndChild( msg3 );
            msg4 = new TiXmlElement( "exclude" );
            msg4->LinkEndChild( new TiXmlText( "gain" ));
            msg3->LinkEndChild( msg4 );}
            break;
        case V4L2_CID_HFLIP:
            parameter->SetAttribute("name","horizontal_flip");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Horizontal Flip" ));
            break;
        case V4L2_CID_VFLIP:
            parameter->SetAttribute("name","vertical_flip");
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
            parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
            parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
            parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            msg1->LinkEndChild( new TiXmlText( "Vertical Flip" ));
            break;
        default:
            parameter->SetAttribute("name",(char*)query_ext_ctrl.name);
            set_type(query_ext_ctrl.type,parameter);
            parameter->SetAttribute("default",std::to_string((uint32_t)query_ext_ctrl.default_value));
	    parameter->SetAttribute("min",std::to_string(query_ext_ctrl.minimum));
	    parameter->SetAttribute("max",std::to_string(query_ext_ctrl.maximum));
	    parameter->SetAttribute("step",std::to_string(query_ext_ctrl.step));
            if(query_ext_ctrl.type==3)
              enumerate_menu (fd,parameter);
            msg1->LinkEndChild( new TiXmlText( (char*)query_ext_ctrl.name ));

        }
}
}
    query_ext_ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
}
if (errno != EINVAL) {
    perror("VIDIOC_QUERY_EXT_CTRL");
    exit(EXIT_FAILURE);
}
       
doc.LinkEndChild( root );
doc.SaveFile( "camera-def-file.xml" );
}
