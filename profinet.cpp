#define _CRT_SECURE_NO_WARNINGS
#include "include/profinet.h"
#include "snap7.h"
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <string>
#include <sstream>
#include <cstring>
#include <codecvt>
#include <vector>
#include "include/sqlite3.h"


using namespace std;
Profinet::Profinet(){}
Profinet::Profinet(const char* PLC_IP)
{
    IP = PLC_IP;
}

Profinet::~Profinet()
{

}


uint16_t Profinet::S7_reverse_endianess(uint16_t value)
{
    return ((value >> 8) & 0xff) | ((value << 8) & 0xff00);
}

uint32_t Profinet::S32_reverse_endianess(uint32_t value)
{
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) |((value << 8) & 0xff0000) |((value << 24) & 0xff000000);
}

uint64_t Profinet::S64_reverse_endianess(uint64_t value0)
{
    return ((value0 >> 56) & 0xff) | ((value0 >> 40) & 0xff00) | ((value0 >> 24) & 0xff0000) | ((value0 >> 8) & 0xff000000) | ((value0 << 8) & 0xff00000000) | ((value0 << 24) & 0xff0000000000) | ((value0 << 40) & 0xff000000000000) | ((value0 << 56) & 0xff00000000000000);
}

float Profinet::F32_reverse_endianess(float value)
{
    unsigned char		s[4], t[4];
    float				fResult;
 
    memcpy(s, &value, sizeof(float));
 
    t[0] = s[3];
    t[1] = s[2];
    t[2] = s[1];
    t[3] = s[0];
 
    memcpy(&fResult, t, sizeof(float));
 
    return fResult;
}

float Profinet::Float_reverse_endianess(byte value[])
{
    float result;
    *((byte*)&result + 0) = value[3];
    *((byte*)&result + 1) = value[2];
    *((byte*)&result + 2) = value[1];
    *((byte*)&result + 3) = value[0];
    return result;
}

void Profinet::mytoupper(string& s){
    int len=s.size();
    for(int i=0;i<len;i++){
        if(s[i]>='a'&&s[i]<='z'){
            s[i]-=32;//+32转换为小写
            //s[i]=s[i]-'a'+'A';
        }
    }
}


wstring Profinet::uint8ArrayToWString(const uint8_t* arr, size_t size) {
    std::stringstream ss;
    for (size_t i = 0; i < size; ++i) {
        ss << static_cast<char>(arr[i]);
    }
    std::string strLocale = setlocale(LC_ALL, "");
    const char* chSrc = ss.str().c_str();
    size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t* wchDest = new wchar_t[nDestSize];
    wmemset(wchDest, 0, nDestSize);
    mbstowcs(wchDest, chSrc, nDestSize);
    std::wstring wstrResult = wchDest;
    delete[] wchDest;
    setlocale(LC_ALL, strLocale.c_str());
    return wstrResult;
}

string Profinet::WString2String(const std::wstring& ws)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const wchar_t* wchSrc = ws.c_str();
    size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
    char* chDest = new char[nDestSize];
    memset(chDest, 0, nDestSize);
    wcstombs(chDest, wchSrc, nDestSize);
    std::string strResult = chDest;
    delete[] chDest;
    setlocale(LC_ALL, strLocale.c_str());
    return strResult;
}

wstring Profinet::String2WString(const std::string& s)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const char* chSrc = s.c_str();
    size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t* wchDest = new wchar_t[nDestSize];
    wmemset(wchDest, 0, nDestSize);
    mbstowcs(wchDest, chSrc, nDestSize);
    std::wstring wstrResult = wchDest;
    delete[] wchDest;
    setlocale(LC_ALL, strLocale.c_str());
    return wstrResult;
}

string Profinet::ReadStr(uint8_t str2[])
{
    int charnums = str2[3];

    uint16_t num;
    wstring wstr;
    string str;


    for (int i = 0; i < charnums; i++)
    {
        num = ((str2[2 * i + 4]) << 8) + str2[2 * i + 5];
        // 将Unicode编码转换为宽字符并添加到字符串
        wstr = static_cast<wchar_t>(num);
        str += WString2String(wstr);
    }

    return str;
}


void Profinet::WriteWStr(string str,int8_t* str1)
{
    
    
    
    wstring wstr = String2WString(str);
    //cout << "wstr lenth is :" << wstr.length() << endl;
    //int8_t str1[wstr.length()*2+4];
    str1[0] = 0x00;
    str1[1] = 0x0A;
    str1[2] = 0x00;
    str1[3] = wstr.length();
    for (int i = 0; i < wstr.length(); i++) 
    {
        str1[2*i + 4] = (wstr[i] >> 8) & 0xff;
        str1[2*i + 5] = wstr[i] & 0xff;
    }

}

void Profinet::WriteStr(string str, int8_t *str1)
{
    const char* charArray1 = str.c_str();
    str1[0] = 255;
    str1[1] = str.length();
    for (int i = 0 ;i<str.length();i++)
    {
        str1[i+2] = charArray1[i];
    }
}

void Profinet::DownloadFlightInfo(int row,flightInfoData flightInfo[],TS7Client* client_)
{
    
    //const char* PLC_IP = "192.168.0.1";
    
    int8_t str[80][254]={0};
    
    for(int i = 0;i<80;i++)
    {
        WriteWStr(flightInfo[i].flightNum,str[i]);
        
        client_->DBWrite(6, 68*i, 4+str[i][3]*2, &str[i]);

        WriteWStr(flightInfo[i].aircraftType,str[i]);

        client_->DBWrite(6, 68*i+24, 4+str[i][3]*2, &str[i]);

        WriteWStr(flightInfo[i].startPlace,str[i]);

        client_->DBWrite(6, 68*i+44, 4+str[i][3]*2, &str[i]);

        WriteWStr(flightInfo[i].endPlace,str[i]);
        
        client_->DBWrite(6, 68*i+60, 4+str[i][3]*2, &str[i]);

        flightInfo[i].arriveMonth = S7_reverse_endianess(flightInfo[i].arriveMonth);
        client_->DBWrite(6, 68*i+76, 2, &flightInfo[i].arriveMonth);
        
        flightInfo[i].arriveDay = S7_reverse_endianess(flightInfo[i].arriveDay);
        client_->DBWrite(6, 68*i+78, 2, &flightInfo[i].arriveDay);

        flightInfo[i].arriveHour = S7_reverse_endianess(flightInfo[i].arriveHour);
        client_->DBWrite(6, 68*i+80, 2, &flightInfo[i].arriveHour);

        flightInfo[i].arriveMinute = S7_reverse_endianess(flightInfo[i].arriveMinute);
        client_->DBWrite(6, 68*i+82, 2, &flightInfo[i].arriveMinute);
        
        flightInfo[i].leaveMonth = S7_reverse_endianess(flightInfo[i].leaveMonth);
        client_->DBWrite(6, 68*i+84, 2, &flightInfo[i].leaveMonth);

        flightInfo[i].leaveDay = S7_reverse_endianess(flightInfo[i].leaveDay);
        client_->DBWrite(6, 68*i+86, 2, &flightInfo[i].leaveDay);

        flightInfo[i].leaveHour = S7_reverse_endianess(flightInfo[i].leaveHour);
        client_->DBWrite(6, 68*i+88, 2, &flightInfo[i].leaveHour);
        
        flightInfo[i].leaveMinute = S7_reverse_endianess(flightInfo[i].leaveMinute);
        client_->DBWrite(6, 68*i+90, 2, &flightInfo[i].leaveMinute);
    }
    

    //flightInfo.arriveHour = S7_reverse_endianess(flightInfo.arriveHour);
    //client_->DBWrite(1, 456, 2, &flightInfo.arriveMonth);

}

void Profinet::DownloadVolumeInfo(int row, volumeInfoData volumeInfo[], TS7Client *client_)
{
    int8_t str[80][254]={0};
    float f = 0;
    
    for(int i = 0;i<80;i++)
    {
        //cout<<"Mark2"<<endl;
        WriteWStr(volumeInfo[i].flightNum,str[i]);
        
        client_->DBWrite(14, 112*i, 4+str[i][3]*2, &str[i]);

        WriteStr(volumeInfo[i].coverage_start,str[i]);

        client_->DBWrite(14, 112*i+24, 22, &str[i]);

        WriteStr(volumeInfo[i].coverage_end,str[i]);

        client_->DBWrite(14, 112*i+46, 22, &str[i]);


        f = F32_reverse_endianess(volumeInfo[i].coverage_duration);
        client_->DBWrite(14, 112*i+68, 4, &f);
        
        f = F32_reverse_endianess(volumeInfo[i].power_duration);
        client_->DBWrite(14, 112*i+72, 4, &f);

        f = F32_reverse_endianess(volumeInfo[i].power_consumption);
        client_->DBWrite(14, 112*i+76, 4, &f);

        f = F32_reverse_endianess(volumeInfo[i].wind_duration);
        client_->DBWrite(14, 112*i+80, 4, &f);
        
        f = F32_reverse_endianess(volumeInfo[i].wind_volume);
        client_->DBWrite(14, 112*i+84, 4, &f);

        f = F32_reverse_endianess(volumeInfo[i].clean_duration);
        client_->DBWrite(14, 112*i+88, 4, &f);

        f = F32_reverse_endianess(volumeInfo[i].clean_volume);
        client_->DBWrite(14, 112*i+92, 4, &f);
        
        f = F32_reverse_endianess(volumeInfo[i].flush_duration);
        client_->DBWrite(14, 112*i+96, 4, &f);

        f = F32_reverse_endianess(volumeInfo[i].flush_volume);
        client_->DBWrite(14, 112*i+100, 4, &f);

        f = F32_reverse_endianess(volumeInfo[i].sewage_duration);
        client_->DBWrite(14, 112*i+104, 4, &f);
        
        f = F32_reverse_endianess(volumeInfo[i].sewage_volume);
        client_->DBWrite(14, 112*i+108, 4, &f);
    }
}


flightInfoData Profinet::InsertFlightInfo(TS7Client *client_, flightInfoData flightInfo)
{
    //int insertFlag = 0;
    uint8_t buf[254];
    uint16_t temp;
    client_ -> DBRead(6,5440,16,&buf);
    flightInfo.flightNum = ReadStr(buf);
    client_ -> DBRead(6,5456,12,&buf);
    flightInfo.aircraftType = ReadStr(buf);
    client_ -> DBRead(6,5468,12,&buf);
    flightInfo.startPlace = ReadStr(buf);
    client_ -> DBRead(6,5480,12,&buf);
    flightInfo.endPlace = ReadStr(buf);
    client_ -> DBRead(6,5492,2,&temp);
    flightInfo.arriveMonth = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5494,2,&temp);
    flightInfo.arriveDay = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5496,2,&temp);
    flightInfo.arriveHour = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5498,2,&temp);
    flightInfo.arriveMinute = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5500,2,&temp);
    flightInfo.leaveMonth = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5502,2,&temp);
    flightInfo.leaveDay = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5504,2,&temp);
    flightInfo.leaveHour = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5506,2,&temp);
    flightInfo.leaveMinute = S7_reverse_endianess(temp);
    //client_ -> DBRead(6,5508,2,&temp);
    //insertFlag = S7_reverse_endianess(temp);
    time_t t = time(0); 
    char tmp[64],tmp2[64];
    strftime(tmp, sizeof(tmp), "%Y",localtime(&t));
    string years;
    years.assign(tmp);
    snprintf(tmp2,64,"%s-%02d-%02d %02d:%02d",years.c_str(),flightInfo.arriveMonth,flightInfo.arriveDay,flightInfo.arriveHour,flightInfo.arriveMinute);
    flightInfo.arriveTime.assign(tmp2);
    snprintf(tmp2,64,"%s-%02d-%02d %02d:%02d",years.c_str(),flightInfo.leaveMonth,flightInfo.leaveDay,flightInfo.leaveHour,flightInfo.leaveMinute);
    flightInfo.leaveTime.assign(tmp2);

    mytoupper(flightInfo.flightNum);
    mytoupper(flightInfo.aircraftType);

    return flightInfo;
}

flightInfoData Profinet::DeleteFlightInfo(TS7Client* client_,flightInfoData flightInfo)
{
    int delFlag = 0;
    uint8_t buf[254];
    uint16_t temp;
    client_ -> DBRead(6,5510,16,&buf);
    flightInfo.flightNum = ReadStr(buf);
    client_ -> DBRead(6,5526,12,&buf);
    flightInfo.aircraftType = ReadStr(buf);
    client_ -> DBRead(6,5538,12,&buf);
    flightInfo.startPlace = ReadStr(buf);
    client_ -> DBRead(6,5550,12,&buf);
    flightInfo.endPlace = ReadStr(buf);
    client_ -> DBRead(6,5562,2,&temp);
    flightInfo.arriveMonth = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5564,2,&temp);
    flightInfo.arriveDay = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5566,2,&temp);
    flightInfo.arriveHour = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5568,2,&temp);
    flightInfo.arriveMinute = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5570,2,&temp);
    flightInfo.leaveMonth = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5572,2,&temp);
    flightInfo.leaveDay = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5574,2,&temp);
    flightInfo.leaveHour = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5576,2,&temp);
    flightInfo.leaveMinute = S7_reverse_endianess(temp);
    client_ -> DBRead(6,5578,2,&temp);
    delFlag = S7_reverse_endianess(temp);
    time_t t = time(0); 
    char tmp[64],tmp2[64];
    strftime(tmp, sizeof(tmp), "%Y",localtime(&t));
    string years;
    years.assign(tmp);
    snprintf(tmp2,64,"%s-%02d-%02d %02d:%02d",years.c_str(),flightInfo.arriveMonth,flightInfo.arriveDay,flightInfo.arriveHour,flightInfo.arriveMinute);
    flightInfo.arriveTime.assign(tmp2);
    snprintf(tmp2,64,"%s-%02d-%02d %02d:%02d",years.c_str(),flightInfo.leaveMonth,flightInfo.leaveDay,flightInfo.leaveHour,flightInfo.leaveMinute);
    flightInfo.leaveTime.assign(tmp2);
    mytoupper(flightInfo.flightNum);
    mytoupper(flightInfo.aircraftType);
    
    return flightInfo;
}


airWellData Profinet::GetairWellInfo(TS7Client* client_)
{
    airWellData aData;
    aData.deviceType = "4-1";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(300,1,&tmp);
    aData.running_state =  tmp &  0x01;
    aData.is_alarm = (tmp >> 1) & 0x01;
    aData.is_stop = (tmp >> 2) & 0x01;
    aData.hp_start = (tmp >> 4) & 0x01;
    aData.psrv_work = (tmp >> 5) & 0x01;

    client_ ->MBRead(301,1,&tmp); 
    aData.mwp_manual = (tmp >> 2) & 0x01;
    aData.bwp_manual = (tmp >> 3) & 0x01;
    aData.mwp_function = (tmp >> 4) & 0x01;
    aData.bwp_function = (tmp >> 5) & 0x01;
    aData.dd_function = (tmp >> 6) & 0x01;
    
    client_ -> MBRead(302,1,&tmp);
    aData.pv1_function = (tmp >> 1) & 0x01;
    aData.pv2_function = (tmp >> 2) & 0x01;
    aData.cw1_open = (tmp >> 3) & 0x01;
    aData.cw1_close = (tmp >> 4) & 0x01;
    aData.cw2_open = (tmp >> 5) & 0x01;
    aData.cw2_close = (tmp >> 6) & 0x01;
    
    client_ -> MBRead(320,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.gas_concentration = tmp;
    
    client_ -> MBRead(330,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.ps_pressure = tmp;
    
    client_ -> MBRead(340,4,&tmp32);
    tmp32 = S32_reverse_endianess(tmp32);
    aData.cw_amount = tmp32;

    return aData;
}


airWellData Profinet::GetairWellLeftInfo(TS7Client* client_)
{
    airWellData aData;
    aData.deviceType = "4-1";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(300,1,&tmp);
    aData.running_state =  tmp &  0x01;
    aData.is_alarm = (tmp >> 1) & 0x01;
    aData.is_stop = (tmp >> 2) & 0x01;
    aData.hp_start = (tmp >> 4) & 0x01;
    aData.psrv_work = (tmp >> 5) & 0x01;
    tmp = 0;
    client_ ->MBRead(301,1,&tmp);
    aData.mwp_manual = (tmp >> 2) & 0x01;
    aData.bwp_manual = (tmp >> 3) & 0x01;
    aData.mwp_function = (tmp >> 4) & 0x01;
    aData.bwp_function = (tmp >> 5) & 0x01;
    aData.dd_function = (tmp >> 6) & 0x01;
    tmp = 0;
    client_ -> MBRead(302,1,&tmp);
    aData.pv1_function = (tmp >> 1) & 0x01;
    aData.pv2_function = (tmp >> 2) & 0x01;
    aData.cw1_open = (tmp >> 3) & 0x01;
    aData.cw1_close = (tmp >> 4) & 0x01;
    aData.cw2_open = (tmp >> 5) & 0x01;
    aData.cw2_close = (tmp >> 6) & 0x01;
    tmp = 0;
    client_ -> MBRead(303,1,&tmp);
    aData.dd2_function = (tmp >> 3) & 0x01;
    

    
    client_ -> MBRead(320,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.gas_concentration = tmp;
    
    client_ -> MBRead(330,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.ps_pressure = tmp;
    
    client_ -> MBRead(340,4,&tmp32);
    tmp32 = S32_reverse_endianess(tmp32);
    aData.cw_amount = tmp32;

    return aData;
}

airWellData Profinet::GetairWellMidInfo(TS7Client* client_)
{
    airWellData aData;
    aData.deviceType = "4-2";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(500,1,&tmp);
    aData.running_state =  tmp &  0x01;
    aData.is_alarm = (tmp >> 1) & 0x01;
    aData.is_stop = (tmp >> 2) & 0x01;
    aData.hp_start = (tmp >> 4) & 0x01;
    aData.psrv_work = (tmp >> 5) & 0x01;
    tmp = 0;
    client_ ->MBRead(501,1,&tmp);
    aData.mwp_manual = (tmp >> 2) & 0x01;
    aData.bwp_manual = (tmp >> 3) & 0x01;
    aData.mwp_function = (tmp >> 4) & 0x01;
    aData.bwp_function = (tmp >> 5) & 0x01;
    aData.dd_function = (tmp >> 6) & 0x01;
    tmp = 0;
    client_ -> MBRead(502,1,&tmp);
    aData.pv1_function = (tmp >> 1) & 0x01;
    aData.pv2_function = (tmp >> 2) & 0x01;
    aData.cw1_open = (tmp >> 3) & 0x01;
    aData.cw1_close = (tmp >> 4) & 0x01;
    aData.cw2_open = (tmp >> 5) & 0x01;
    aData.cw2_close = (tmp >> 6) & 0x01;
    tmp = 0;
    client_ -> MBRead(503,1,&tmp);
    aData.dd2_function = (tmp >> 3) & 0x01;
    

    
    client_ -> MBRead(620,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.gas_concentration = tmp;
    
    client_ -> MBRead(630,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.ps_pressure = tmp;
    tmp32 = 0;
    client_ -> MBRead(640,4,&tmp32);
    tmp32 = S7_reverse_endianess(tmp32);
    aData.cw_amount = tmp32;

    return aData;
}

airWellData Profinet::GetairWellRightInfo(TS7Client* client_)
{
    airWellData aData;
    aData.deviceType = "4-3";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(800,1,&tmp);
    aData.running_state =  tmp &  0x01;
    aData.is_alarm = (tmp >> 1) & 0x01;
    aData.is_stop = (tmp >> 2) & 0x01;
    aData.hp_start = (tmp >> 4) & 0x01;
    aData.psrv_work = (tmp >> 5) & 0x01;
    tmp = 0;
    client_ ->MBRead(801,1,&tmp);
    aData.mwp_manual = (tmp >> 2) & 0x01;
    aData.bwp_manual = (tmp >> 3) & 0x01;
    aData.mwp_function = (tmp >> 4) & 0x01;
    aData.bwp_function = (tmp >> 5) & 0x01;
    aData.dd_function = (tmp >> 6) & 0x01;
    tmp = 0;
    client_ -> MBRead(802,1,&tmp);
    aData.pv1_function = (tmp >> 1) & 0x01;
    aData.pv2_function = (tmp >> 2) & 0x01;
    aData.cw1_open = (tmp >> 3) & 0x01;
    aData.cw1_close = (tmp >> 4) & 0x01;
    aData.cw2_open = (tmp >> 5) & 0x01;
    aData.cw2_close = (tmp >> 6) & 0x01;
    tmp = 0;
    client_ -> MBRead(803,1,&tmp);
    aData.dd2_function = (tmp >> 3) & 0x01;
    

    tmp32 = 0;
    client_ -> MBRead(820,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.gas_concentration = tmp;
    tmp32 = 0;
    client_ -> MBRead(830,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    aData.ps_pressure = tmp;
    tmp32 = 0;
    client_ -> MBRead(840,4,&tmp32);
    tmp32 = S7_reverse_endianess(tmp32);
    aData.cw_amount = tmp32;

    return aData;
}


elecWellData Profinet::GetelecWellInfo(TS7Client* client_)
{
    elecWellData eData;
    eData.deviceType = "3-1";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(200,1,&tmp);
    //tmp = S7_reverse_endianess(tmp8);
    eData.running_state =  tmp &  0x01;
    eData.is_alarm = (tmp >> 1) & 0x01;
    eData.is_stop = (tmp >> 2) & 0x01;
    eData.hp_start = (tmp >> 4) & 0x01;
    eData.psrv_work = (tmp >> 5) & 0x01;
    
    client_ ->MBRead(201,1,&tmp);
    eData.mwp_manual = (tmp >> 2) & 0x01;
    eData.bwp_manual = (tmp >> 3) & 0x01;
    eData.mwp_function = (tmp >> 4) & 0x01;
    eData.bwp_function = (tmp >> 5) & 0x01;
    eData.dd_function = (tmp >> 6) & 0x01;

    client_ -> MBRead(202,1,&tmp);
    eData.connection_plug = (tmp >> 1) & 0x01;
    eData.ms_plug = (tmp >> 2) & 0x01;

    client_ -> MBRead(220,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    eData.gas_concentration = tmp;

    client_ -> MBRead(230,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    eData.ps_pressure = tmp;
    return eData;
}

elecWellData Profinet::GetelecWellDouInfo(TS7Client* client_)
{
    elecWellData eData;
    eData.deviceType = "3-2";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(700,1,&tmp);
    eData.running_state =  tmp &  0x01;
    eData.is_alarm = (tmp >> 1) & 0x01;
    eData.is_stop = (tmp >> 2) & 0x01;
    eData.hp_start = (tmp >> 4) & 0x01;
    eData.psrv_work = (tmp >> 5) & 0x01;
    
    client_ ->MBRead(701,1,&tmp);
    eData.mwp_manual = (tmp >> 2) & 0x01;
    eData.bwp_manual = (tmp >> 3) & 0x01;
    eData.mwp_function = (tmp >> 4) & 0x01;
    eData.bwp_function = (tmp >> 5) & 0x01;
    eData.dd_function = (tmp >> 6) & 0x01;

    client_ -> MBRead(702,1,&tmp);
    eData.connection_plug = (tmp >> 1) & 0x01;
    eData.ms_plug = (tmp >> 2) & 0x01;
    eData.dd2_function = (tmp >> 3) & 0x01;
    eData.connection2_plug = (tmp >> 6) & 0x01;
    eData.ms2_plug = (tmp >> 7) & 0x01;

    client_ -> MBRead(720,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    eData.gas_concentration = tmp;
    
    client_ -> MBRead(730,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    eData.ps_pressure = tmp;
    return eData;
}

sewageWellData Profinet::GetswgWellInfo(TS7Client* client_)
{
    sewageWellData sData;
    sData.deviceType = "5-1";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(400,1,&tmp);
    sData.running_state =  tmp &  0x01;
    sData.is_alarm = (tmp >> 1) & 0x01;
    sData.is_stop = (tmp >> 2) & 0x01;
    sData.hp_start = (tmp >> 4) & 0x01;
    sData.psrv_work = (tmp >> 5) & 0x01;
    
    client_ ->MBRead(401,1,&tmp);
    sData.mwp_manual = (tmp >> 2) & 0x01;
    sData.bwp_manual = (tmp >> 3) & 0x01;
    sData.mwp_function = (tmp >> 4) & 0x01;
    sData.bwp_function = (tmp >> 5) & 0x01;
    sData.dd_function = (tmp >> 6) & 0x01;
    
    client_ -> MBRead(402,1,&tmp);
    sData.rmv_function = (tmp >> 1) & 0x01;
    sData.sv_function = (tmp >> 2) & 0x01;
    sData.rwv_sv_function = (tmp >> 3) & 0x01;
    sData.rwv_sv_open = (tmp >> 4) & 0x01;
    sData.rwv_sv_close = (tmp >> 5) & 0x01;
    sData.sv_open = (tmp >> 6) & 0x01;
    sData.sv_close = (tmp >> 7) & 0x01;

    client_ -> MBRead(403,1,&tmp);
    sData.rwv_open = tmp & 0x01;
    sData.rwv_close = (tmp >> 1) & 0x01;

    
    client_ -> MBRead(420,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.gas_concentration = tmp;
    
    client_ -> MBRead(430,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.ps_pressure = tmp;
    tmp32 = 0;
    client_ -> MBRead(440,4,&tmp32);
    tmp32 = S32_reverse_endianess(tmp32);
    sData.rwv_amount = tmp32;

    return sData;
}

sewageWellData Profinet::GetswgWellLeftInfo(TS7Client *client_)
{
    sewageWellData sData;
    sData.deviceType = "5-1";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(400,1,&tmp);
    sData.running_state =  tmp &  0x01;
    sData.is_alarm = (tmp >> 1) & 0x01;
    sData.is_stop = (tmp >> 2) & 0x01;
    sData.hp_start = (tmp >> 4) & 0x01;
    sData.psrv_work = (tmp >> 5) & 0x01;
    
    client_ ->MBRead(401,1,&tmp);
    sData.mwp_manual = (tmp >> 2) & 0x01;
    sData.bwp_manual = (tmp >> 3) & 0x01;
    sData.mwp_function = (tmp >> 4) & 0x01;
    sData.bwp_function = (tmp >> 5) & 0x01;
    sData.dd_function = (tmp >> 6) & 0x01;
    
    client_ -> MBRead(402,1,&tmp);
    sData.rmv_function = (tmp >> 1) & 0x01;
    sData.sv_function = (tmp >> 2) & 0x01;
    sData.rwv_sv_function = (tmp >> 3) & 0x01;
    sData.rwv_sv_open = (tmp >> 4) & 0x01;
    sData.rwv_sv_close = (tmp >> 5) & 0x01;
    sData.sv_open = (tmp >> 6) & 0x01;
    sData.sv_close = (tmp >> 7) & 0x01;

    client_ -> MBRead(403,1,&tmp);
    sData.rwv_open = tmp & 0x01;
    sData.rwv_close = (tmp >> 1) & 0x01;

    tmp32 = 0;
    client_ -> MBRead(420,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.gas_concentration = tmp;
    tmp32 = 0;
    client_ -> MBRead(430,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.ps_pressure = tmp;
    tmp32 = 0;
    client_ -> MBRead(440,4,&tmp32);
    tmp32 = S32_reverse_endianess(tmp32);
    sData.rwv_amount = tmp32;

    return sData;
}

sewageWellData Profinet::GetswgWellMidInfo(TS7Client *client_)
{
    sewageWellData sData;
    sData.deviceType = "5-2";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(600,1,&tmp);
    sData.running_state =  tmp &  0x01;
    sData.is_alarm = (tmp >> 1) & 0x01;
    sData.is_stop = (tmp >> 2) & 0x01;
    sData.hp_start = (tmp >> 4) & 0x01;
    sData.psrv_work = (tmp >> 5) & 0x01;
    
    client_ ->MBRead(601,1,&tmp);
    sData.mwp_manual = (tmp >> 2) & 0x01;
    sData.bwp_manual = (tmp >> 3) & 0x01;
    sData.mwp_function = (tmp >> 4) & 0x01;
    sData.bwp_function = (tmp >> 5) & 0x01;
    sData.dd_function = (tmp >> 6) & 0x01;
    
    client_ -> MBRead(602,1,&tmp);
    sData.rmv_function = (tmp >> 1) & 0x01;
    sData.sv_function = (tmp >> 2) & 0x01;
    sData.rwv_sv_function = (tmp >> 3) & 0x01;
    sData.rwv_sv_open = (tmp >> 4) & 0x01;
    sData.rwv_sv_close = (tmp >> 5) & 0x01;
    sData.sv_open = (tmp >> 6) & 0x01;
    sData.sv_close = (tmp >> 7) & 0x01;

    client_ -> MBRead(603,1,&tmp);
    sData.rwv_open = tmp & 0x01;
    sData.rwv_close = (tmp >> 1) & 0x01;

    client_ -> MBRead(620,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.gas_concentration = tmp;
    
    client_ -> MBRead(630,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.ps_pressure = tmp;
    
    client_ -> MBRead(640,4,&tmp32);
    tmp32 = S32_reverse_endianess(tmp32);
    sData.rwv_amount = tmp32;

    return sData;
}
sewageWellData Profinet::GetswgWellRightInfo(TS7Client *client_)
{
    sewageWellData sData;
    sData.deviceType = "5-3";
    uint16_t tmp = 0;
    uint32_t tmp32;
    client_ ->MBRead(900,1,&tmp);
    sData.running_state =  tmp &  0x01;
    sData.is_alarm = (tmp >> 1) & 0x01;
    sData.is_stop = (tmp >> 2) & 0x01;
    sData.hp_start = (tmp >> 4) & 0x01;
    sData.psrv_work = (tmp >> 5) & 0x01;
    
    client_ ->MBRead(901,1,&tmp);
    sData.mwp_manual = (tmp >> 2) & 0x01;
    sData.bwp_manual = (tmp >> 3) & 0x01;
    sData.mwp_function = (tmp >> 4) & 0x01;
    sData.bwp_function = (tmp >> 5) & 0x01;
    sData.dd_function = (tmp >> 6) & 0x01;
    
    client_ -> MBRead(902,1,&tmp);
    sData.rmv_function = (tmp >> 1) & 0x01;
    sData.sv_function = (tmp >> 2) & 0x01;
    sData.rwv_sv_function = (tmp >> 3) & 0x01;
    sData.rwv_sv_open = (tmp >> 4) & 0x01;
    sData.rwv_sv_close = (tmp >> 5) & 0x01;
    sData.sv_open = (tmp >> 6) & 0x01;
    sData.sv_close = (tmp >> 7) & 0x01;

    client_ -> MBRead(903,1,&tmp);
    sData.rwv_open = tmp & 0x01;
    sData.rwv_close = (tmp >> 1) & 0x01;

    
    client_ -> MBRead(920,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.gas_concentration = tmp;
    
    client_ -> MBRead(930,2,&tmp);
    tmp = S7_reverse_endianess(tmp);
    sData.ps_pressure = tmp;
    
    client_ -> MBRead(940,4,&tmp32);
    tmp32 = S32_reverse_endianess(tmp32);
    sData.rwv_amount = tmp32;

    return sData;
}


powersupplyData Profinet::GetpowersupplyData(TS7Client *client_)
{
    powersupplyData psData;
    psData.deviceType = "1-1";
    uint16_t tmp;
    uint32_t tmp32;
    client_->DBRead(3,0,2,&tmp);
    psData.power_state = S7_reverse_endianess(tmp);
    client_->DBRead(3,2,2,&tmp);
    psData.alarm_code = S7_reverse_endianess(tmp);
    client_->DBRead(3,4,2,&tmp);
    psData.input_freq = S7_reverse_endianess(tmp);
    client_->DBRead(3,6,2,&tmp);
    psData.input_volt12 = S7_reverse_endianess(tmp);
    client_->DBRead(3,8,2,&tmp);
    psData.input_volt23 = S7_reverse_endianess(tmp);
    client_->DBRead(3,10,2,&tmp);
    psData.input_volt13 = S7_reverse_endianess(tmp);
    client_->DBRead(3,12,2,&tmp);
    psData.A_volt = S7_reverse_endianess(tmp);
    client_->DBRead(3,14,2,&tmp);
    psData.B_volt = S7_reverse_endianess(tmp);
    client_->DBRead(3,16,2,&tmp);
    psData.C_volt = S7_reverse_endianess(tmp);
    client_->DBRead(3,18,2,&tmp);
    psData.frequency = S7_reverse_endianess(tmp);
    client_->DBRead(3,20,2,&tmp);
    psData.A_curr = S7_reverse_endianess(tmp);
    client_->DBRead(3,22,2,&tmp);
    psData.B_curr = S7_reverse_endianess(tmp);
    client_->DBRead(3,24,2,&tmp);
    psData.C_curr = S7_reverse_endianess(tmp);
    client_->DBRead(3,26,2,&tmp);
    psData.Dc_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,28,2,&tmp);
    psData.Ac1_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,30,2,&tmp);
    psData.Ac2_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,32,2,&tmp);
    psData.Ac3_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,34,2,&tmp);
    psData.trans_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,36,4,&tmp32);
    psData.total_out_time =S32_reverse_endianess(tmp32);
    client_->DBRead(3,40,4,&tmp32);
    psData.total_out_power =S32_reverse_endianess(tmp32);
    return psData;
}

powersupplyData Profinet::Getpowersupply2Data(TS7Client *client_)
{
    powersupplyData psData;
    psData.deviceType = "1-2";
    uint16_t tmp;
    uint32_t tmp32;
    client_->DBRead(7,0,2,&tmp);
    psData.power_state = S7_reverse_endianess(tmp);
    client_->DBRead(7,2,2,&tmp);
    psData.alarm_code = S7_reverse_endianess(tmp);
    client_->DBRead(7,4,2,&tmp);
    psData.input_freq = S7_reverse_endianess(tmp);
    client_->DBRead(7,6,2,&tmp);
    psData.input_volt12 = S7_reverse_endianess(tmp);
    client_->DBRead(7,8,2,&tmp);
    psData.input_volt23 = S7_reverse_endianess(tmp);
    client_->DBRead(7,10,2,&tmp);
    psData.input_volt13 = S7_reverse_endianess(tmp);
    client_->DBRead(7,12,2,&tmp);
    psData.A_volt = S7_reverse_endianess(tmp);
    client_->DBRead(7,14,2,&tmp);
    psData.B_volt = S7_reverse_endianess(tmp);
    client_->DBRead(7,16,2,&tmp);
    psData.C_volt = S7_reverse_endianess(tmp);
    client_->DBRead(7,18,2,&tmp);
    psData.frequency = S7_reverse_endianess(tmp);
    client_->DBRead(7,20,2,&tmp);
    psData.A_curr = S7_reverse_endianess(tmp);
    client_->DBRead(7,22,2,&tmp);
    psData.B_curr = S7_reverse_endianess(tmp);
    client_->DBRead(7,24,2,&tmp);
    psData.C_curr = S7_reverse_endianess(tmp);
    client_->DBRead(7,26,2,&tmp);
    psData.Dc_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,28,2,&tmp);
    psData.Ac1_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,30,2,&tmp);
    psData.Ac2_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,32,2,&tmp);
    psData.Ac3_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,34,2,&tmp);
    psData.trans_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,36,4,&tmp32);
    psData.total_out_time =S32_reverse_endianess(tmp32);
    client_->DBRead(7,40,4,&tmp32);
    psData.total_out_power =S32_reverse_endianess(tmp32);
    return psData;
}

airconditionData Profinet::GetairconditionData(TS7Client *client_)
{
    airconditionData acData;
    acData.deviceType = "2-1";
    uint16_t tmp;
    uint32_t tmp32;

    client_ ->DBRead(3,101,1,&tmp);
    // client_ ->DBRead(3,100,1,&tmp);
    acData.air_state = tmp & 0x01;
    client_->DBRead(3,128,2,&tmp);
    acData.newair_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,130,2,&tmp);
    acData.splyair_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,132,2,&tmp);
    acData.set_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(3,134,2,&tmp);
    acData.splyair_pres = S7_reverse_endianess(tmp);
    client_->DBRead(3,136,2,&tmp);
    acData.sply_volume = S7_reverse_endianess(tmp);
    client_->DBRead(3,138,2,&tmp);
    acData.newair_spd = S7_reverse_endianess(tmp);
    client_->DBRead(3,140,2,&tmp);
    acData.splyair_spd = S7_reverse_endianess(tmp);
    client_->DBRead(3,142,2,&tmp);
    acData.cmpsor1_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,144,2,&tmp);
    acData.cmpsor1_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,146,2,&tmp);
    acData.cmpsor2_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,148,2,&tmp);
    acData.cmpsor2_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,150,2,&tmp);
    acData.cmpsor3_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,152,2,&tmp);
    acData.cmpsor3_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,154,2,&tmp);
    acData.cmpsor4_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,156,2,&tmp);
    acData.cmpsor4_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(3,112,2,&tmp);
    acData.cond1_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(3,114,2,&tmp);
    acData.cond2_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(3,116,2,&tmp);
    acData.cmpsor1_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(3,118,2,&tmp);
    acData.cmpsor2_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(3,120,2,&tmp);
    acData.cmpsor3_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(3,122,2,&tmp);
    acData.cmpsor4_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(3,124,2,&tmp);
    acData.oprtion_time_H = S7_reverse_endianess(tmp);
    client_->DBRead(3,126,2,&tmp);
    acData.oprtion_time_M = S7_reverse_endianess(tmp);
    client_->DBRead(3,108,2,&tmp);
    acData.unit_oprtion_mode = S7_reverse_endianess(tmp);
    return acData;
}

airconditionData Profinet::Getaircondition2Data(TS7Client *client_)
{
    airconditionData acData;
    acData.deviceType = "2-2";
    uint16_t tmp;
    uint32_t tmp32;

    client_ ->DBRead(7,131,1,&tmp);
    acData.air_state = tmp & 0x01;

    client_->DBRead(7,138,2,&tmp);
    acData.cond1_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,140,2,&tmp);
    acData.cond2_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,142,2,&tmp);
    acData.cond3_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,144,2,&tmp);
    acData.oprtion_time_H = S7_reverse_endianess(tmp);
    client_->DBRead(7,146,2,&tmp);
    acData.oprtion_time_M = S7_reverse_endianess(tmp);
    // client_ ->DBRead(3,100,1,&tmp);
    // acData.air_state = (tmp >> 1) & 0x01;
    client_->DBRead(7,148,2,&tmp);
    acData.newair_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,150,2,&tmp);
    acData.splyair_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,152,2,&tmp);
    acData.set_tmp = S7_reverse_endianess(tmp);
    client_->DBRead(7,154,2,&tmp);
    acData.splyair_pres = S7_reverse_endianess(tmp);
    client_->DBRead(7,156,2,&tmp);
    acData.sply_volume = S7_reverse_endianess(tmp);
    client_->DBRead(7,158,2,&tmp);
    acData.newair_spd = S7_reverse_endianess(tmp);
    client_->DBRead(7,160,2,&tmp);
    acData.splyair_spd = S7_reverse_endianess(tmp);
    client_->DBRead(7,162,2,&tmp);
    acData.cmpsor1_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,164,2,&tmp);
    acData.cmpsor1_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,166,2,&tmp);
    acData.cmpsor2_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,168,2,&tmp);
    acData.cmpsor2_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,170,2,&tmp);
    acData.cmpsor3_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,172,2,&tmp);
    acData.cmpsor3_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,174,2,&tmp);
    acData.cmpsor4_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,176,2,&tmp);
    acData.cmpsor4_low_ps = S7_reverse_endianess(tmp);

    client_->DBRead(7,178,2,&tmp);
    acData.cmpsor5_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,180,2,&tmp);
    acData.cmpsor5_low_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,182,2,&tmp);
    acData.cmpsor6_high_ps = S7_reverse_endianess(tmp);
    client_->DBRead(7,184,2,&tmp);
    acData.cmpsor6_low_ps = S7_reverse_endianess(tmp);


    
    client_->DBRead(7,186,2,&tmp);
    acData.cmpsor1_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,188,2,&tmp);
    acData.cmpsor2_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,190,2,&tmp);
    acData.cmpsor3_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,192,2,&tmp);
    acData.cmpsor4_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,194,2,&tmp);
    acData.cmpsor5_tt_time = S7_reverse_endianess(tmp);
    client_->DBRead(7,196,2,&tmp);
    acData.cmpsor6_tt_time = S7_reverse_endianess(tmp);

    client_->DBRead(7,198,2,&tmp);
    acData.airsply_tt_time = S7_reverse_endianess(tmp);


    
    client_->DBRead(7,200,2,&tmp);
    acData.unit_oprtion_mode = S7_reverse_endianess(tmp);
    return acData;
}

// wellDeviceData Profinet::GetmixDeviceEInfo(TS7Client *client_)
// {   
//     wellDeviceData dData;
//     uint16_t tmp;
//     uint32_t tmp32;
//     client_->DBRead(4,0,2,&tmp);
//     dData.power_state = S7_reverse_endianess(tmp);
//     client_->DBRead(4,2,2,&tmp);
//     dData.alarm_code = S7_reverse_endianess(tmp);
//     client_->DBRead(4,4,2,&tmp);
//     dData.input_freq = S7_reverse_endianess(tmp);
//     client_->DBRead(4,6,2,&tmp);
//     dData.input_volt12 = S7_reverse_endianess(tmp);
//     client_->DBRead(4,8,2,&tmp);
//     dData.input_volt23 = S7_reverse_endianess(tmp);
//     client_->DBRead(4,10,2,&tmp);
//     dData.input_volt13 = S7_reverse_endianess(tmp);
//     client_->DBRead(4,12,2,&tmp);
//     dData.A_volt = S7_reverse_endianess(tmp);
//     client_->DBRead(4,14,2,&tmp);
//     dData.B_volt = S7_reverse_endianess(tmp);
//     client_->DBRead(4,16,2,&tmp);
//     dData.C_volt = S7_reverse_endianess(tmp);
//     client_->DBRead(4,18,2,&tmp);
//     dData.frequency = S7_reverse_endianess(tmp);
//     client_->DBRead(4,20,2,&tmp);
//     dData.A_curr = S7_reverse_endianess(tmp);
//     client_->DBRead(4,22,2,&tmp);
//     dData.B_curr = S7_reverse_endianess(tmp);
//     client_->DBRead(4,24,2,&tmp);
//     dData.C_curr = S7_reverse_endianess(tmp);
//     client_->DBRead(4,26,2,&tmp);
//     dData.Dc_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,28,2,&tmp);
//     dData.Ac1_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,30,2,&tmp);
//     dData.Ac2_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,32,2,&tmp);
//     dData.Ac3_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,34,2,&tmp);
//     dData.trans_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,36,4,&tmp32);
//     dData.total_out_time =S32_reverse_endianess(tmp32);
//     client_->DBRead(4,40,4,&tmp32);
//     dData.total_out_power =S32_reverse_endianess(tmp32);
//     client_ ->DBRead(3,101,1,&tmp);
//     dData.air_state = tmp & 0x01;
//     client_->DBRead(4,148,2,&tmp);
//     dData.newair_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,150,2,&tmp);
//     dData.splyair_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,152,2,&tmp);
//     dData.set_tmp = S7_reverse_endianess(tmp);
//     client_->DBRead(4,154,2,&tmp);
//     dData.splyair_pres = S7_reverse_endianess(tmp);
//     client_->DBRead(4,156,2,&tmp);
//     dData.sply_volume = S7_reverse_endianess(tmp);
//     client_->DBRead(4,158,2,&tmp);
//     dData.newair_spd = S7_reverse_endianess(tmp);
//     client_->DBRead(4,160,2,&tmp);
//     dData.splyair_spd = S7_reverse_endianess(tmp);
//     client_->DBRead(4,162,2,&tmp);
//     dData.cmpsor1_high_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,164,2,&tmp);
//     dData.cmpsor1_low_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,166,2,&tmp);
//     dData.cmpsor2_high_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,168,2,&tmp);
//     dData.cmpsor2_low_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,170,2,&tmp);
//     dData.cmpsor3_high_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,172,2,&tmp);
//     dData.cmpsor3_low_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,174,2,&tmp);
//     dData.cmpsor4_high_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,176,2,&tmp);
//     dData.cmpsor4_low_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,178,2,&tmp);
//     dData.cmpsor5_high_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,180,2,&tmp);
//     dData.cmpsor5_low_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,182,2,&tmp);
//     dData.cmpsor6_high_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,184,2,&tmp);
//     dData.cmpsor6_low_ps = S7_reverse_endianess(tmp);
//     client_->DBRead(4,138,2,&tmp);
//     dData.cond1_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,140,2,&tmp);
//     dData.cond2_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,142,2,&tmp);
//     dData.cond3_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,186,2,&tmp);
//     dData.cmpsor1_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,188,2,&tmp);
//     dData.cmpsor2_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,190,2,&tmp);
//     dData.cmpsor3_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,192,2,&tmp);
//     dData.cmpsor4_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,194,2,&tmp);
//     dData.cmpsor5_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,196,2,&tmp);
//     dData.cmpsor6_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,198,2,&tmp);
//     dData.airsply_tt_time = S7_reverse_endianess(tmp);
//     client_->DBRead(4,144,2,&tmp);
//     dData.oprtion_time_H = S7_reverse_endianess(tmp);
//     client_->DBRead(4,146,2,&tmp);
//     dData.oprtion_time_M = S7_reverse_endianess(tmp);
//     client_->DBRead(4,200,2,&tmp);
//     dData.unit_oprtion_mode = S7_reverse_endianess(tmp);


//     return dData;
// }

sewageStationData Profinet::GetsewageStationInfo(TS7Client *client_)
{
    sewageStationData swgdata;
    uint16_t tmp = 0;
    uint32_t tmp32 = 0;
    client_ ->DBRead(1,0,1,&tmp);
    swgdata.ejector1_auto =  tmp &  0x01;
    swgdata.ejector2_auto = (tmp >> 1) & 0x01;
    swgdata.ejector3_auto = (tmp >> 2) & 0x01;
    swgdata.spare1 = (tmp >> 3) & 0x01;
    swgdata.dc_y0_auto = (tmp >> 4) & 0x01;
    swgdata.dc_y1_auto = (tmp >> 5) & 0x01;
    swgdata.dc_time_switch = (tmp >> 6) & 0x01;
    swgdata.dc_level_switch = (tmp >> 7) & 0x01;
    client_ ->DBRead(1,1,1,&tmp);
    swgdata.antipump_auto =  tmp &  0x01;
    swgdata.alarm_reset = (tmp >> 1) & 0x01;
    swgdata.low_level_bypass = (tmp >> 2) & 0x01;
    swgdata.ejector1_overload = (tmp >> 3) & 0x01;
    swgdata.ejector2_overload = (tmp >> 4) & 0x01;
    swgdata.ejector3_overload = (tmp >> 5) & 0x01;
    swgdata.spare2 = (tmp >> 6) & 0x01;
    swgdata.antifoam_pump_overload = (tmp >> 7) & 0x01;
    client_ ->DBRead(1,2,1,&tmp);
    swgdata.dc_allowed =  tmp &  0x01;
    swgdata.antipump_fail = (tmp >> 1) & 0x01;
    client_ ->DBRead(1,8,1,&tmp);
    swgdata.ejector1_run =  tmp &  0x01;
    swgdata.ejector2_run = (tmp >> 1) & 0x01;
    swgdata.ejector3_run = (tmp >> 2) & 0x01;
    swgdata.spare3 = (tmp >> 3) & 0x01;
    swgdata.antiform_run = (tmp >> 4) & 0x01;
    swgdata.udvolt_trip_reset = (tmp >> 5) & 0x01;
    swgdata.dc_value1_close = (tmp >> 6) & 0x01;
    swgdata.dc_value2_close = (tmp >> 7) & 0x01;
    client_ ->DBRead(1,10,1,&tmp);
    swgdata.common_alarm =  tmp &  0x01;
    swgdata.ejector_pump1_control = (tmp >> 1) & 0x01;
    swgdata.ejector_pump2_control = (tmp >> 2) & 0x01;
    swgdata.ejector_pump3_control = (tmp >> 3) & 0x01;
    swgdata.spare4 = (tmp >> 4) & 0x01;
    swgdata.antipump_control = (tmp >> 5) & 0x01;
    swgdata.dc_value1_control = (tmp >> 6) & 0x01;
    swgdata.dc_value2_control = (tmp >> 7) & 0x01;
    client_ ->DBRead(1,11,1,&tmp);
    swgdata.option_ll_level_fault =  tmp &  0x01;
    swgdata.vac_failure = (tmp >> 1) & 0x01;
    swgdata.vac_collpse = (tmp >> 2) & 0x01;
    swgdata.pump_stop_level = (tmp >> 3) & 0x01;
    client_ ->DBRead(1,12,1,&tmp);
    swgdata.spare5 =  tmp &  0x01;
    swgdata.pump_start_level = (tmp >> 1) & 0x01;
    swgdata.highlever_fault = (tmp >> 2) & 0x01;
    swgdata.dc_stp_blocked = (tmp >> 3) & 0x01;
    swgdata.antifoam_pump_overload = (tmp >> 4) & 0x01;
    swgdata.antifoam_tank_lowlevel = (tmp >> 5) & 0x01;
    swgdata.Y01_failure = (tmp >> 6) & 0x01;
    swgdata.Y02_failure = (tmp >> 7) & 0x01;
    client_ ->DBRead(1,20,2,&tmp);
    swgdata.sensor_raw_value = S7_reverse_endianess(tmp);
    client_ ->DBRead(1,22,2,&tmp);
    swgdata.vac_ps1_value = S7_reverse_endianess(tmp);
    client_ ->DBRead(1,24,2,&tmp);
    swgdata.vac_ps2_value = S7_reverse_endianess(tmp);
    client_ ->DBRead(1,30,4,&tmp32);
    swgdata.liquid_level_eng_value = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,34,4,&tmp32);
    swgdata.vac_ps1_value = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,38,4,&tmp32);
    swgdata.vac_ps2_value = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,42,1,&tmp);
    swgdata.tankbody_ll_liquid_level =  tmp &  0x01;
    swgdata.tank_l_liquid_level = (tmp >> 1) & 0x01;
    swgdata.tank_h_liquid_level = (tmp >> 2) & 0x01;
    swgdata.tank_hh_liquid_level = (tmp >> 3) & 0x01;
    client_ ->DBRead(1,44,4,&tmp32);
    swgdata.dc_pump1_running = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,48,4,&tmp32);
    swgdata.dc_pump1_fault = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,52,4,&tmp32);
    swgdata.dc_pump2_running = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,56,4,&tmp32);
    swgdata.dc_pump2_fault = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,60,4,&tmp32);
    swgdata.dc_pump3_running = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,64,4,&tmp32);
    swgdata.dc_pump3_fault = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,68,4,&tmp32);
    swgdata.dc_pump1_running_nums = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,72,4,&tmp32);
    swgdata.dc_pump1_running_hours = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,76,4,&tmp32);
    swgdata.dc_pump1_running_mins = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,80,4,&tmp32);
    swgdata.dc_pump1_running_secs = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,84,4,&tmp32);
    swgdata.dc_pump2_running_nums = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,88,4,&tmp32);
    swgdata.dc_pump2_running_hours = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,92,4,&tmp32);
    swgdata.dc_pump2_running_mins = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,96,4,&tmp32);
    swgdata.dc_pump2_running_secs = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,100,4,&tmp32);
    swgdata.dc_pump3_running_nums = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,104,4,&tmp32);
    swgdata.dc_pump3_running_hours = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,108,4,&tmp32);
    swgdata.dc_pump3_running_mins = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,112,4,&tmp32);
    swgdata.dc_pump3_running_secs = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,116,4,&tmp32);
    swgdata.dc_valve1_running_nums = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,120,4,&tmp32);
    swgdata.dc_valve1_running_hours = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,124,4,&tmp32);
    swgdata.dc_valve1_running_mins = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,128,4,&tmp32);
    swgdata.dc_valve1_running_secs = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,132,4,&tmp32);
    swgdata.dc_valve2_running_nums = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,136,4,&tmp32);
    swgdata.dc_valve2_running_hours = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,140,4,&tmp32);
    swgdata.dc_valve2_running_mins = S32_reverse_endianess(tmp32);
    client_ ->DBRead(1,144,4,&tmp32);
    swgdata.dc_valve2_running_secs = S32_reverse_endianess(tmp32);
    return swgdata;
}

cleanwaterStationData Profinet::GetcleanwaterStationInfo(TS7Client *client_)
{   
    cleanwaterStationData cwData;
    uint16_t tmp16;
    uint32_t tmp32;
    byte tmpf[4]= {0};

    // client_ ->DBRead(24,1,1,&tmp16);
    // cwData.running_signal_1 = (tmp16 >> 2) & 0x01;
    client_->DBRead(210,32,4,&tmpf);
    cwData.frequency_1 = Float_reverse_endianess(tmpf);
    client_->DBRead(500,12,4,&tmp32);
    cwData.totaltime_h_1 = S32_reverse_endianess(tmp32);
    client_->DBRead(500,10,2,&tmp16);
    cwData.totaltime_m_1 = S7_reverse_endianess(tmp16);
    client_->DBRead(500,8,2,&tmp16);
    cwData.totaltime_s_1 = S7_reverse_endianess(tmp16);
    //client_->
    //cwData.running_signal_2 = 
    client_->DBRead(210,36,4,&tmpf);
    cwData.frequency_2 = Float_reverse_endianess(tmpf);
    client_->DBRead(500,20,4,&tmp32);
    cwData.totaltime_h_2 = S32_reverse_endianess(tmp32);
    client_->DBRead(500,18,2,&tmp16);
    cwData.totaltime_m_2 = S7_reverse_endianess(tmp16);
    client_->DBRead(500,16,2,&tmp16);
    cwData.totaltime_s_2 = S7_reverse_endianess(tmp16);

    client_->DBRead(210,8,4,&tmpf);
    cwData.watertank1_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(210,12,4,&tmpf);
    cwData.watertank2_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(210,0,4,&tmpf);
    cwData.outlet1_prs = S32_reverse_endianess(tmp32);
    client_->DBRead(210,4,4,&tmpf);
    cwData.outlet2_prs = S32_reverse_endianess(tmp32);

    return cwData;
}

cleanwaterStationData Profinet::GetcleanwaterStation2Info(TS7Client *client_)
{
    cleanwaterStationData cwData;
    uint16_t tmp16;
    uint32_t tmp32;
    byte tmpf[4]= {0};
    client_ ->DBRead(24,1,1,&tmp16);
    cwData.running_signal_1 = (tmp16 >> 2) & 0x01;
    client_->DBRead(24,8,4,&tmpf);
    cwData.frequency_1 = Float_reverse_endianess(tmpf);
    client_->DBRead(24,16,4,&tmp32);
    cwData.totaltime_h_1 = S32_reverse_endianess(tmp32);
    client_->DBRead(24,20,2,&tmp16);
    cwData.totaltime_m_1 = S7_reverse_endianess(tmp16);
    client_->DBRead(24,22,2,&tmp16);
    cwData.totaltime_s_1 = S7_reverse_endianess(tmp16);

    client_ ->DBRead(24,2,1,&tmp16);
    cwData.running_signal_2 = (tmp16 >> 2) & 0x01;
    client_->DBRead(24,26,4,&tmpf);
    cwData.frequency_2 = Float_reverse_endianess(tmpf);
    client_->DBRead(24,34,4,&tmp32);
    cwData.totaltime_h_2 = S32_reverse_endianess(tmp32);
    client_->DBRead(24,38,2,&tmp16);
    cwData.totaltime_m_2 = S7_reverse_endianess(tmp16);
    client_->DBRead(24,40,2,&tmp16);
    cwData.totaltime_s_2 = S7_reverse_endianess(tmp16);

    client_->DBRead(24,84,4,&tmpf);
    cwData.watertank1_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(24,88,4,&tmpf);
    cwData.watertank2_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(24,92,4,&tmpf);
    cwData.outlet1_prs = S32_reverse_endianess(tmp32);
    client_->DBRead(24,96,4,&tmpf);
    cwData.outlet2_prs = S32_reverse_endianess(tmp32);
    return cwData;
}
flushwaterStationData Profinet::GetflushwaterStationInfo(TS7Client *client_)
{   flushwaterStationData fwData;
    uint16_t tmp16;
    uint32_t tmp32;
    byte tmpf[4]= {0};
    //client_->
    //fwData.running_signal_1 = 
    client_->DBRead(210,32,4,&tmpf);
    fwData.frequency_1 = Float_reverse_endianess(tmpf);
    client_->DBRead(500,12,4,&tmp32);
    fwData.totaltime_h_1 = S32_reverse_endianess(tmp32);
    client_->DBRead(500,10,2,&tmp16);
    fwData.totaltime_m_1 = S7_reverse_endianess(tmp16);
    client_->DBRead(500,8,2,&tmp16);
    fwData.totaltime_s_1 = S7_reverse_endianess(tmp16);
    //client_->
    //fwData.running_signal_2 = 
    client_->DBRead(210,36,4,&tmpf);
    fwData.frequency_2 = Float_reverse_endianess(tmpf);
    client_->DBRead(500,20,4,&tmp32);
    fwData.totaltime_h_2 = S32_reverse_endianess(tmp32);
    client_->DBRead(500,18,2,&tmp16);
    fwData.totaltime_m_2 = S7_reverse_endianess(tmp16);
    client_->DBRead(500,16,2,&tmp16);
    fwData.totaltime_s_2 = S7_reverse_endianess(tmp16);
    
    //client_->
    //fwData.medrunning_signal_1 = 
    client_->DBRead(210,40,4,&tmpf);
    fwData.medfrequency_1 = Float_reverse_endianess(tmpf);
    client_->DBRead(500,28,4,&tmp32);
    fwData.medtotaltime_h_1 = S32_reverse_endianess(tmp32);
    client_->DBRead(500,26,2,&tmp16);
    fwData.medtotaltime_m_1 = S7_reverse_endianess(tmp16);
    client_->DBRead(500,24,2,&tmp16);
    fwData.medtotaltime_s_1 = S7_reverse_endianess(tmp16);
    //client_->
    //fwData.medrunning_signal_2 = 
    client_->DBRead(210,44,4,&tmpf);
    fwData.medfrequency_2 = Float_reverse_endianess(tmpf);
    client_->DBRead(500,36,4,&tmp32);
    fwData.medtotaltime_h_2 = S32_reverse_endianess(tmp32);
    client_->DBRead(500,34,2,&tmp16);
    fwData.medtotaltime_m_2 = S7_reverse_endianess(tmp16);
    client_->DBRead(500,32,2,&tmp16);
    fwData.medtotaltime_s_2 = S7_reverse_endianess(tmp16);
    


    client_->DBRead(210,16,4,&tmpf);
    fwData.medtank_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(210,8,4,&tmpf);
    fwData.medwatertank1_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(210,12,4,&tmpf);
    fwData.medwatertank2_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(210,0,4,&tmpf);
    fwData.medoutlet1_prs = Float_reverse_endianess(tmpf);
    client_->DBRead(210,4,4,&tmpf);
    fwData.medoutlet2_prs = Float_reverse_endianess(tmpf);

    return fwData;
}

flushwaterStationData Profinet::GetflushwaterStation2Info(TS7Client *client_)
{
    flushwaterStationData fwData;
    uint16_t tmp16;
    uint32_t tmp32;
    byte tmpf[4]= {0};

    client_ ->DBRead(24,1,1,&tmp16);
    fwData.running_signal_1 = (tmp16 >> 2) & 0x01;
    client_->DBRead(24,8,4,&tmpf);
    fwData.frequency_1 = Float_reverse_endianess(tmpf);
    client_->DBRead(24,16,4,&tmp32);
    fwData.totaltime_h_1 = S32_reverse_endianess(tmp32);
    client_->DBRead(24,20,2,&tmp16);
    fwData.totaltime_m_1 = S7_reverse_endianess(tmp16);
    client_->DBRead(24,22,2,&tmp16);
    fwData.totaltime_s_1 = S7_reverse_endianess(tmp16);

    client_ ->DBRead(24,2,1,&tmp16);
    fwData.running_signal_2 = (tmp16 >> 2) & 0x01;
    client_->DBRead(24,26,4,&tmpf);
    fwData.frequency_2 = Float_reverse_endianess(tmpf);
    client_->DBRead(24,34,4,&tmp32);
    fwData.totaltime_h_2 = S32_reverse_endianess(tmp32);
    client_->DBRead(24,38,2,&tmp16);
    fwData.totaltime_m_2 = S7_reverse_endianess(tmp16);
    client_->DBRead(24,40,2,&tmp16);
    fwData.totaltime_s_2 = S7_reverse_endianess(tmp16);
    
    client_->DBRead(24,3,1,&tmp16);
    fwData.medrunning_signal_1 = (tmp16 >> 2) & 0x01;
    client_->DBRead(24,44,4,&tmpf);
    fwData.medfrequency_1 = Float_reverse_endianess(tmpf);
    client_->DBRead(24,52,4,&tmp32);
    fwData.medtotaltime_h_1 = S32_reverse_endianess(tmp32);
    client_->DBRead(24,56,2,&tmp16);
    fwData.medtotaltime_m_1 = S7_reverse_endianess(tmp16);
    client_->DBRead(24,58,2,&tmp16);
    fwData.medtotaltime_s_1 = S7_reverse_endianess(tmp16);

    client_->DBRead(24,4,1,&tmp16);
    fwData.medrunning_signal_2 = (tmp16 >> 2) & 0x01;
    client_->DBRead(24,62,4,&tmpf);
    fwData.medfrequency_2 = Float_reverse_endianess(tmpf);
    // client_->DBRead(24,70,4,&tmp32);
    client_->DBRead(12,28,4,&tmp32);
    fwData.medtotaltime_h_2 = S32_reverse_endianess(tmp32);
    client_->DBRead(24,74,2,&tmp16);
    fwData.medtotaltime_m_2 = S7_reverse_endianess(tmp16);
    client_->DBRead(24,76,2,&tmp16);
    fwData.medtotaltime_s_2 = S7_reverse_endianess(tmp16);
    

    client_->DBRead(12,16,4,&tmpf);
    fwData.medtank_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(12,20,4,&tmpf);
    fwData.watertank1_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(12,24,4,&tmpf);
    fwData.watertank2_lvl = Float_reverse_endianess(tmpf);
    client_->DBRead(24,92,4,&tmpf);
    fwData.outlet1_prs = S32_reverse_endianess(tmp32);
    client_->DBRead(24,96,4,&tmpf);
    fwData.outlet2_prs = S32_reverse_endianess(tmp32);

    return fwData;
}

AirwellFaultData Profinet::GetsingleAirwellFaultData(TS7Client *client_)
{
    AirwellFaultData sawfData;
    sawfData.deviceType = "4-1";
    uint16_t tmp;
    client_ ->MBRead(300,1,&tmp);
    sawfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sawfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sawfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(301,1,&tmp);
    sawfData.datamap["积水报警"] = tmp & 0x01;
    sawfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["收送装置1上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(302,1,&tmp);
    sawfData.datamap["收送装置1下限"] = tmp & 0x01;
    sawfData.datamap["清水阀1故障"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(303,1,&tmp);
    sawfData.datamap["清水阀2故障"] = tmp & 0x01;
    sawfData.datamap["气体报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["泵站超压"] = (tmp >> 2) & 0x01;

    return sawfData;
}

AirwellFaultData Profinet::GetMixAirwellLeftFaultData(TS7Client *client_)
{
    AirwellFaultData sawfData;
    sawfData.deviceType = "4-1";
    uint16_t tmp;
    client_ ->MBRead(300,1,&tmp);
    sawfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sawfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sawfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(301,1,&tmp);
    sawfData.datamap["积水报警"] = tmp & 0x01;
    sawfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["收送装置1上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(302,1,&tmp);
    sawfData.datamap["收送装置1下限"] = tmp & 0x01;
    sawfData.datamap["清水阀1故障"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(303,1,&tmp);
    sawfData.datamap["清水阀2故障"] = tmp & 0x01;
    sawfData.datamap["气体报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["泵站超压"] = (tmp >> 2) & 0x01;
    sawfData.datamap["收送装置2上限"] = (tmp >> 4) & 0x01;
    sawfData.datamap["收送装置2下限"] = (tmp >> 5) & 0x01;

    return sawfData;
}

AirwellFaultData Profinet::GetMixAirwellMidFaultData(TS7Client *client_)
{
    AirwellFaultData sawfData;
    sawfData.deviceType = "4-2";
    uint16_t tmp;
    client_ ->MBRead(500,1,&tmp);
    sawfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sawfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sawfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(501,1,&tmp);
    sawfData.datamap["积水报警"] = tmp & 0x01;
    sawfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["收送装置1上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(502,1,&tmp);
    sawfData.datamap["收送装置1下限"] = tmp & 0x01;
    sawfData.datamap["清水阀1故障"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(503,1,&tmp);
    sawfData.datamap["清水阀2故障"] = tmp & 0x01;
    sawfData.datamap["气体报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["泵站超压"] = (tmp >> 2) & 0x01;
    sawfData.datamap["收送装置2上限"] = (tmp >> 4) & 0x01;
    sawfData.datamap["收送装置2下限"] = (tmp >> 5) & 0x01;

    return sawfData;
}

AirwellFaultData Profinet::GetMixAirwellRightFaultData(TS7Client *client_)
{
    AirwellFaultData sawfData;
    sawfData.deviceType = "4-3";
    uint16_t tmp;
    client_ ->MBRead(800,1,&tmp);
    sawfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sawfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sawfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(801,1,&tmp);
    sawfData.datamap["积水报警"] = tmp & 0x01;
    sawfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["收送装置1上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(802,1,&tmp);
    sawfData.datamap["收送装置1下限"] = tmp & 0x01;
    sawfData.datamap["清水阀1故障"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(803,1,&tmp);
    sawfData.datamap["清水阀2故障"] = tmp & 0x01;
    sawfData.datamap["气体报警"] = (tmp >> 1) & 0x01;
    sawfData.datamap["泵站超压"] = (tmp >> 2) & 0x01;
    sawfData.datamap["收送装置2上限"] = (tmp >> 4) & 0x01;
    sawfData.datamap["收送装置2下限"] = (tmp >> 5) & 0x01;

    return sawfData;
}

ElecwellFaultData Profinet::GetsingleElecwellFaultData(TS7Client *client_)
{
    ElecwellFaultData sewfData;
    uint16_t tmp;
    sewfData.deviceType = "3-1";
    client_ ->MBRead(200,1,&tmp);
    sewfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sewfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sewfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(201,1,&tmp);
    sewfData.datamap["积水报警"] = tmp & 0x01;
    sewfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sewfData.datamap["收送装置上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(202,1,&tmp);
    sewfData.datamap["收送装置下限"] = tmp & 0x01;
    sewfData.datamap["真空低限"] = (tmp >> 3) & 0x01;
    sewfData.datamap["气体报警"] = (tmp >> 6) & 0x01;
    sewfData.datamap["泵站超压"] = (tmp >> 7) & 0x01;


    return sewfData;
}

ElecwellFaultData Profinet::GetMixElecwellFaultData(TS7Client *client_)
{
    ElecwellFaultData sewfData;
    uint16_t tmp;
    sewfData.deviceType = "3-2";
    client_ ->MBRead(700,1,&tmp);
    sewfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sewfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sewfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(701,1,&tmp);
    sewfData.datamap["积水报警"] = tmp & 0x01;
    sewfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sewfData.datamap["收送装置1上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(702,1,&tmp);
    sewfData.datamap["收送装置1下限"] = tmp & 0x01;
    sewfData.datamap["收送装置2上限"] = (tmp >> 4) & 0x01;
    sewfData.datamap["收送装置2下限"] = (tmp >> 5) & 0x01;

    client_ ->MBRead(703,1,&tmp);
    sewfData.datamap["气体报警"] = tmp & 0x01;
    sewfData.datamap["泵站超压"] = (tmp >> 1) & 0x01;


    return sewfData;
}

SwgwellFaultData Profinet::GetsingleSwgwellFaultData(TS7Client *client_)
{   
    SwgwellFaultData sswfData;
    uint16_t tmp;
    sswfData.deviceType = "5-1";
    client_ ->MBRead(400,1,&tmp);
    sswfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sswfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(401,1,&tmp);
    sswfData.datamap["积水报警"] = tmp & 0x01;
    sswfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sswfData.datamap["收送装置上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(402,1,&tmp);
    sswfData.datamap["收送装置下限"] = tmp & 0x01;

    client_ -> MBRead(403,1,&tmp);
    sswfData.datamap["冲洗水转污水阀故障"] = (tmp >> 2) & 0x01;
    sswfData.datamap["污水阀故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["冲洗水阀故障"] = (tmp >> 4) & 0x01;
    sswfData.datamap["真空低限"] = (tmp >> 5) & 0x01;
    sswfData.datamap["气体报警"] = (tmp >> 6) & 0x01;
    sswfData.datamap["泵站超压"] = (tmp >> 7) & 0x01;


    return sswfData;
}

SwgwellFaultData Profinet::GetMixSwgwellFaultLeftData(TS7Client *client_)
{
    SwgwellFaultData sswfData;
    uint16_t tmp;
    sswfData.deviceType = "5-1";
    client_ ->MBRead(400,1,&tmp);
    sswfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sswfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(401,1,&tmp);
    sswfData.datamap["积水报警"] = tmp & 0x01;
    sswfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sswfData.datamap["收送装置上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(402,1,&tmp);
    sswfData.datamap["收送装置下限"] = tmp & 0x01;

    client_ -> MBRead(403,1,&tmp);
    sswfData.datamap["冲洗水转污水阀故障"] = (tmp >> 2) & 0x01;
    sswfData.datamap["污水阀故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["冲洗水阀故障"] = (tmp >> 4) & 0x01;
    sswfData.datamap["真空低限"] = (tmp >> 5) & 0x01;
    sswfData.datamap["气体报警"] = (tmp >> 6) & 0x01;
    sswfData.datamap["泵站超压"] = (tmp >> 7) & 0x01;


    return sswfData;
}

SwgwellFaultData Profinet::GetMixSwgwellFaultMidData(TS7Client *client_)
{
    SwgwellFaultData sswfData;
    uint16_t tmp;
    sswfData.deviceType = "5-2";
    client_ ->MBRead(600,1,&tmp);
    sswfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sswfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(601,1,&tmp);
    sswfData.datamap["积水报警"] = tmp & 0x01;
    sswfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sswfData.datamap["收送装置上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(602,1,&tmp);
    sswfData.datamap["收送装置下限"] = tmp & 0x01;

    client_ -> MBRead(603,1,&tmp);
    sswfData.datamap["冲洗水转污水阀故障"] = (tmp >> 2) & 0x01;
    sswfData.datamap["污水阀故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["冲洗水阀故障"] = (tmp >> 4) & 0x01;
    sswfData.datamap["真空低限"] = (tmp >> 5) & 0x01;
    sswfData.datamap["气体报警"] = (tmp >> 6) & 0x01;
    sswfData.datamap["泵站超压"] = (tmp >> 7) & 0x01;


    return sswfData;
}

SwgwellFaultData Profinet::GetMixSwgwellFaultRightData(TS7Client *client_)
{
    SwgwellFaultData sswfData;
    uint16_t tmp;
    sswfData.deviceType = "5-3";
    client_ ->MBRead(900,1,&tmp);
    sswfData.datamap["液压站故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["轴流风机故障"] = (tmp >> 6) & 0x01;
    sswfData.datamap["水泵故障"] = (tmp >> 7) & 0x01;

    client_ ->MBRead(901,1,&tmp);
    sswfData.datamap["积水报警"] = tmp & 0x01;
    sswfData.datamap["缺相报警"] = (tmp >> 1) & 0x01;
    sswfData.datamap["收送装置上限"] = (tmp >> 7) & 0x01;

    client_ -> MBRead(902,1,&tmp);
    sswfData.datamap["收送装置下限"] = tmp & 0x01;

    client_ -> MBRead(903,1,&tmp);
    sswfData.datamap["冲洗水转污水阀故障"] = (tmp >> 2) & 0x01;
    sswfData.datamap["污水阀故障"] = (tmp >> 3) & 0x01;
    sswfData.datamap["冲洗水阀故障"] = (tmp >> 4) & 0x01;
    sswfData.datamap["真空低限"] = (tmp >> 5) & 0x01;
    sswfData.datamap["气体报警"] = (tmp >> 6) & 0x01;
    sswfData.datamap["泵站超压"] = (tmp >> 7) & 0x01;


    return sswfData;
}

PowerFaultData Profinet::GetsinglePowerFaultData(TS7Client *client_)
{
    PowerFaultData spfData;
    uint16_t tmp;
    spfData.deviceType = "1-1";
    client_->DBRead(3,2,2,&tmp);
    spfData.datamap["pow1_alarm_code"] = S7_reverse_endianess(tmp);

    return spfData;
}

PowerFaultData Profinet::GetDouPowerFaultData(TS7Client *client_)
{
    PowerFaultData spfData;
    uint16_t tmp;
    spfData.deviceType = "1-2";
    client_->DBRead(4,2,2,&tmp);
    spfData.datamap["pow1_alarm_code"] = S7_reverse_endianess(tmp);

    return spfData;
}

AirCondFaultData Profinet::GetsingleCondFaultData(TS7Client *client_)
{
    AirCondFaultData scfData;
    uint16_t tmp;
    scfData.deviceType = "2-1";
    client_ ->DBRead(3,102,1,&tmp);
    scfData.datamap["电源故障"] = (tmp >> 4) & 0x01;
    scfData.datamap["送风机过载"] = (tmp >> 5) & 0x01;
    scfData.datamap["火灾报警"] = (tmp >> 7) & 0x01;

    client_ ->DBRead(3,104,1,&tmp);
    scfData.datamap["送风机检修时间到"] = tmp & 0x01;
    scfData.datamap["电加热过热报警"] = (tmp >> 1) & 0x01;
    scfData.datamap["送风温度过高报警"] = (tmp >> 2) & 0x01;
    scfData.datamap["烟雾传感器报警"] = (tmp >> 3) & 0x01;
    scfData.datamap["冷凝风机过载"] = (tmp >> 7) & 0x01;

    client_ ->DBRead(3,105,1,&tmp);
    scfData.datamap["风速传感器失灵"] = (tmp >> 0) & 0x01;
    scfData.datamap["风压传感器失灵"] = (tmp >> 1) & 0x01;
    scfData.datamap["送风温度传感器失灵"] = (tmp >> 2) & 0x01;
    scfData.datamap["新风温度传感器失灵"] = (tmp >> 3) & 0x01; 
    scfData.datamap["压缩机4检修时间到"] = (tmp >> 4) & 0x01;
    scfData.datamap["压缩机3检修时间到"] = (tmp >> 5) & 0x01;
    scfData.datamap["压缩机2检修时间到"] = (tmp >> 6) & 0x01;
    scfData.datamap["压缩机1检修时间到"] = (tmp >> 7) & 0x01;
    return scfData;
}

AirCondFaultData Profinet::GetDouCondFaultData(TS7Client *client_)
{
    AirCondFaultData scfData;
    uint16_t tmp;
    scfData.deviceType = "2-2";
    client_ ->DBRead(7,126,1,&tmp);
    scfData.datamap["送风机检修时间到"] = tmp & 0x01;
    scfData.datamap["电加热过热报警"] = (tmp >> 1) & 0x01;
    scfData.datamap["送风温度过高报警"] = (tmp >> 2) & 0x01;
    scfData.datamap["烟雾传感器报警"] = (tmp >> 3) & 0x01;
    scfData.datamap["电源故障"] = (tmp >> 4) & 0x01;
    scfData.datamap["送风机过载"] = (tmp >> 5) & 0x01;
    scfData.datamap["火灾报警"] = (tmp >> 6) & 0x01;
    scfData.datamap["冷凝风机过载"] = (tmp >> 7) & 0x01;
    // client_ ->DBRead(3,100,1,&tmp);
    // scfData.datamap["烟雾传感器报警"] = (tmp >> 2) & 0x01;
    client_ ->DBRead(7,127,1,&tmp);
    scfData.datamap["风速传感器失灵"] = (tmp >> 0) & 0x01;
    scfData.datamap["风压传感器失灵"] = (tmp >> 1) & 0x01;
    scfData.datamap["送风温度传感器失灵"] = (tmp >> 2) & 0x01;
    scfData.datamap["新风温度传感器失灵"] = (tmp >> 3) & 0x01;
    scfData.datamap["压缩机4检修时间到"] = (tmp >> 4) & 0x01;
    scfData.datamap["压缩机3检修时间到"] = (tmp >> 5) & 0x01;
    scfData.datamap["压缩机2检修时间到"] = (tmp >> 6) & 0x01;
    scfData.datamap["压缩机1检修时间到"] = (tmp >> 7) & 0x01;

    client_ ->DBRead(7,128,1,&tmp);
    scfData.datamap["压缩机5检修时间到"] = (tmp >> 6) & 0x01;
    scfData.datamap["压缩机6检修时间到"] = (tmp >> 7) & 0x01;

    client_ ->DBRead(7,131,1,&tmp);
    scfData.datamap["机组故障"] = (tmp >> 1) & 0x01;

    return scfData;
}
volumeInfoData Profinet::GetMixvolumeData(TS7Client *client_)
{
    volumeInfoData vData;
    byte tmpf[4]= {0};
    client_->DBRead(13,20,4,&tmpf);
    vData.power_consumption1  = Float_reverse_endianess(tmpf);
    client_->DBRead(13,16,4,&tmpf);
    vData.power_duration1 = Float_reverse_endianess(tmpf);

    client_->DBRead(13,44,4,&tmpf);
    vData.power_consumption2  = Float_reverse_endianess(tmpf);
    client_->DBRead(13,40,4,&tmpf);
    vData.power_duration2 = Float_reverse_endianess(tmpf);

    vData.power_duration = vData.power_duration1 + vData.power_duration2;
    client_->DBRead(13,48,4,&tmpf);
    vData.power_consumption  = Float_reverse_endianess(tmpf);

    client_->DBRead(13,68,4,&tmpf);
    vData.wind_duration1 = Float_reverse_endianess(tmpf);
    client_->DBRead(13,72,4,&tmpf);
    vData.wind_volume1 = Float_reverse_endianess(tmpf);

    client_->DBRead(13,92,4,&tmpf);
    vData.wind_duration2 = Float_reverse_endianess(tmpf);
    client_->DBRead(13,96,4,&tmpf);
    vData.wind_volume2 = Float_reverse_endianess(tmpf);

    vData.wind_duration = vData.wind_duration1 + vData.wind_duration2;
    client_->DBRead(13,100,4,&tmpf);
    vData.wind_volume = Float_reverse_endianess(tmpf);

    client_->DBRead(13,120,4,&tmpf);
    vData.clean_duration = Float_reverse_endianess(tmpf);
    client_->DBRead(13,124,4,&tmpf);
    vData.clean_volume = Float_reverse_endianess(tmpf);

    client_->DBRead(13,144,4,&tmpf);
    vData.flush_duration = Float_reverse_endianess(tmpf);
    client_->DBRead(13,148,4,&tmpf);
    vData.flush_volume = Float_reverse_endianess(tmpf);

    client_->DBRead(13,168,4,&tmpf);
    vData.sewage_duration = Float_reverse_endianess(tmpf);
    client_->DBRead(13,172,4,&tmpf);
    vData.sewage_volume = Float_reverse_endianess(tmpf);


    return vData;
}