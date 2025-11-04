
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
#include <thread>
#include <mutex>
#include "include/profinet.h"
#include <iomanip>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <nlohmann/json.hpp>
#include <cctype>
#include <chrono>
#include <mqttclient.h>
#include <atomic>

using namespace std;

char* host =(char *) "192.168.1.20";
char* port = (char *)"1883";
char* username = (char *)"JavaServer";
char* password = (char *)"123456";
char* clientid = (char *)"mqttclient";
const char * pulishtopic = "flightinfo";
string LocalSeat = "217";

const int	QOS = 0;

mutex mtx_db,mtx_profinet,mtx_client,mtx_mqttdb;
int uprow,upcol,volumerow,volumecol;
flightInfoData flightInfo[80];
volumeInfoData volumeInfo[80],vData;
int insertFlag = 0;
uint8_t volumeKey;
bool DownloadFlag = false;
bool PLCisConnect = false;
std::atomic<bool> vdgsFault(true);
std::atomic<bool> itMoinitor(true);
bool VolumeDownloadFlag = false;
bool powerKey,condKey,cleanKey,flushKey,sewegaKey;
int deleteFlag = 0;
const int BUF_LEN = 1000;

map<string,string> deviceNum;
dataCache datacache;
airWellData aWellL,aWellM,aWellR;
elecWellData eWell,eWellDou;
sewageWellData sWellL,sWellM,sWellR;
powersupplyData psData,psData2;
airconditionData acData,acData2;
cleanwaterStationData cwData;
AirwellFaultData aWellFDataL,aWellFDataM,aWellFDataR;
ElecwellFaultData eWellFData,eWellFDataDou;
SwgwellFaultData sWellFDataL,sWellFDataM,sWellFDataR;
PowerFaultData sPowerFData,sPowerFData2;
AirCondFaultData sCondFData,sCondFData2;
JDLC_BZJD bzjdData;
JDLC_WGZY wgzyData;
BWYD_basic basicData;
BWYD_guidance guidanceData;
BWYD_chock chockData;
BWYD_aircraft aircraftData;

controlMessage controlmess;

typedef struct zhiling
{
    std::string newsID = " ";
    std::string secretKey = " ";
    std::string instruction = " ";
    std::string diviceNumber = " ";
    std::string seatNumber = " ";
} zhiling;
int bitreadset = 0;   // 读移位标识符
int bytereadset = 0;  // 读字节标识符
int bitwriteset = 0;  // 写移位标识符
int bytewriteset = 0; // 写字节标识符
uint16_t snapLittle(uint16_t t)
{
    return (t << 8 | t >> 8);
}
std::string getTimestampString()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

float Float_reverse_endianess(byte value[])
{
    float result;
    *((byte*)&result + 0) = value[3];
    *((byte*)&result + 1) = value[2];
    *((byte*)&result + 2) = value[1];
    *((byte*)&result + 3) = value[0];
    return result;
}

void setBit(uint16_t &value, uint8_t bitPosition)
{
    value |= (1U << bitPosition);
}

void clearBit(uint16_t &value, uint8_t bitPosition)
{
    value &= ~(1U << bitPosition);
}

void setBitTo(uint16_t &value, uint8_t bitPosition, bool bitValue)
{
    if (bitValue)
    {
        setBit(value, bitPosition);
    }else{
        clearBit(value, bitPosition);
    }
}


string splice_insert(flightInfoData inFInfo)
{
    string insertsql = "";
    // (flightNum,aircraftType,startPlace,endPlace,arriveMonth,arriveDay,arriveHour,arriveMinute,leaveMonth,leaveDay,leaveHour,leaveMinute,sendFlag)
    insertsql += "INSERT INTO flightInfo (flightNum, aircraftType, startPlace, endPlace, arriveTime, leaveTime, sendFlag,upFlag) values('";
    insertsql += inFInfo.flightNum;
    insertsql += "','";
    insertsql += inFInfo.aircraftType;
    insertsql += "','";
    insertsql += inFInfo.startPlace;
    insertsql += "','";
    insertsql += inFInfo.endPlace;
    insertsql += "','";
    insertsql += inFInfo.arriveTime;
    insertsql += "','";
    insertsql += inFInfo.leaveTime;
    insertsql += "',";
    insertsql += to_string(1);
    insertsql += ",";
    insertsql += to_string(2);
    insertsql +=  ");";
    return insertsql;
}

string splice_delete(flightInfoData delInfo)
{   
    string delsql = "";
    delsql += "DELETE FROM flightInfo where flightNum = '";
    delsql += delInfo.flightNum;
    delsql += "' and aircraftType = '";
    delsql += delInfo.aircraftType;
    delsql += "' and substr(arriveTime,6) = '";
    stringstream tmp;
    tmp << setw(2) << setfill('0') << delInfo.arriveMonth;
    string stmp = tmp.str();
    delsql += stmp + "-";
    tmp.str("");
    tmp << setw(2) << setfill('0') << delInfo.arriveDay;
    stmp = tmp.str();
    delsql += stmp + " ";
    tmp.str("");
    tmp << setw(2) << setfill('0') << delInfo.arriveHour;
    stmp = tmp.str();
    delsql += stmp + ":";
    tmp.str("");
    tmp << setw(2) << setfill('0') << delInfo.arriveMinute;
    stmp = tmp.str();
    delsql += stmp;
    tmp.str("");
    // delsql += "-";
    // delsql += delInfo.arriveDay;
    // delsql += " ";
    // delsql += delInfo.arriveHour;
    // delsql += ":";
    // delsql += delInfo.arriveMinute;
    delsql += "';";
    return delsql;
}

//单机位所需
string splice_airwell(airWellData aWell)
{
    string device = deviceNum[aWell.deviceType];
    string insertsql = "";
    insertsql += "INSERT INTO business_plc_well_air (seat,device,bizKey,running_state,is_alarm,is_stop,hp_start,psrv_work";
    insertsql += ",mwp_manual,bwp_manual,mwp_function,bwp_function";
    insertsql += ",dd_function,pv1_function,pv2_function,cw1_open,cw1_close";
    insertsql += ",cw2_open,cw2_close,dd2_function,gas_concentration,ps_pressure,cw_amount) VALUES ('";
    insertsql += LocalSeat;
    insertsql += "','";
    insertsql += device;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "',";
    insertsql += to_string(aWell.running_state);
    insertsql += ",";
    insertsql += to_string(aWell.is_alarm);
    insertsql += ",";
    insertsql += to_string(aWell.is_stop);
    insertsql += ",";
    insertsql += to_string(aWell.hp_start);
    insertsql += ",";
    insertsql += to_string(aWell.psrv_work);
    insertsql += ",";
    insertsql += to_string(aWell.mwp_manual);
    insertsql += ",";
    insertsql += to_string(aWell.bwp_manual);
    insertsql += ",";
    insertsql += to_string(aWell.mwp_function);
    insertsql += ",";
    insertsql += to_string(aWell.bwp_function);
    insertsql += ",";
    insertsql += to_string(aWell.dd_function);
    insertsql += ",";
    insertsql += to_string(aWell.pv1_function);
    insertsql += ",";
    insertsql += to_string(aWell.pv2_function);
    insertsql += ",";
    insertsql += to_string(aWell.cw1_open);
    insertsql += ",";
    insertsql += to_string(aWell.cw1_close);
    insertsql += ",";
    insertsql += to_string(aWell.cw2_open);
    insertsql += ",";
    insertsql += to_string(aWell.cw2_close);
    insertsql += ",";
    insertsql += to_string(aWell.dd2_function);
    insertsql += ",";
    insertsql += to_string(aWell.gas_concentration);
    insertsql += ",";
    insertsql += to_string(aWell.ps_pressure);
    insertsql += ",";
    insertsql += to_string(aWell.cw_amount);
    insertsql += ");";
    return insertsql;
}

string splice_elewell(elecWellData eWell)
{
    string device = deviceNum[eWell.deviceType];
    string insertsql = "";
    insertsql += "INSERT INTO business_plc_well_electric (seat,device,bizKey,running_state,is_alarm,is_stop,hp_start";
    insertsql += ",psrv_work,mwp_manual,bwp_manual,mwp_function,bwp_function";
    insertsql += ",dd_function,connection_plug,ms_plug,dd2_function,connection2_plug,ms2_plug";
    insertsql += ",gas_concentration,ps_pressure) VALUES ('";
    insertsql += LocalSeat;
    insertsql += "','";
    insertsql += device;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "',";
    insertsql += to_string(eWell.running_state);
    insertsql += ",";
    insertsql += to_string(eWell.is_alarm);
    insertsql += ",";
    insertsql += to_string(eWell.is_stop);
    insertsql += ",";
    insertsql += to_string(eWell.hp_start);
    insertsql += ",";
    insertsql += to_string(eWell.psrv_work);
    insertsql += ",";
    insertsql += to_string(eWell.mwp_manual);
    insertsql += ",";
    insertsql += to_string(eWell.bwp_manual);
    insertsql += ",";
    insertsql += to_string(eWell.mwp_function);
    insertsql += ",";
    insertsql += to_string(eWell.bwp_function);
    insertsql += ",";
    insertsql += to_string(eWell.dd_function);
    insertsql += ",";
    insertsql += to_string(eWell.connection_plug);
    insertsql += ",";
    insertsql += to_string(eWell.ms_plug);
    insertsql += ",";
    insertsql += to_string(eWell.dd2_function);
    insertsql += ",";
    insertsql += to_string(eWell.connection2_plug);
    insertsql += ",";
    insertsql += to_string(eWell.ms2_plug);
    insertsql += ",";
    insertsql += to_string(eWell.gas_concentration);
    insertsql += ",";
    insertsql += to_string(eWell.ps_pressure);
    insertsql += ");";
    return insertsql;
}

string splice_swgwell(sewageWellData sWell)
{
    string device = deviceNum[sWell.deviceType];
    string insertsql = "";
    insertsql += "INSERT INTO business_plc_well_sewage(seat,device,bizKey,running_state,is_alarm,is_stop,hp_start,psrv_work";
    insertsql += ",mwp_manual,bwp_manual,mwp_function,bwp_function,dd_function";
    insertsql += ",rwv_function,sv_function,rwv_sv_function,rwv_sv_open,rwv_sv_close";
    insertsql += ",sv_open,sv_close,rwv_open,rwv_close";
    insertsql += ",gas_concentration,ps_pressure,rwv_amount) VALUES ('";
    insertsql += LocalSeat;
    insertsql += "','";
    insertsql += device;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "',";
    insertsql += to_string(sWell.running_state);
    insertsql += ",";
    insertsql += to_string(sWell.is_alarm);
    insertsql += ",";
    insertsql += to_string(sWell.is_stop);
    insertsql += ",";
    insertsql += to_string(sWell.hp_start);
    insertsql += ",";
    insertsql += to_string(sWell.psrv_work);
    insertsql += ",";
    insertsql += to_string(sWell.mwp_manual);
    insertsql += ",";
    insertsql += to_string(sWell.bwp_manual);
    insertsql += ",";
    insertsql += to_string(sWell.mwp_function);
    insertsql += ",";
    insertsql += to_string(sWell.bwp_function);
    insertsql += ",";
    insertsql += to_string(sWell.dd_function);
    insertsql += ",";
    insertsql += to_string(sWell.rmv_function);
    insertsql += ",";
    insertsql += to_string(sWell.sv_function);
    insertsql += ",";
    insertsql += to_string(sWell.rwv_sv_function);
    insertsql += ",";
    insertsql += to_string(sWell.rwv_sv_open);
    insertsql += ",";
    insertsql += to_string(sWell.rwv_sv_close);
    insertsql += ",";
    insertsql += to_string(sWell.sv_open);
    insertsql += ",";
    insertsql += to_string(sWell.sv_close);
    insertsql += ",";
    insertsql += to_string(sWell.rwv_open);
    insertsql += ",";
    insertsql += to_string(sWell.rwv_close);
    insertsql += ",";
    insertsql += to_string(sWell.gas_concentration);
    insertsql += ",";
    insertsql += to_string(sWell.ps_pressure);
    insertsql += ",";
    insertsql += to_string(sWell.rwv_amount);
    insertsql += ");";
    return insertsql;
}

string splice_power(powersupplyData pData)
{
    string device = deviceNum[pData.deviceType];
    string insertsql = "";
    insertsql += "INSERT INTO business_plc_device_powersupply(seat,device,bizKey,power_state,alarm_code,input_frequency,input_volt_L1L2";
    insertsql += ",input_volt_L2L3,input_volt_L1L3,A_volt,B_volt,C_volt,frequency,A_current,B_current,C_current";
    insertsql += ",DC_temperature,AC1_temperature,AC2_temperature,AC3_temperature,transf_temp";
    insertsql += ",total_out_time,total_out_power) VALUES ('";
    insertsql += LocalSeat;
    insertsql += "','";
    insertsql += device;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "',";
    insertsql += to_string(pData.power_state);
    insertsql += ",";
    insertsql += to_string(pData.alarm_code);
    insertsql += ",";
    insertsql += to_string(pData.input_freq);
    insertsql += ",";
    insertsql += to_string(pData.input_volt12);
    insertsql += ",";
    insertsql += to_string(pData.input_volt23);
    insertsql += ",";
    insertsql += to_string(pData.input_volt13);
    insertsql += ",";
    insertsql += to_string(pData.A_volt);
    insertsql += ",";
    insertsql += to_string(pData.B_volt);
    insertsql += ",";
    insertsql += to_string(pData.C_volt);
    insertsql += ",";
    insertsql += to_string(pData.frequency);
    insertsql += ",";
    insertsql += to_string(pData.A_curr);
    insertsql += ",";
    insertsql += to_string(pData.B_curr);
    insertsql += ",";
    insertsql += to_string(pData.C_curr);
    insertsql += ",";
    insertsql += to_string(pData.Dc_tmp);
    insertsql += ",";
    insertsql += to_string(pData.Ac1_tmp);
    insertsql += ",";
    insertsql += to_string(pData.Ac2_tmp);
    insertsql += ",";
    insertsql += to_string(pData.Ac3_tmp);
    insertsql += ",";
    insertsql += to_string(pData.trans_tmp);
    insertsql += ",";
    insertsql += to_string(pData.total_out_time);
    insertsql += ",";
    insertsql += to_string(pData.total_out_power);
    insertsql += ");";
    return insertsql;
}

string splice_aircond(airconditionData aData)
{
    string device = deviceNum[aData.deviceType];
    string insertsql = "";
    insertsql += "INSERT INTO business_plc_device_aircondition(seat,device,bizKey,air_state,newair_tmp,splyair_tmp,set_tmp";
    insertsql += ",splyair_pres,splyair_volume,newair_spd,splyair_spd,cmpsor1_high_ps,cmpsor1_low_ps,cmpsor2_high_ps";
    insertsql += ",cmpsor2_low_ps,cmpsor3_high_ps,cmpsor3_low_ps,cmpsor4_high_ps,cmpsor4_low_ps,cmpsor5_high_ps,cmpsor5_low_ps,cmpsor6_high_ps,cmpsor6_low_ps,cond1_total_time,cond2_total_time,cond3_total_time";
    insertsql += ",cmpsor1_total_time,cmpsor2_total_time,cmpsor3_total_time,cmpsor4_total_time,cmpsor_fv_total_time,cmpsor_s_total_time";
    insertsql += ",operation_time_Hour,operation_time_Minute,airsply_total_time,unit_operation_mode) VALUES ('";
    insertsql += LocalSeat;
    insertsql += "','";
    insertsql += device;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "',";
    insertsql += to_string(aData.air_state);
    insertsql += ",";
    insertsql += to_string(aData.newair_tmp);
    insertsql += ",";
    insertsql += to_string(aData.splyair_tmp);
    insertsql += ",";
    insertsql += to_string(aData.set_tmp);
    insertsql += ",";
    insertsql += to_string(aData.splyair_pres);
    insertsql += ",";
    insertsql += to_string(aData.sply_volume);
    insertsql += ",";
    insertsql += to_string(aData.newair_spd);
    insertsql += ",";
    insertsql += to_string(aData.splyair_spd);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor1_high_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor1_low_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor2_high_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor2_low_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor3_high_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor3_low_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor4_high_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor4_low_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor5_high_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor5_low_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor6_high_ps);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor6_low_ps);
    insertsql += ",";
    insertsql += to_string(aData.cond1_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cond2_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cond3_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor1_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor2_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor3_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor4_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor5_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.cmpsor6_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.oprtion_time_H);
    insertsql += ",";
    insertsql += to_string(aData.oprtion_time_M);
    insertsql += ",";
    insertsql += to_string(aData.airsply_tt_time);
    insertsql += ",";
    insertsql += to_string(aData.unit_oprtion_mode);
    insertsql += ");";
    return insertsql;
}

string splice_cleanstation(cleanwaterStationData cwData)
{
    //string device = deviceNum[aData.deviceType];
    string insertsql = "";
    insertsql += "INSERT INTO cleanwater_station (running_signal_1,frequency_1,totaltime_h_1,totaltime_m_1,totaltime_s_1,running_signal_2,frequency_2,totaltime_h_2";
    insertsql += ",totaltime_m_2,totaltime_s_2,watertank1_lvl,watertank2_lvl,outlet1_prs,outlet2_prs) VALUES (";
    insertsql += to_string(cwData.running_signal_1);
    insertsql += ",";
    insertsql += to_string(cwData.frequency_1);
    insertsql += ",";
    insertsql += to_string(cwData.totaltime_h_1);
    insertsql += ",";
    insertsql += to_string(cwData.totaltime_m_1);
    insertsql += ",";
    insertsql += to_string(cwData.totaltime_s_1);
    insertsql += ",";
    insertsql += to_string(cwData.running_signal_2);
    insertsql += ",";
    insertsql += to_string(cwData.frequency_2);
    insertsql += ",";
    insertsql += to_string(cwData.totaltime_h_2);
    insertsql += ",";
    insertsql += to_string(cwData.totaltime_m_2);
    insertsql += ",";
    insertsql += to_string(cwData.totaltime_s_2);
    insertsql += ",";
    insertsql += to_string(cwData.watertank1_lvl);
    insertsql += ",";
    insertsql += to_string(cwData.watertank2_lvl);
    insertsql += ",";
    insertsql += to_string(cwData.outlet1_prs);
    insertsql += ",";
    insertsql += to_string(cwData.outlet2_prs);
    insertsql += ");";
    return insertsql;
}

string splice_flushstation(flushwaterStationData fwData)
{
    //string device = deviceNum[aData.deviceType];
    string insertsql = "";
    insertsql += "INSERTINTO flushwater_station(id,running_signal_1,frequency_1,totaltime_h_1,totaltime_m_1,totaltime_s_1,running_signal_2,frequency_2,totaltime_h_2,totaltime_m_2";
    insertsql += ",totaltime_s_2,medrunning_signal_1,medfrequency_1,medtotaltime_h_1,medtotaltime_m_1,medtotaltime_s_1,medrunning_signal_2,medfrequency_2,medtotaltime_h_2";
    insertsql += ",medtotaltime_m_2,medtotaltime_s_2,medtank_lvl,watertank1_lvl,watertank2_lvl,outlet1_prs,outlet2_prs,create_time)VALUES(";
    
    insertsql += to_string(fwData.running_signal_1);
    insertsql += ",";
    insertsql += to_string(fwData.frequency_1);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_h_1);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_m_1);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_s_1);
    insertsql += ",";
    insertsql += to_string(fwData.running_signal_2);
    insertsql += ",";
    insertsql += to_string(fwData.frequency_2);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_h_2);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_m_2);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_s_2);
    insertsql += ",";
    insertsql += to_string(fwData.medrunning_signal_1);
    insertsql += ",";
    insertsql += to_string(fwData.frequency_1);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_h_1);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_m_1);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_s_1);
    insertsql += ",";
    insertsql += to_string(fwData.running_signal_2);
    insertsql += ",";
    insertsql += to_string(fwData.frequency_2);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_h_2);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_m_2);
    insertsql += ",";
    insertsql += to_string(fwData.totaltime_s_2);
    insertsql += ",";
    insertsql += to_string(fwData.watertank1_lvl);
    insertsql += ",";
    insertsql += to_string(fwData.watertank2_lvl);
    insertsql += ",";
    insertsql += to_string(fwData.outlet1_prs);
    insertsql += ",";
    insertsql += to_string(fwData.outlet2_prs);
    insertsql += ");";
    return insertsql;
}
string splice_dosage(volumeInfoData vData)
{
    string insertsql = "";
    insertsql += "INSERT INTO business_device_usage(flightNum,bizKey,guaranteeStart,guaranteeEnd,guaranDuration,elecDuration,elecQuantity";
    insertsql += ",airDuration,airQuantity,clearwaterDuration,clearwaterQuantity,flushwaterDuration";
    insertsql += ",flushwaterQuantity,sewagewaterDuration,sewagewaterQuantity) VALUES ('";
    insertsql += datacache.airlinecode;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "','";
    insertsql += vData.coverage_start;
    insertsql += "','";
    insertsql += vData.coverage_end;
    insertsql += "',";
    insertsql += to_string(vData.coverage_duration);
    insertsql += ",";
    insertsql += to_string(vData.power_duration);
    insertsql += ",";
    insertsql += to_string(vData.power_consumption);
    insertsql += ",";
    insertsql += to_string(vData.wind_duration);
    insertsql += ",";
    insertsql += to_string(vData.wind_volume);
    insertsql += ",";
    insertsql += to_string(vData.clean_duration);
    insertsql += ",";
    insertsql += to_string(vData.clean_volume);
    insertsql += ",";
    insertsql += to_string(vData.flush_duration);
    insertsql += ",";
    insertsql += to_string(vData.flush_volume);
    insertsql += ",";
    insertsql += to_string(vData.clean_duration);
    insertsql += ",";
    insertsql += to_string(vData.clean_volume);
    insertsql += ");";
    return insertsql;
}



string splice_faultInfo(string device, string alarmtype)
{
    
    string insertsql = "";
    insertsql += "INSERT INTO business_device_alarm(seat,device,bizKey,alarmType,alarmStatus)VALUES('";
    insertsql += LocalSeat;
    insertsql += "','";
    insertsql += device;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "','";
    insertsql += alarmtype;
    insertsql += "','";
    insertsql += "故障";
    insertsql += "');";
    return insertsql;
}

// string splice_groundNode()
// {

// }

string splice_flightNode(JDLC_BZJD bzjdData)
{
    string insertsql = "";
    insertsql += "INSERT INTO flightNode(cameraId,standNo,bizKey,event,createTime,type)VALUES('";
    insertsql += bzjdData.cameraId;
    insertsql += "','";
    insertsql += bzjdData.standNo;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "','";
    insertsql += bzjdData.event;
    insertsql += "','";
    insertsql += bzjdData.createTime;
    insertsql += "','";
    insertsql += bzjdData.type;
    insertsql += "');";
    return insertsql;
}

string splice_flightAlarm(JDLC_WGZY wgzyData)
{
    string insertsql = "";
    insertsql += "INSERT INTO flightAlarm(cameraId,standNo,bizKey,code,createTime,type)VALUES('";
    insertsql += wgzyData.cameraId;
    insertsql += "','";
    insertsql += wgzyData.standNo;
    insertsql += "','";
    insertsql += datacache.bizKey;
    insertsql += "','";
    insertsql += wgzyData.code;
    insertsql += "','";
    insertsql += wgzyData.createTime;
    insertsql += "','";
    insertsql += wgzyData.type;
    insertsql += "');";
    return insertsql;
}

string splice_basic(BWYD_basic basicData)
{
    string insertsql = "";
    insertsql += "INSERT INTO berth_basic(dgsid,stand,lockstand,workmode,state,syserror,adjlock)VALUES('";
    insertsql += basicData.dgsid;
    insertsql += "','";
    insertsql += basicData.stand;
    insertsql += "','";
    insertsql += basicData.lockstand;
    insertsql += "',";
    insertsql += to_string(basicData.workmode);
    insertsql += ",";
    insertsql += to_string(basicData.state);
    insertsql += ",";
    insertsql += to_string(basicData.systemerror);
    insertsql += ",";
    insertsql += to_string(basicData.adjlock);
    insertsql += ");";
    return insertsql;
}

string splice_guidance(BWYD_guidance guidanceData)
{
    string insertsql = "";
    insertsql += "INSERT INTO berth_guidance(dgsid,guidid,stand,centerline,flightno,actype,stopline,step,idstate,azstate,overspeed,effect,alert)VALUES('";
    insertsql += guidanceData.dgsid;
    insertsql += "','";
    insertsql += guidanceData.guidid;
    insertsql += "','";
    insertsql += guidanceData.stand;
    insertsql += "','";
    insertsql += guidanceData.centerline;
    insertsql += "','";
    insertsql += guidanceData.flightno;
    insertsql += "','";
    insertsql += guidanceData.actype;
    insertsql += "',";
    insertsql += to_string(guidanceData.stopline);
    insertsql += ",";
    insertsql += to_string(guidanceData.step);
    insertsql += ",";
    insertsql += to_string(guidanceData.idstate);
    insertsql += ",";
    insertsql += to_string(guidanceData.azstate);
    insertsql += ",";
    insertsql += to_string(guidanceData.overspeed);
    insertsql += ",";
    insertsql += to_string(guidanceData.effect);
    insertsql += ",";
    insertsql += to_string(guidanceData.alert);
    insertsql += ");";
    return insertsql;
}

string splice_aircraft(BWYD_aircraft aircraftData)
{
    string insertsql = "";
    insertsql += "INSERT INTO berth_aircraft(guidid,distance,azimuth,speed)VALUES('";
    insertsql += aircraftData.guidid;
    insertsql += "',";
    insertsql += to_string(aircraftData.distance);
    insertsql += ",";
    insertsql += to_string(aircraftData.azimuth);
    insertsql += ",";
    insertsql += to_string(aircraftData.speed);
    insertsql += ");";
    return insertsql;
}

string splice_chock(BWYD_chock chockData)
{
    string insertsql = "";
    insertsql += "Insert Or Replace INTO berth_chock(chocktime,flightno,chock)VALUES('";
    insertsql += chockData.chocktime;
    insertsql += "','";
    insertsql += chockData.flightno;
    insertsql += "',";
    insertsql += to_string(chockData.chock);
    insertsql += ");";
    return insertsql;
}

bool isWithin30Minutes(const string timeNow,const string timeEst)
{
    tm tm1 = {};
    tm tm2 = {};
    istringstream ss1(timeNow),ss2(timeEst);

    ss1 >> get_time(&tm1,"%Y-%m-%d %H:%M");
    ss2 >> get_time(&tm2,"%Y-%m-%d %H:%M");

    if(ss1.fail() || ss2.fail())
    {
        return false;
    }

    tm1.tm_isdst = -1;
    tm2.tm_isdst = -1;

    //转换为time_t（秒数）
    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    const double diff = difftime(t2,t1);
    if (diff < 1800.0)
    {
        return true;
    }else
    {
        return false;
    }
}

string formatPlantime(const string& input)
{
    string processed = input;
    if (processed.length() < 12)
    {
        processed.append(12-processed.length(),'0');
    }else if (processed.length() > 12)
    {
        processed = processed.substr(0,12);
    }

    string year = processed.substr(0,4);
    string month = processed.substr(4,2);
    string day = processed.substr(6,2);
    string hour = processed.substr(8,2);
    string minute = processed.substr(10,2);

    return year + "-" + month + "-" + day + " " + hour + ":" + minute;
}

void Task_Fault()
{
    sqlite3 *db;
    char *err_msg = 0;
    string dev,atype;
    
    

    while(1)
    {
        int rc = sqlite3_open("singleDB.db", &db);
        //cout<< rc <<endl;
        if (rc)
        {
            fprintf(stderr, "task_fault 无法打开数据库: %s\n", sqlite3_errmsg(db));
            //return 1;
            continue;
        }
        else 
        {
            //fprintf(stderr, "成功打开数据库\n");
        }

        string insertsql="";
        for(map<string,uint16_t>::iterator it=aWellFDataL.datamap.begin();it!=aWellFDataL.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[aWellFDataL.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=aWellFDataM.datamap.begin();it!=aWellFDataM.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[aWellFDataM.deviceType];
                atype = it->first;
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=aWellFDataR.datamap.begin();it!=aWellFDataR.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[aWellFDataR.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=eWellFData.datamap.begin();it!=eWellFData.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[eWellFData.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=eWellFDataDou.datamap.begin();it!=eWellFDataDou.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[eWellFDataDou.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sWellFDataL.datamap.begin();it!=sWellFDataL.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sWellFDataL.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sWellFDataM.datamap.begin();it!=sWellFDataM.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sWellFDataM.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sWellFDataR.datamap.begin();it!=sWellFDataR.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sWellFDataR.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sPowerFData.datamap.begin();it!=sPowerFData.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sPowerFData.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sPowerFData2.datamap.begin();it!=sPowerFData2.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sPowerFData2.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sCondFData.datamap.begin();it!=sCondFData.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sCondFData.deviceType];
                atype = it->first;
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }

        for(map<string,uint16_t>::iterator it=sCondFData2.datamap.begin();it!=sCondFData2.datamap.end();it++)
        {
            if(it->second != 0)
            {
                
                dev = deviceNum[sCondFData2.deviceType];
                atype = it->first;
                
                
                insertsql += splice_faultInfo(dev,atype);

                // mtx_db.lock();
                // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
                // mtx_db.unlock();
                // if (inrc == SQLITE_OK)
                // {
                    
                // }
                // else
                // {
                //     cout<<"Faultinfo:"<< err_msg<<endl;
                //     sqlite3_free(err_msg);
                // }
            }
            
        }
        
        mtx_db.lock();
        int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        mtx_db.unlock();
        if (inrc == SQLITE_OK)
        {
            
        }
        else
        {
            cout<<"Faultinfo:"<< err_msg<<endl;
            sqlite3_free(err_msg);
        }
        sqlite3_close(db);
        chrono::milliseconds sleep_dur(2000);
        this_thread::sleep_for(sleep_dur);
    }
}

void on_Messagearrived(void* client, message_data_t* msg) 
{
    
    MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d, \nmessage:%s", __FILE__, __LINE__, __FUNCTION__, 
            msg->topic_name, msg->message->qos, (char*)msg->message->payload);

    char *temp =(char *) msg->message->payload;
    string JDLCString(temp);

    nlohmann::json recvjson;
    string insertsql = "";
    recvjson = nlohmann::json::parse(JDLCString);
    if(recvjson["data"].contains("event"))
    {
        if (!recvjson["data"]["createTime"].is_null())
        {
            bzjdData.createTime = recvjson["data"]["createTime"];
        }
        else
        {
            bzjdData.createTime = "";
        }
        
        if (!recvjson["data"]["cameraId"].is_null())
        {
            bzjdData.cameraId = recvjson["data"]["cameraId"];
        }
        else
        {
            bzjdData.cameraId = "";
        }

        if (!recvjson["data"]["standNo"].is_null())
        {
            bzjdData.standNo = recvjson["data"]["standNo"];
        }
        else
        {
            bzjdData.standNo = "";
        }

        if (!recvjson["data"]["event"].is_null())
        {
            bzjdData.event = recvjson["data"]["event"];
        }
        else
        {
            bzjdData.event = ""; 
        }
        
        if (!recvjson["data"]["type"].is_null())
        {
            bzjdData.type = recvjson["data"]["type"];
        }
        else
        {
            bzjdData.type = "";
        }
        insertsql = splice_flightNode(bzjdData);
    }
    else
    {
        if (!recvjson["data"]["createTime"].is_null())
        {
            wgzyData.createTime = recvjson["data"]["createTime"];
        }
        else
        {
            wgzyData.createTime = "";
        }

        if (!recvjson["data"]["cameraId"].is_null())
        {
            wgzyData.cameraId = recvjson["data"]["cameraId"];
        }
        else
        {
            wgzyData.cameraId = "";
        }

        if (!recvjson["data"]["code"].is_null())
        {
            wgzyData.code = recvjson["data"]["code"];
        }
        else
        {
            wgzyData.code = "";
        }

        if (!recvjson["data"]["standNo"].is_null())
        {
            wgzyData.standNo = recvjson["data"]["standNo"];
        }
        else
        {
            wgzyData.standNo = "";
        }
        
        if (!recvjson["data"]["type"].is_null())
        {
            wgzyData.type = recvjson["data"]["type"];
        }
        else
        {
            wgzyData.type = "";
        }
        insertsql = splice_flightAlarm(wgzyData);
        itMoinitor = false;
    }

    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("singleDB.db", &db);
    //cout<< rc <<endl;
    if (rc)
    {
        fprintf(stderr, "on mess 无法打开数据库: %s\n", sqlite3_errmsg(db));
        //return 1;
        //continue;
    }
    else 
    {
        //fprintf(stderr, "成功打开数据库\n");
    }

    mtx_db.lock();
    int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
    mtx_db.unlock();
    if (inrc == SQLITE_OK)
    {
        
    }
    else
    {
        cout<<"JDLC"<< err_msg<<endl;
        sqlite3_free(err_msg);
    }
    sqlite3_close(db);

}



void control_Message(void* client_, message_data_t* msg)
{
    MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d, \nmessage:%s", __FILE__, __LINE__, __FUNCTION__, 
            msg->topic_name, msg->message->qos, (char*)msg->message->payload);

    zhiling tmp;
    char* backclientid = (char *)"mqttbackclient";
    mqtt_client_t *myClient = NULL;
    myClient = mqtt_lease();
    mqtt_set_host(myClient,host);
    mqtt_set_port(myClient,port);
    mqtt_set_user_name(myClient,username);
    mqtt_set_password(myClient,password);
    mqtt_set_client_id(myClient,backclientid);
    mqtt_set_clean_session(myClient,1);
    mqtt_set_keep_alive_interval(myClient,50);
    mqtt_set_cmd_timeout(myClient,5000);
    mqtt_set_read_buf_size(myClient,4000);
    mqtt_set_write_buf_size(myClient,4000);
    mqtt_connect(myClient);
    uint16_t a = 0;
    uint16_t b = 0;
    uint8_t c = 0;
    int rc = 1;
    message_data_t *msg_zhiling;
    mqtt_message_t msg_reply;
    nlohmann::json json_reply;
    std::string newsType;
    std::string operationalState;
    std::string str1 = "T2-" + LocalSeat + "-PCA-001";
    std::string str2 = "T2-" + LocalSeat + "-GPU-001";
    std::string str3 = "T2-" + LocalSeat + "-PIT-003";
    std::string str4 = "T2-" + LocalSeat + "-PIT-001";
    std::string str5 = "T2-" + LocalSeat + "-PIT-006";
    std::string str6 = "kai";
    std::string str7 = "guan";
    byte bt[4];
    memset(&msg_reply, 0, sizeof(msg_reply));
    try {
        if (msg == nullptr || msg->message == nullptr || msg->message->payload == nullptr) {
            throw std::runtime_error("Invalid MQTT message or payload");
        }

        char* jsonstring = (char *)(msg->message->payload);
        if (jsonstring[0] == '\0') {
            throw std::runtime_error("Empty payload");
        }

        std::string Jsonstring(jsonstring);
        nlohmann::json json_zhiling = nlohmann::json::parse(Jsonstring);

        if (json_zhiling.contains("newsId") && json_zhiling["newsId"].is_string()) {
            tmp.newsID = json_zhiling["newsId"];
            json_reply["newsId"]=json_zhiling["newsId"];
        } else {
            throw std::runtime_error("Invalid or missing 'newsID' field");
        }

        if (json_zhiling.contains("secretKey") && json_zhiling["secretKey"].is_string()) {
            tmp.secretKey = json_zhiling["secretKey"];
            json_reply["secretKey"]=json_zhiling["secretKey"];
        }

        if (json_zhiling.contains("seatNumber") && json_zhiling["seatNumber"].is_string()) {
            tmp.seatNumber = json_zhiling["seatNumber"];
        }

        if (json_zhiling.contains("deviceNumber") && json_zhiling["deviceNumber"].is_string()) {
            tmp.diviceNumber = json_zhiling["deviceNumber"];
        }

        if (json_zhiling.contains("instruction") && json_zhiling["instruction"].is_string()) {
            tmp.instruction = json_zhiling["instruction"];
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in message_handle_zhiling: " << e.what() << std::endl;
    }

    if (!str1.compare(tmp.diviceNumber) && !str6.compare(tmp.instruction))
    {
        bitwriteset = 4;
        bytewriteset = 0;
        bytereadset = 13;
    }
    if (!str1.compare(tmp.diviceNumber) && !str7.compare(tmp.instruction))
    {
        bitwriteset = 5;
        bytewriteset = 0;
        bytereadset = 15;
    }
    if (!str2.compare(tmp.diviceNumber) && !str6.compare(tmp.instruction))
    {
        bitwriteset = 0;
        bytewriteset = 0;
        bytereadset = 5;
    }
    if (!str2.compare(tmp.diviceNumber) && !str7.compare(tmp.instruction))
    {
        bitwriteset = 1;
        bytewriteset = 0;
        bytereadset = 7;
    }
    if (!str3.compare(tmp.diviceNumber) && !str6.compare(tmp.instruction))
    {
        bitwriteset = 2;
        bytewriteset = 1;
        bytereadset = 25;
    }
    if (!str3.compare(tmp.diviceNumber) && !str7.compare(tmp.instruction))
    {
        bitwriteset = 2;
        bytewriteset = 1;
        bytereadset = 15;
    }
    if (!str4.compare(tmp.diviceNumber) && !str6.compare(tmp.instruction))
    {
        bitwriteset = 0;
        bytewriteset = 1;
        bytereadset = 21;
    }
    if (!str4.compare(tmp.diviceNumber) && !str7.compare(tmp.instruction))
    {
        bitwriteset = 0;
        bytewriteset = 1;
        bytereadset = 21;
    }
    if (!str5.compare(tmp.diviceNumber) && !str6.compare(tmp.instruction))
    {
        bitwriteset = 4;
        bytewriteset = 1;
        bytereadset = 29;
    }
    if (!str5.compare(tmp.diviceNumber) && !str7.compare(tmp.instruction))
    {
        bitwriteset = 4;
        bytewriteset = 1;
        bytereadset = 29;
    }
    if(!str6.compare(tmp.instruction))
    {
        c |= (1 << bitwriteset);
        cout<<"kai"<<endl;
    }else
    {
        c |= (0 << bitwriteset);
        cout<<"guan"<<endl;
    }
    
    TS7Client *snap = new TS7Client();
    rc = snap->ConnectTo("192.168.0.113", 0, 1);
    std::cout<< "rc1: " << rc<< std::endl;
    if (rc != 0)
    {
        json_reply["newsType"] = "1";
        json_reply["operationalState"] = "2";
        json_reply["operationalTime"] = getTimestampString();
        std::string str = json_reply.dump();
        if (str.empty())
        {
            throw std::runtime_error("Empty payload for MQTT publish");
        }
        //cout<<str<<endl;
        //str=R"({"newsId":"78","secretKey":"02f44751b4784f919322d480d0bf1215","newsType":"1","operationalState":"1","operationalTime":"2024-12-25 15:32:40"})";
        //cout<<str<<endl;
        msg_reply.payload = (char *)str.c_str();
        mqtt_publish(myClient,"CommandBack", &msg_reply);
        
    }
    else
    {
        json_reply["newsType"] = "1";
        json_reply["operationalState"] = "1";
        json_reply["operationalTime"] = getTimestampString();
        std::string str = json_reply.dump();
        if (str.empty())
        {
            throw std::runtime_error("Empty payload for MQTT publish");
        }
        // std::string str=R"({"newsId":"78","secretKey":"02f44751b4784f919322d480d0bf1215","newsType":"1","operationalState":"1","operationalTime":"2024-12-25 15:32:40"})";
        msg_reply.payload = (char *)str.c_str();
        mqtt_publish(myClient, "CommandBack", &msg_reply);

        rc = 1;
        std::cout<<"bitwrite: "<< bitwriteset << std::endl;
        std::cout<<"bytewrite: "<< bytewriteset << std::endl;
        std::cout<<"byteread: "<< bytereadset << std::endl;
        rc = snap->DBWrite(18, bytewriteset, 1, &c);
        
        std::cout<<"rc: "<< rc << std::endl;
        

        if (rc != 0)
        {
            throw std::runtime_error("Failed to read from DB");
        }
        else
        {
            rc = snap->DBRead(18, bytereadset, 4, &bt);
            float f  = Float_reverse_endianess(bt);
            a=(uint16_t)f;
            std::cout<<"a: "<< a << std::endl;
            if (a == 0x02)
            {
                json_reply["newsType"] = "2";
                json_reply["operationalState"] = "2";
                json_reply["operationalTime"] = getTimestampString();
                std::string str = json_reply.dump();
                if (str.empty())
                {
                    throw std::runtime_error("Empty payload for MQTT publish");
                }
                msg_reply.payload = (char *)str.c_str();
                mqtt_publish(myClient, "CommandBack", &msg_reply);
                std::cout<<"a=0x02"<< std::endl;
            }
            if (a == 0x04 || a == 0x00)
            {
                json_reply["newsType"] = "2";
                json_reply["operationalState"] = "4";
                json_reply["operationalTime"] = getTimestampString();
                std::string str = json_reply.dump();
                if (str.empty())
                {
                    throw std::runtime_error("Empty payload for MQTT publish");
                }
                msg_reply.payload = (char *)str.c_str();
                mqtt_publish(myClient, "CommandBack", &msg_reply);
                std::cout<<"a=0x04"<< std::endl;
                rc = 0;
            }
            if (a == 0x03)
            {
                json_reply["newsType"] = "2";
                json_reply["operationalState"] = "3";
                json_reply["operationalTime"] = getTimestampString();
                std::string str = json_reply.dump();
                if (str.empty())
                {
                    throw std::runtime_error("Empty payload for MQTT publish");
                }
                msg_reply.payload = (char *)str.c_str();
                mqtt_publish(myClient, "CommandBack", &msg_reply);
                std::cout<<"a=0x03"<< std::endl;
            }
            if (a == 0x01)
            {
                json_reply["newsType"] = "2";
                json_reply["operationalState"] = "1";
                json_reply["operationalTime"] = getTimestampString();
                std::string str = json_reply.dump();
                if (str.empty())
                {
                    throw std::runtime_error("Empty payload for MQTT publish");
                }
                msg_reply.payload = (char *)str.c_str();
                mqtt_publish(myClient, "CommandBack", &msg_reply);
                std::cout<<"a=0x01"<< std::endl;
                c = 0;
            }
        }
    }
    
    snap->Disconnect();
    delete snap;
    snap=NULL;
    mqtt_disconnect(myClient);

}



void zgsd_basic(void* client_, message_data_t* msg)
{
    // MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d, \nmessage:%s", __FILE__, __LINE__, __FUNCTION__, 
    //         msg->topic_name, msg->message->qos, (char*)msg->message->payload);

    char *temp =(char *) msg->message->payload;
    string JDLCString(temp);

    nlohmann::json recvjson;
    string insertsql = "";
    recvjson = nlohmann::json::parse(JDLCString);
    
    if (!recvjson["dgsid"].is_null())
    {
        basicData.dgsid = recvjson["dgsid"];
    }
    else
    {
        basicData.dgsid = "";
    }

    if (!recvjson["stand"].is_null())
    {
        basicData.stand = recvjson["stand"];
    }
    else
    {
        basicData.stand = "";
    }

    if (!recvjson["workmode"].is_null())
    {
        basicData.workmode = recvjson["workmode"];
    }
    else
    {
        basicData.workmode = 0;
    }

    if (!recvjson["state"].is_null())
    {
        basicData.state = recvjson["state"];
    }
    else
    {
        basicData.state = 0;
    }

    if (!recvjson["systemerror"].is_null())
    {
        basicData.systemerror = recvjson["systemerror"];
    }
    else
    {
        basicData.systemerror = 0;
    }

    if (!recvjson["adjlock"].is_null())
    {
        basicData.adjlock = recvjson["adjlock"];
    }
    else
    {
        basicData.adjlock = 0;
    }

    if (!recvjson["lockstand"].is_null())
    {
        basicData.lockstand = recvjson["lockstand"];
    }
    else
    {
        basicData.lockstand = "";
    }

    insertsql = splice_basic(basicData);

    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("mqttDB.db", &db);
    //cout<< rc <<endl;
    if (rc)
    {
        fprintf(stderr, "zgsd_basic 无法打开数据库: %s\n", sqlite3_errmsg(db));
        //return 1;
        //continue;
    }
    else 
    {
        //fprintf(stderr, "成功打开数据库\n");
        mtx_mqttdb.lock();
        int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        mtx_mqttdb.unlock();
        if (inrc == SQLITE_OK)
        {
            
        }
        else
        {
            cout<<"BWYD_basic"<< err_msg<<endl;
            sqlite3_free(err_msg);
        }
        
    }
    sqlite3_close(db);
    // chrono::milliseconds sleep_dur(100);
    //     this_thread::sleep_for(sleep_dur);
    //mqtt_sleep_ms(1000);
}



void zgsd_guidance(void* client_, message_data_t* msg)
{
    // MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d, \nmessage:%s", __FILE__, __LINE__, __FUNCTION__, 
    //         msg->topic_name, msg->message->qos, (char*)msg->message->payload);

    char *temp =(char *) msg->message->payload;
    string JDLCString(temp);

    nlohmann::json recvjson;
    string insertsql = "";
    recvjson = nlohmann::json::parse(JDLCString);
    

    if (!recvjson["guideid"].is_null())
    {
        guidanceData.guidid = recvjson["guideid"];
    }
    else
    {
        guidanceData.guidid = "";
    }

    if (!recvjson["dgsid"].is_null())
    {
        guidanceData.dgsid = recvjson["dgsid"];
    }
    else
    {
        guidanceData.dgsid = "";
    }

    if (!recvjson["stand"].is_null())
    {
        guidanceData.stand = recvjson["stand"];
    }
    else
    {
        guidanceData.stand = "";
    }

    if (!recvjson["centerline"].is_null())
    {
        guidanceData.centerline = recvjson["centerline"];
    }
    else
    {
        guidanceData.centerline = "";
    }

    if (!recvjson["flightno"].is_null())
    {
        guidanceData.flightno = recvjson["flightno"];
    }
    else
    {
        guidanceData.flightno = "";
    }

    if (!recvjson["actype"].is_null())
    {
        guidanceData.actype = recvjson["actype"];
    }
    else
    {
        guidanceData.actype = "";
    }

    if (!recvjson["stopline"].is_null())
    {
        guidanceData.stopline = recvjson["stopline"];
    }
    else
    {
        guidanceData.stopline = 0;
    }
    if (!recvjson["step"].is_null())
    {
        guidanceData.step = recvjson["step"];
    }
    else
    {
        guidanceData.step = 0;
    }
    if (!recvjson["idstate"].is_null())
    {
        guidanceData.idstate = recvjson["idstate"];
    }
    else
    {
        guidanceData.idstate = 0;
    }
    if (!recvjson["azstate"].is_null())
    {
        guidanceData.azstate = recvjson["azstate"];
    }
    else
    {
        guidanceData.azstate = 0;
    }
    if (!recvjson["effect"].is_null())
    {
        guidanceData.effect = recvjson["effect"];
    }
    else
    {
        guidanceData.effect = 0;
    }
    if (!recvjson["alert"].is_null())
    {
        guidanceData.alert = recvjson["alert"];
    }
    else
    {
        guidanceData.alert = 0;
    }
    if(guidanceData.alert!=0)
    {
        vdgsFault = false;
    }
    if (!recvjson["overspeed"].is_null())
    {
        guidanceData.overspeed = recvjson["overspeed"];
    }
    else
    {
        guidanceData.overspeed = false;
    }

    
    insertsql = splice_guidance(guidanceData);

    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("mqttDB.db", &db);
    //cout<< rc <<endl;
    if (rc)
    {
        fprintf(stderr, "zgsd_guidance 无法打开数据库: %s\n", sqlite3_errmsg(db));
        //return 1;
        //continue;
    }
    else 
    {
        //fprintf(stderr, "成功打开数据库\n");
        mtx_mqttdb.lock();
        int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        mtx_mqttdb.unlock();
        if (inrc == SQLITE_OK)
        {
            
        }
        else
        {
            cout<<"BWYD_guidance"<< err_msg<<endl;
            sqlite3_free(err_msg);
        }
    }
    sqlite3_close(db);
    chrono::milliseconds sleep_dur(50);
        this_thread::sleep_for(sleep_dur);
    
    
}

void zgsd_aircraft(void* client_, message_data_t* msg)
{
    // MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d, \nmessage:%s", __FILE__, __LINE__, __FUNCTION__, 
    //         msg->topic_name, msg->message->qos, (char*)msg->message->payload);
    // cout<<endl;
    char *temp =(char *) msg->message->payload;
    string JDLCString(temp);

    nlohmann::json recvjson;
    string insertsql = "";
    recvjson = nlohmann::json::parse(JDLCString);

    if (!recvjson["guideid"].is_null())
    {
        aircraftData.guidid = recvjson["guideid"];
    }
    else
    {
        aircraftData.guidid = "";
    }

    if (!recvjson["distance"].is_null())
    {
        aircraftData.distance = recvjson["distance"];
    }
    else
    {
        aircraftData.distance = 0;
    }

    if (!recvjson["azimuth"].is_null())
    {
        aircraftData.azimuth = recvjson["azimuth"];
    }
    else
    {
        aircraftData.azimuth = 0;
    }

    if (!recvjson["speed"].is_null())
    {
        aircraftData.speed = recvjson["speed"];
    }
    else
    {
        aircraftData.speed = 0;
    }

    insertsql = splice_aircraft(aircraftData);

    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("mqttDB.db", &db);
    
    if (rc)
    {
        fprintf(stderr, "zgsd_aircraft 无法打开数据库: %s\n", sqlite3_errmsg(db));
        //return 1;
        //continue;
    }
    else 
    {
        //fprintf(stderr, "成功打开数据库\n");
        mtx_mqttdb.lock();
        int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        mtx_mqttdb.unlock();
        if (inrc == SQLITE_OK)
        {
            
        }
        else
        {
            cout<<"BWYD_aircraft"<< err_msg<<endl;
            sqlite3_free(err_msg);
        }
    }
    sqlite3_close(db);
    chrono::milliseconds sleep_dur(150);
    this_thread::sleep_for(sleep_dur);
   
    
}

void zgsd_chock(void* client_, message_data_t* msg)
{
    // MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d, \nmessage:%s", __FILE__, __LINE__, __FUNCTION__, 
    //         msg->topic_name, msg->message->qos, (char*)msg->message->payload);

    char *temp =(char *) msg->message->payload;
    string JDLCString(temp);

    nlohmann::json recvjson;
    string insertsql = "";
    recvjson = nlohmann::json::parse(JDLCString);

    if (!recvjson["chock"].is_null())
    {
        chockData.chock = recvjson["chock"];
    }
    else
    {
        chockData.chock = false;
    }

    if (!recvjson["time"].is_null())
    {
        chockData.chocktime = recvjson["time"];
    }
    else
    {
        chockData.chocktime = "";
    }

    if (!recvjson["flightno"].is_null())
    {
        chockData.flightno = recvjson["flightno"];
    }
    else
    {
        chockData.flightno = "";
    }

    insertsql = splice_chock(chockData);

    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("mqttDB.db", &db);
    //cout<< rc <<endl;
    if (rc)
    {
        fprintf(stderr, "zgsd_chock 无法打开数据库: %s\n", sqlite3_errmsg(db));
        //return 1;
        //continue;
    }
    else 
    {
        //fprintf(stderr, "成功打开数据库\n");
        mtx_mqttdb.lock();
        int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        mtx_mqttdb.unlock();
        if (inrc == SQLITE_OK)
        {
            
        }
        else
        {
            cout<<"BWYD_chock"<< err_msg<<endl;
            sqlite3_free(err_msg);
        }
    }
    sqlite3_close(db);
    // chrono::milliseconds sleep_dur(100);
    //     this_thread::sleep_for(sleep_dur);
    //mqtt_sleep_ms(1000);
}



void Task_JDLC()
{
    sqlite3 *db;
    char *err_msg = 0;
    mqtt_client_t *client = NULL;
    mqtt_log_init();
    client = mqtt_lease();
    mqtt_set_host(client,host);
    mqtt_set_port(client,port);
    mqtt_set_user_name(client,username);
    mqtt_set_password(client,password);
    mqtt_set_client_id(client,clientid);
    mqtt_set_clean_session(client,1);
    mqtt_set_keep_alive_interval(client,50);
    mqtt_set_cmd_timeout(client,5000);
    mqtt_set_read_buf_size(client,4000);
    mqtt_set_write_buf_size(client,4000);

    mqtt_message_t msg;
    jsonS jsoninfo;

    memset(&msg, 0, sizeof(msg));
    //msg.payload = buf;
    
//     std::string jsonstring = R"({
//     "time": "2023-05-05 22:00:05",
//     "data": {
//         "Airline": "3U",
//         "FlightNo": "5510",
//         "CraftModel": "BD700",
//         "Code": "211L",
//         "PlanTime": "202104200700",
//         "FlightOrigin": "ZUHZGSD",
//         "FlightDes": "ZUHZGSD"
//     }
// })";

    mqtt_connect(client);
    
    mqtt_subscribe(client,"IrsAlarmMessageTop",QOS0,on_Messagearrived);

    mqtt_subscribe(client,"ControlCommand",QOS0,control_Message);

    mqtt_subscribe(client,"zgsd/dgs/2/data/basic",QOS0,zgsd_basic);

    mqtt_subscribe(client,"zgsd/dgs/2/data/guidance",QOS0,zgsd_guidance);

    mqtt_subscribe(client,"zgsd/dgs/2/data/aircraft",QOS0,zgsd_aircraft);

    mqtt_subscribe(client,"zgsd/dgs/2/data/chock",QOS0,zgsd_chock);

    //mqtt_subscribe(client,"")


    while(1)
    {
        //cout<< "我进来了！"<<endl;
        time_t t =time(0);
        char tmp[32];
        strftime(tmp,sizeof(tmp),"%Y-%m-%d %H:%M:%S",localtime(&t));
        //mtx_cache.lock();
        string timeNow(tmp);
        string timeEst = formatPlantime(datacache.plantime);
        
        jsoninfo.time = timeNow;
        jsoninfo.code = "217";
        if (datacache.airlinecode.size()!=0)
        {
            jsoninfo.airline = datacache.airlinecode.substr(0,2);
            jsoninfo.flightno = datacache.airlinecode.substr(2,4);
        }
        else
        {
            jsoninfo.airline = "";
            jsoninfo.flightno = "";
        }
        
        
        jsoninfo.craftmodel = datacache.crafttype;
        //jsoninfo.bridgeposition = 
        jsoninfo.plantime = datacache.plantime;
        jsoninfo.flightorigin = datacache.airlinestartplace;
        jsoninfo.flightdes = datacache.airlineendplace;
        // jsoninfo.powersts = 
        // jsoninfo.airsts =
        // jsoninfo.weatherinfo = 
        // jsoninfo.baggageinfo = 

        //mtx_cache.unlock();
        
        
        nlohmann::json JDLCsendjson,BWYDflight,BWYDground;

        JDLCsendjson["data"]["Code"] = jsoninfo.code;
        JDLCsendjson["data"]["Airline"] = jsoninfo.airline;
        JDLCsendjson["data"]["FlightNo"] = jsoninfo.flightno;
        JDLCsendjson["data"]["CraftModel"] = jsoninfo.craftmodel;
        //JDLCsendjson["data"]["BridgePosition"] = jsoninfo.bridgeposition;
        JDLCsendjson["data"]["PlanTime"] = jsoninfo.plantime;
        
        JDLCsendjson["data"]["FlightOrigin"] = jsoninfo.flightorigin;
        JDLCsendjson["data"]["FlightDes"] = jsoninfo.flightdes;
        //JDLCsendjson["data"]["PowerSts"] = jsoninfo.powersts;
        //JDLCsendjson["data"]["AirSts"] = jsoninfo.airsts;
        //JDLCsendjson["data"]["Weatherinfo"] = jsoninfo.weatherinfo;
        //JDLCsendjson["data"]["baggageinfo"] = jsoninfo.baggageinfo;
        JDLCsendjson["time"] = jsoninfo.time;

        string jsonstring = JDLCsendjson.dump();

        msg.payload = (char *)jsonstring.c_str();
        mqtt_publish(client,pulishtopic,&msg);



        //if (isWithin30Minutes(timeNow,timeEst))
        if(1)
        {

            BWYDflight["flightno"] = datacache.airlinecode;
            BWYDflight["actype"] = datacache.crafttype;
            BWYDflight["flightcode"] = "";
            BWYDflight["arrival"] = (datacache.inoutsign == "A");
            BWYDflight["etx"] = timeEst;
            BWYDflight["other_port"] = (datacache.inoutsign == "A" ? datacache.startplacecode.substr(0,3):datacache.endplacecode.substr(0,3));
            BWYDflight["other_port_name"] = (datacache.inoutsign == "A" ? datacache.startPlace:datacache.endPlace);
            BWYDflight["stand"] = "218";
            BWYDflight["remaining"] = 34;
            BWYDflight["centerline"] = "C";
            BWYDflight["eibt"] = timeEst;
            BWYDflight["tobt"] = timeEst;
            BWYDflight["tsat"] = timeEst;
            jsonstring = BWYDflight.dump();
            msg.payload = (char *)jsonstring.c_str();
            mqtt_publish(client,"zgsd/dgs/2/update/flight",&msg);

            
            //cout << BWYDflight << endl;

            if (datacache.inoutsign == "A")
            {
                BWYDground["pbb"] = false;
            }else// if (datacache.inoutsign == "D")
            {
                BWYDground["pbb"] = true;
            }
            BWYDground["gpu"] = nlohmann::json::array();
            BWYDground["pca"] = nlohmann::json::array();
            BWYDground["well"] = nlohmann::json::array();

            //BWYDground["gpu"].push_back(datacache.powersts==1);
            BWYDground["gpu"].push_back(true);
            //BWYDground["gpu"].push_back(true);
            //BWYDground["pca"].push_back(datacache.airsts==1);
            BWYDground["pca"].push_back(true);
            //BWYDground["pca"].push_back(true);
            //BWYDground["well"].push_back(true);
            //BWYDground["well"].push_back(true);
            BWYDground["well"].push_back(datacache.wellsts);
            // for(int i = 0; i<datacache.wellsts.size();i++)
            // {
            //     //BWYDflight["well"].push_back(datacache.wellsts[i]==1);                
            // }
            BWYDground["supervise"] = true;
            BWYDground["service"] = true;
            BWYDground["luggage"] = true;
            jsonstring = BWYDground.dump();
            msg.payload = (char *)jsonstring.c_str();
            mqtt_publish(client,"zgsd/dgs/2/update/ground",&msg);
            //cout << BWYDground << endl;
        }



        mqtt_sleep_ms(1000);
        //mqtt_disconnect(client);
    }
}



void Task_getFlightInfo()
{
    sqlite3 *db;
    char *err_msg = 0;

    while(1)
    {
        int rc = sqlite3_open("singleDB.db", &db);
        //cout<< rc <<endl;
        if (rc)
        {
            fprintf(stderr, "task_getflightinfo 无法打开数据库: %s\n", sqlite3_errmsg(db));
            //return 1;
            continue;
        }
        else 
        {
            //fprintf(stderr, "成功打开数据库\n");
        }

        
        char **buf,**upbuf;
        string sql = "select flightNum,bizKey,aircraftType,inoutsign,planTime,airportCode_start,airportCode_end,startPlace,endPlace,strftime('%m',arrivetime),strftime('%d',arrivetime),strftime('%H',arrivetime),strftime('%M',arrivetime),strftime('%m',leavetime),strftime('%d',leavetime),strftime('%H',leavetime),strftime('%M',leavetime) from flightInfo where workend = 1 order by arrivetime;";
        
        mtx_db.lock();
        rc = sqlite3_get_table(db, sql.c_str(), &buf, &uprow, &upcol, &err_msg);
        mtx_db.unlock();
        if (rc == SQLITE_OK)
        {
            for (int i = 0 ; i<80; i++)
            {
                if (i<uprow)
                {
                    flightInfo[i].flightNum = buf[i*17+17];
                    flightInfo[i].bizKey = buf[i*17+18];
                    flightInfo[i].aircraftType = buf[i*17+19];
                    flightInfo[i].inoutsign = buf[i*17+20];

                    flightInfo[i].plantime = buf[i*17+21];
                    flightInfo[i].startplacecode = buf[i*17+22];
                    flightInfo[i].endplacecode = buf[i*17+23];
                    flightInfo[i].startPlace = buf[i*17+24];
                    flightInfo[i].endPlace = buf[i*17+25];
                    
                    //flightInfo[i].arriveTime = buf[i*8+4];
                    //flightInfo[i].leaveTime = buf[i*8+5];
                    if (!flightInfo[i].arriveTime.empty())
                    {
                        
                        flightInfo[i].arriveMonth = (uint16_t)atoi(buf[i*17+26]);
                        flightInfo[i].arriveDay = (uint16_t)atoi(buf[i*17+27]);
                        flightInfo[i].arriveHour = (uint16_t)atoi(buf[i*17+28]);
                        flightInfo[i].arriveMinute = (uint16_t)atoi(buf[i*17+29]);
                        
                    }
                    else
                    { 
                        flightInfo[i].arriveMonth = (uint16_t)0;
                        flightInfo[i].arriveDay = (uint16_t)0;
                        flightInfo[i].arriveHour = (uint16_t)0;
                        flightInfo[i].arriveMinute = (uint16_t)0;
                    }
                    
                    if (!flightInfo[i].leaveTime.empty())
                    {
                        flightInfo[i].leaveMonth = (uint16_t)atoi(buf[i*17+30]);
                        flightInfo[i].leaveDay = (uint16_t)atoi(buf[i*17+31]);
                        flightInfo[i].leaveHour = (uint16_t)atoi(buf[i*17+32]);
                        flightInfo[i].leaveMinute = (uint16_t)atoi(buf[i*17+33]);
                    }
                    else
                    {
                        flightInfo[i].leaveMonth = (uint16_t)0;
                        flightInfo[i].leaveDay = (uint16_t)0;
                        flightInfo[i].leaveHour = (uint16_t)0;
                        flightInfo[i].leaveMinute = (uint16_t)0;
                    }
                    
                }else
                {
                    flightInfo[i].flightNum = "";
                    flightInfo[i].bizKey = "";
                    flightInfo[i].aircraftType = "";
                    flightInfo[i].startPlace = "";
                    flightInfo[i].endPlace = "";
                    flightInfo[i].inoutsign = "";
                    flightInfo[i].plantime = "";
                    flightInfo[i].placecode = "";
                    flightInfo[i].startplacecode = "";
                    flightInfo[i].endplacecode = "";
                    flightInfo[i].arriveTime = "";
                    flightInfo[i].leaveTime = "";
                    flightInfo[i].arriveMonth = 0;
                    flightInfo[i].arriveDay = 0;
                    flightInfo[i].arriveHour = 0;
                    flightInfo[i].arriveMinute = 0;
                    flightInfo[i].leaveMonth = 0;
                    flightInfo[i].leaveDay = 0;
                    flightInfo[i].leaveHour = 0;
                    flightInfo[i].leaveMinute = 0;
                }
            }
            sqlite3_free_table(buf); // 释放结果
        }
        else
        {
            //  if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
            //  {
            //      continue;
            //  }
            // else
            {
            std::cerr << "查询失败: " << err_msg << std::endl;
            sqlite3_free(err_msg); // 释放错误消息   
            }       
            //continue;
        }
        
        //mtx_cache.lock();
        datacache.airlinecode = flightInfo[0].flightNum;
        datacache.bizKey = flightInfo[0].bizKey;
        datacache.crafttype = flightInfo[0].aircraftType;
        //datacache.guard = 
        //datacache.service = 
        //datacache.bridgeposition =
        datacache.startPlace = flightInfo[0].startPlace;
        datacache.startplacecode = flightInfo[0].startplacecode;
        datacache.endPlace = flightInfo[0].endPlace;
        datacache.endplacecode = flightInfo[0].endplacecode; 
        datacache.inoutsign = flightInfo[0].inoutsign;
        datacache.plantime = flightInfo[0].plantime;
        datacache.predictarrivetime = flightInfo[0].arriveTime;
        datacache.predictleavetime = flightInfo[0].leaveTime;
        datacache.airlinestartplace = flightInfo[0].startplacecode;
        datacache.airlineendplace = flightInfo[0].endplacecode;
        datacache.powersts = psData.power_state;
        datacache.airsts = acData.air_state;
        datacache.wellsts = aWellL.running_state & aWellM.running_state & aWellR.running_state & eWell.running_state & eWellDou.running_state & sWellL.running_state & sWellM.running_state & sWellR.running_state;
        //datacache.wellsts.pushback(aWell.running_state);
        //datacache.wellsts.pushback(eWell.running_state);
        //datacache.wellsts[2] = sWell.running_state;
        // datacache.wellsts[3] = 
        // datacache.wellsts[4] = 
        // datacache.wellsts[5] = 
        // datacache.wellsts[6] = 
        // datacache.wellsts[7] = 
        // datacache.weathersts = 
        // datacache.baggagests =
        //mtx_cache.unlock();
        sqlite3_close(db);
        chrono::milliseconds sleep_dur(2000);
        this_thread::sleep_for(sleep_dur);
    }
}


void Task_getVolumeInfo()
{
    sqlite3 *db;
    char *err_msg = 0;

    while(1)
    {
        int rc = sqlite3_open("singleDB.db", &db);
        //cout<< rc <<endl;
        if (rc)
        {
            fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
            //return 1;
            continue;
        }
        else 
        {
            //fprintf(stderr, "成功打开数据库\n");
        }

        
        
        char **buf;
        string sql = "select flightNum,bizKey,guaranteeStart,guaranteeEnd,guaranDuration,elecDuration,elecQuantity,airDuration,airQuantity,clearwaterDuration,clearwaterQuantity,flushwaterDuration,flushwaterQuantity,sewagewaterDuration,sewagewaterQuantity from business_device_usage order by id;";
        
        mtx_db.lock();
        rc = sqlite3_get_table(db, sql.c_str(), &buf, &volumerow, &volumecol, &err_msg);
        mtx_db.unlock();
        if (rc == SQLITE_OK)
        {
            for (int i = 0 ; i<80; i++)
            {
                if (i<volumerow)
                {
                    volumeInfo[i].flightNum = buf[i*15+15];
                    //cout<<"test"<<endl;
                    volumeInfo[i].bizKey = buf[i*15+16];
                    volumeInfo[i].coverage_start = buf[i*15+17];
                    volumeInfo[i].coverage_end = buf[i*15+18];
                    volumeInfo[i].coverage_duration = atof(buf[i*15+19]);
                    volumeInfo[i].power_duration = atof(buf[i*15+20]);
                    volumeInfo[i].power_consumption = atof(buf[i*15+21]);
                    volumeInfo[i].wind_duration = atof(buf[i*15+22]);
                    volumeInfo[i].wind_volume = atof(buf[i*15+23]);
                    volumeInfo[i].clean_duration = atof(buf[i*15+24]);
                    volumeInfo[i].clean_volume = atof(buf[i*15+25]);
                    volumeInfo[i].flush_duration = atof(buf[i*15+26]);
                    volumeInfo[i].flush_volume = atof(buf[i*15+27]);
                    volumeInfo[i].sewage_duration = atof(buf[i*15+28]);
                    volumeInfo[i].sewage_volume = atof(buf[i*15+29]);     
                }else
                {
                    volumeInfo[i].flightNum = "";
                    volumeInfo[i].coverage_start = "";
                    volumeInfo[i].coverage_end = "";
                    volumeInfo[i].coverage_duration = 0;
                    volumeInfo[i].power_duration = 0;
                    volumeInfo[i].power_consumption = 0;
                    volumeInfo[i].wind_duration = 0;
                    volumeInfo[i].wind_volume = 0;
                    volumeInfo[i].clean_duration = 0;
                    volumeInfo[i].clean_volume = 0;
                    volumeInfo[i].flush_duration = 0;
                    volumeInfo[i].flush_volume = 0;
                    volumeInfo[i].sewage_duration = 0;
                    volumeInfo[i].sewage_volume = 0;
                }
            }
            sqlite3_free_table(buf); // 释放结果
        }
        else
        {
            //  if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
            //  {
            //      continue;
            //  }
            // else
            {
            std::cerr << "查询失败: " << err_msg << std::endl;
            sqlite3_free(err_msg); // 释放错误消息   
            }       
            //continue;
        }
        sqlite3_close(db);
        chrono::milliseconds sleep_dur(2000);
        this_thread::sleep_for(sleep_dur);
    }
}




void Task_well()
{
    sqlite3 *db;
    char *err_msg = 0;
    //将deviceTpye转变为设备代码
    //----

    

    while(1)
    {
        if (!PLCisConnect)
        {
            continue;
        }
        int rc = sqlite3_open("singleDB.db", &db);
        //cout<< rc <<endl;
        if (rc)
        {
            fprintf(stderr, "task_well无法打开数据库: %s\n", sqlite3_errmsg(db));
            //return 1;
            //continue;
        }
        else 
        {
            //fprintf(stderr, "成功打开数据库\n");
        }
        string insertsql = splice_airwell(aWellL);
        //cout<< " Well sql :" << insertsql<< endl;
        // mtx_db.lock();
        // int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
        // }
        insertsql += splice_airwell(aWellM);
        //cout<< " Well sql :" << insertsql<< endl;
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
        // }
        insertsql += splice_airwell(aWellR);
        //cout<< " Well sql :" << insertsql<< endl;
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
        // }

        //将deviceTpye转变为设备代码
        //----

        insertsql += splice_elewell(eWell);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);   
        // }

        insertsql += splice_elewell(eWellDou);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);   
        // }
        //将deviceTpye转变为设备代码
        //----


        insertsql += splice_swgwell(sWellL);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }

        insertsql += splice_swgwell(sWellM);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }

        insertsql += splice_swgwell(sWellR);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }

        
        powerKey = volumeKey & 0x01;
        condKey = (volumeKey >> 1) & 0x01;
        cleanKey = (volumeKey >> 2) & 0x01;
        flushKey = (volumeKey >> 3) & 0x01;
        sewegaKey = (volumeKey >> 4) & 0x01;
        if (powerKey||condKey||cleanKey||flushKey||sewegaKey)
        {
            insertsql += splice_dosage(vData);
            // mtx_db.lock();
            // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
            // mtx_db.unlock();
            // if (inrc != SQLITE_OK)
            // {
            //     cout<<err_msg<<endl;
            //     sqlite3_free(err_msg);
                    
            // }
        }

        insertsql += splice_power(psData);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }

        insertsql += splice_power(psData2);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }

        insertsql += splice_aircond(acData);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<<"air1:"<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }

        insertsql += splice_aircond(acData2);
        mtx_db.lock();
        int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        mtx_db.unlock();
        if (inrc != SQLITE_OK)
        {
            cout<<"wellInsert error:"<< err_msg<<endl;
                sqlite3_free(err_msg);
                
        }


        sqlite3_close(db);
        chrono::milliseconds sleep_dur(1000);
        this_thread::sleep_for(sleep_dur);
    }

    

    //将deviceTpye转变为设备代码
    //----

    // insertsql = splice_sigldevice(wdData);
    // mtx_db.lock();
    // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
    // mtx_db.unlock();
    // if (inrc != SQLITE_OK)
    // {
    //     cout<< err_msg<<endl;
    //         sqlite3_free(err_msg);
            
    // }

    // //faultinfo
    // insertsql = splice_faultInfo(aWell,eWell,sWell,wdData);
    // mtx_db.lock();
    // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
    // mtx_db.unlock();
    // if (inrc != SQLITE_OK)
    // {
    //     cout<< err_msg<<endl;
    //         sqlite3_free(err_msg);
    // }
}

void monitor_bool(atomic<bool>& target) 
{
    using namespace std::chrono;
    auto last_false = steady_clock::time_point();
    bool is_timing = false;

    while (true)
    {
        if(!target.load())
        {
            if(!is_timing)
            {
                last_false = steady_clock::now();
                is_timing = true;
            }else if (steady_clock::now() - last_false >= 5s)
            {
                /* code */
                target.store(true);
                is_timing = false;
            }
            
        }else{
            is_timing = false;
        }
        std::this_thread::sleep_for(100ms);
    }
}


int main()
{
   
    

    std::thread([](std::atomic<bool>& f) { monitor_bool(f);},std::ref(vdgsFault)).detach();
    std::thread([](std::atomic<bool>& f) { monitor_bool(f);},std::ref(itMoinitor)).detach();

    sqlite3 *db;
    char *err_msg = 0;

    char **devbuf;
    string sql = "select ONLY_NUMBER,DEVICE_NUMBER from DEVICE_NUMBER order by ID;";
    int devrow,devcol;
   
    

    int rc = sqlite3_open("singleDB.db", &db);
    //cout<< rc <<endl;
    if (rc)
    {
        fprintf(stderr, "devicenumber无法打开数据库: %s\n", sqlite3_errmsg(db));
        //return 1;
        //continue;
    }
    else 
    {
        //fprintf(stderr, "成功打开数据库\n");
    }
    mtx_db.lock();
    rc = sqlite3_get_table(db, sql.c_str(), &devbuf, &devrow, &devcol, &err_msg);
    mtx_db.unlock();
    
    if (rc == SQLITE_OK)
    {
        for (int i = 0 ; i<devrow; i++)
        {
            if (i<devrow)
            {
                string str = devbuf[i*2 + 2];
                //cout<< str <<endl;
                deviceNum[str] = devbuf[i*2 + 3];
                //cout<<deviceNum[str]<<endl;
            }
        }
        sqlite3_free_table(devbuf); // 释放结果
    }
    else
    {
        //  if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
        //  {
        //      continue;
        //  }
        // else
        {
        std::cerr << "查询失败: " << err_msg << std::endl;
        sqlite3_free(err_msg); // 释放错误消息   
        }       
        //continue;
    }
    sqlite3_close(db);
    
    if (deviceNum["localseat"] != "")
    {
        LocalSeat = deviceNum["localseat"];
    }
    else
    {
        LocalSeat = "无机位号";
    }
    thread first(Task_getFlightInfo);
    thread secend(Task_JDLC);
    thread third(Task_well);
    //thread fouth(Task_getVolumeInfo);
    thread fifth(Task_Fault);
    first.detach();
    secend.detach();
    third.detach();
    //fouth.detach();
    fifth.detach();
    
    //PLC Connect
    const char* PLC_IP = "192.168.0.113";
    Profinet profinet_(PLC_IP);
    int PLC_RACK_NUMBER = 0;
    int PLC_SLOT_NUMBER = 1;
    int result = 0;
    
    bool delSigl = false;
    TS7Client* client_ = new TS7Client();

    while(1)
    {
        if ((result = client_->ConnectTo(PLC_IP, PLC_RACK_NUMBER, PLC_SLOT_NUMBER) != 0)) 
        {
            std::cerr << "Connect PLC Error ! err msg : "
                << CliErrorText(result).data() << std::endl;
            PLCisConnect = false;
            sleep(5);
            continue;
        }
        else
        {
            PLCisConnect = true;
            //std::cout << "Connect PLC Success !" << std::endl;
        }
        //char sql[255] = {0};
        //sprintf(sql,"select * from flightInfo where sendFlag = 0;");
        
        
        //sleep(1);
        int rc = sqlite3_open("singleDB.db", &db);
        //cout<< rc <<endl;
        if (rc)
        {
            fprintf(stderr, "main 无法打开数据库: %s\n", sqlite3_errmsg(db));
            //return 1;
            continue;
        }
        else 
        {
            //fprintf(stderr, "成功打开数据库\n");
        }

        
        
        
        

        //航班信息下发
        //
        int uprow;
        int upcol;
        char **upbuf;
        string upsql = "select * from flightInfo where sendFlag = 1;";
        mtx_db.lock();
        int uprc = sqlite3_get_table(db,upsql.c_str(),&upbuf,&uprow,&upcol,&err_msg);
        mtx_db.unlock();
        if ((uprow != 0 && uprc == SQLITE_OK) or delSigl == true)
        {
            profinet_.DownloadFlightInfo(uprow,flightInfo,client_);
            string sql = "UPDATE flightInfo set sendFlag = 0 where sendFlag = 1;";
            mtx_db.lock();
            rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
            mtx_db.unlock();
            if (rc != SQLITE_OK)
            {
                std::cerr << "SQL错误: " << err_msg << std::endl;
                sqlite3_free(err_msg);
            } else 
            {
                std::cout << "数据已更新" << std::endl;
                cout<< uprow <<endl;
                delSigl = false;
            }
        }
        // else{
            
        // }

        //用时用量信息下发
        //
        int vrow;
        int vcol;
        char **vbuf;
        //
        //cout<< "Mark1"<<endl;
        string volumesql = "select * from business_device_usage where sendFlag =1;";
        mtx_db.lock();
        int volumerc = sqlite3_get_table(db,volumesql.c_str(),&vbuf,&vrow,&vcol,&err_msg);
        mtx_db.unlock();
        //cout<<volumerc<<endl;
        if ((volumerc == SQLITE_OK && vrow!=0) or delSigl == true)//vrow != 0 && 
        {
            profinet_.DownloadVolumeInfo(volumerow,volumeInfo,client_);
            sql = "UPDATE business_device_usage set sendFlag = 0 where sendFlag = 1;";
            mtx_db.lock();
            volumerc = sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
            mtx_db.unlock();
            if (volumerc != SQLITE_OK)
            {
                std::cerr << "用时用量SQL错误: " << err_msg << std::endl;
                sqlite3_free(err_msg);
            } else 
            {
                std::cout << "用时用量数据已更新" << std::endl;
                //cout<< volumerow <<endl;
                delSigl = false;
            }
        }
        // else{
        //      std::cerr <<  err_msg << std::endl;
        //         sqlite3_free(err_msg);
        // }

        

        
        //航班信息添加
        
        flightInfoData inFInfo,delInfo;
        client_ -> DBRead(6,5508,2,&insertFlag);
        insertFlag = profinet_.S7_reverse_endianess(insertFlag);
        //delete
        client_ -> DBRead(6,5578,2,&deleteFlag);
        deleteFlag = profinet_.S7_reverse_endianess(deleteFlag);
        

        if (insertFlag == 1)
        {
            inFInfo = profinet_.InsertFlightInfo(client_,inFInfo);
            string insertsql = splice_insert(inFInfo);
            mtx_db.lock();
            int inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
            mtx_db.unlock();
            if (inrc == SQLITE_OK)
            {
                uint16_t zero = profinet_.S7_reverse_endianess(0);
                client_ ->DBWrite(6,5508,2,&zero);
            }
            else
            {
                cout<< err_msg<<endl;
                 sqlite3_free(err_msg);
            }
        }

        //delete info
        if (deleteFlag == 1)
        {
            delInfo = profinet_.DeleteFlightInfo(client_,delInfo);
            string delsql = splice_delete(delInfo);
            
            cout<<delsql<<endl;
            mtx_db.lock();
            int delrc = sqlite3_exec(db,delsql.c_str(),0,0,&err_msg);
            mtx_db.unlock();
            if (delrc == SQLITE_OK)
            {
                uint16_t zero = profinet_.S7_reverse_endianess(0);
                client_ ->DBWrite(6,5578,2,&zero);
                delSigl = true;
            }
            else
            {
                cout<<err_msg<<endl;
                 sqlite3_free(err_msg);
            }
          }
          sqlite3_close(db);
            

        //wellInfo
        //airwell Info
        

        //time
        // int wellFlag;
        // int tmp,count=0;
        // client_ -> MBRead(0,1,&tmp);
        // wellFlag = (tmp >> 7) & 0x01;
        
        //空调井信息
        aWellL = profinet_.GetairWellLeftInfo(client_);
        aWellM = profinet_.GetairWellMidInfo(client_);
        aWellR = profinet_.GetairWellRightInfo(client_);

        //电源井信息
        eWell = profinet_.GetelecWellInfo(client_);
        eWellDou = profinet_.GetelecWellDouInfo(client_);

        //污水井信息
        sWellL = profinet_.GetswgWellLeftInfo(client_);
        sWellM = profinet_.GetswgWellMidInfo(client_);
        sWellR = profinet_.GetswgWellRightInfo(client_);

        //空调井错误信息
        aWellFDataL = profinet_.GetMixAirwellLeftFaultData(client_);
        aWellFDataM = profinet_.GetMixAirwellMidFaultData(client_);
        aWellFDataR = profinet_.GetMixAirwellRightFaultData(client_);

        //电源井错误信息
        eWellFData = profinet_.GetsingleElecwellFaultData(client_);
        eWellFDataDou = profinet_.GetMixElecwellFaultData(client_);

        //污水井报错信息
        sWellFDataL = profinet_.GetMixSwgwellFaultLeftData(client_);
        sWellFDataM = profinet_.GetMixSwgwellFaultMidData(client_);
        sWellFDataR = profinet_.GetMixSwgwellFaultRightData(client_);

        //singlewellDeviceData
        psData = profinet_.GetpowersupplyData(client_);
        psData2 = profinet_.Getpowersupply2Data(client_);
        acData = profinet_.GetairconditionData(client_);
        acData2 = profinet_.Getaircondition2Data(client_);
        
        sPowerFData = profinet_.GetsinglePowerFaultData(client_);
        sPowerFData2 = profinet_.GetDouPowerFaultData(client_);
        sCondFData = profinet_.GetsingleCondFaultData(client_);
        sCondFData2 = profinet_.GetDouCondFaultData(client_);

        //用时用量信息
        
        client_->DBRead(13,120,1,&volumeKey);   //标志位
        
        powerKey = volumeKey & 0x01;
        condKey = (volumeKey >> 1) & 0x01;
        cleanKey = (volumeKey >> 2) & 0x01;
        flushKey = (volumeKey >> 3) & 0x01;
        sewegaKey = (volumeKey >> 4) & 0x01;
        if (powerKey||condKey||cleanKey||flushKey||sewegaKey)
        {
            vData = profinet_.GetMixvolumeData(client_);
        }

        //清水泵站
        cwData  =profinet_.GetcleanwaterStation2Info(client_);


        client_->Disconnect();
        //sewageStationData


        //机位控制器报警
        uint16_t faultKey = 0;
        client_->DBRead(1,0,2,&faultKey);
        if(itMoinitor = false)
        {
            setBitTo(faultKey,0,true);
            client_->DBWrite(1,0,2,&faultKey);
        }else
        {
            setBitTo(faultKey,0,false);
            client_->DBWrite(1,0,2,&faultKey);
        }

        if(vdgsFault = false)
        {
            setBitTo(faultKey,1,true);
            client_->DBWrite(1,0,2,&faultKey);
        }else
        {
            setBitTo(faultKey,1,false);
            client_->DBWrite(1,0,2,&faultKey);
        }
        
        // swgstationData = profinet_.GetsewageStationInfo(client_);
        // insertsql = splice_swgStation(swgstationData);
        // mtx_db.lock();
        // inrc = sqlite3_exec(db, insertsql.c_str(), 0, 0, &err_msg);
        // mtx_db.unlock();
        // if (inrc != SQLITE_OK)
        // {
        //     cout<< err_msg<<endl;
        //         sqlite3_free(err_msg);
                
        // }
    } 
}