#include <iostream>
#include <cstdint>
#include "../snap7.h"
#include <map>
#include <vector>

using namespace std;


struct dataCache
{
    string airlinecode;
    string bizKey;
    string crafttype;
    int guard;
    int service;
    int bridgeposition;
    string plantime;
    string inoutsign;
    string startPlace;
    string startplacecode;
    string endPlace;
    string endplacecode;
    string predictarrivetime;
    string predictleavetime;
    string airlinestartplace;
    string airlineendplace;
    int powersts;
    int airsts;
    bool wellsts;
    int weathersts;
    int baggagests;
};


struct jsonS
{
    string time;
    string code;
    string airline;
    string flightno;
    string craftmodel;
    string bridgeposition;
    string plantime;
    string flightorigin;
    string flightdes;
    string powersts;
    string airsts;
    string wellsts[8];
    string weatherinfo;
    string baggageinfo;
};

struct flightInfoData
{
    /* data */
    string flightNum;
    string bizKey;
    string aircraftType;
    string inoutsign;
    string plantime;
    string placecode;
    string startPlace;
    string startplacecode;
    string endPlace;
    string endplacecode;
    string arriveTime;
    string leaveTime;
    uint16_t arriveMonth;
    uint16_t arriveDay;
    uint16_t arriveHour;
    uint16_t arriveMinute;
    uint16_t leaveMonth;
    uint16_t leaveDay;
    uint16_t leaveHour;
    uint16_t leaveMinute;
    uint16_t sendFlag;
    uint16_t upFlag;
};

struct volumeInfoData
{
    string flightNum,bizKey;
    string coverage_start;
    string coverage_end;
    float coverage_duration;
    float power_consumption,power_consumption1,power_consumption2;
    float power_duration,power_duration1,power_duration2;
    float wind_duration,wind_duration1,wind_duration2;
    float wind_volume,wind_volume1,wind_volume2;
    float clean_duration;
    float clean_volume;
    float flush_duration;
    float flush_volume;
    float sewage_duration;
    float sewage_volume;
};

struct airWellData
{
    string deviceType = "4-1";
    uint16_t running_state;
    uint16_t is_alarm;
    uint16_t is_stop;
    uint16_t hp_start;
    uint16_t psrv_work;
    uint16_t mwp_manual;
    uint16_t bwp_manual;
    uint16_t mwp_function;
    uint16_t bwp_function;
    uint16_t dd_function;
    uint16_t pv1_function;
    uint16_t pv2_function;
    uint16_t cw1_open;
    uint16_t cw1_close;
    uint16_t cw2_open;
    uint16_t cw2_close;
    uint16_t dd2_function;
    uint16_t gas_concentration;
    uint16_t ps_pressure;
    uint32_t cw_amount;
    string create_time;
};

struct elecWellData
{
    string deviceType = "3-1";
    uint16_t running_state;
    uint16_t is_alarm;
    uint16_t is_stop;
    uint16_t hp_start;
    uint16_t psrv_work;
    uint16_t mwp_manual;
    uint16_t bwp_manual;
    uint16_t mwp_function;
    uint16_t bwp_function;
    uint16_t dd_function;
    uint16_t connection_plug;
    uint16_t ms_plug;
    uint16_t dd2_function=0;
    uint16_t connection2_plug=0;
    uint16_t ms2_plug=0;
    uint16_t gas_concentration;
    uint16_t ps_pressure;
    uint16_t create_time;
};

struct sewageWellData
{
    string deviceType = "5-1";
    uint16_t running_state;
    uint16_t is_alarm;
    uint16_t is_stop;
    uint16_t hp_start;
    uint16_t psrv_work;
    uint16_t mwp_manual;
    uint16_t bwp_manual;
    uint16_t mwp_function;
    uint16_t bwp_function;
    uint16_t dd_function;
    uint16_t rmv_function;
    uint16_t sv_function;
    uint16_t rwv_sv_function;
    uint16_t rwv_sv_open;
    uint16_t rwv_sv_close;
    uint16_t sv_open;
    uint16_t sv_close;
    uint16_t rwv_open;
    uint16_t rwv_close;
    uint16_t gas_concentration;
    uint16_t ps_pressure;
    uint16_t rwv_amount;
    string create_time;
};



struct powersupplyData
{
    string deviceType = "1-1";
    uint16_t power_state;
    uint16_t alarm_code;
    uint input_freq;
    uint input_volt12;
    uint input_volt23;
    uint input_volt13;
    uint A_volt;
    uint B_volt;
    uint C_volt;
    uint frequency;
    uint A_curr;
    uint B_curr;
    uint C_curr;
    uint Dc_tmp;
    uint Ac1_tmp;
    uint Ac2_tmp;
    uint Ac3_tmp;
    uint trans_tmp;
    uint32_t total_out_time;
    uint32_t total_out_power;
};

struct airconditionData
{
    string deviceType = "2-1";
    uint air_state;
    uint newair_tmp;
    uint splyair_tmp;
    uint set_tmp;
    uint splyair_pres;
    uint sply_volume;
    uint newair_spd;
    uint splyair_spd;
    uint cmpsor1_high_ps;
    uint cmpsor1_low_ps;
    uint cmpsor2_high_ps;
    uint cmpsor2_low_ps;
    uint cmpsor3_high_ps;
    uint cmpsor3_low_ps;
    uint cmpsor4_high_ps;
    uint cmpsor4_low_ps;
    uint cmpsor5_high_ps = 0;
    uint cmpsor5_low_ps = 0;
    uint cmpsor6_high_ps = 0 ;
    uint cmpsor6_low_ps = 0;
    uint cond1_tt_time;
    uint cond2_tt_time;
    uint cond3_tt_time = 0;
    uint cmpsor1_tt_time;
    uint cmpsor2_tt_time;
    uint cmpsor3_tt_time;
    uint cmpsor4_tt_time;
    uint cmpsor5_tt_time = 0;
    uint cmpsor6_tt_time = 0;
    uint oprtion_time_H;
    uint oprtion_time_M;
    uint airsply_tt_time = 0;
    uint unit_oprtion_mode;
};


struct deviceFaultData
{
    map<string,uint16_t> datamap;
};

struct AirwellFaultData
{
    map<string,uint16_t> datamap;
    string deviceType = "4-1";
};

struct ElecwellFaultData
{
    map<string,uint16_t> datamap;
    string deviceType = "3-1";
};

struct SwgwellFaultData
{
    map<string,uint16_t> datamap;
    string deviceType = "5-1";
};

struct PowerFaultData
{
    map<string,uint16_t> datamap;
    string deviceType = "1-1";
};

struct AirCondFaultData
{
    map<string,uint16_t> datamap;
    string deviceType = "2-1";
};


struct sewageStationData
{
    int ejector1_auto;
    int ejector2_auto;
    int ejector3_auto;
    int spare1;
    int dc_y0_auto;
    int dc_y1_auto;
    int dc_time_switch;
    int dc_level_switch;
    int antipump_auto;
    int alarm_reset;
    int low_level_bypass;
    int ejector1_overload;
    int ejector2_overload;
    int ejector3_overload;
    int spare2;
    int antipump_overload;
    int dc_allowed;
    int antipump_fail;
    int ejector1_run;
    int ejector2_run;
    int ejector3_run;
    int spare3;
    int antiform_run;
    int udvolt_trip_reset;
    int dc_value1_close;
    int dc_value2_close;
    int common_alarm;
    int ejector_pump1_control;    
    int ejector_pump2_control;        
    int ejector_pump3_control;    
    int spare4;
    int antipump_control;
    int dc_value1_control;
    int dc_value2_control;
    int option_ll_level_fault;
    int vac_failure;
    int vac_collpse;
    int pump_stop_level;
    int spare5;
    int pump_start_level;
    int highlever_fault;
    int dc_stp_blocked;
    int antifoam_pump_overload;
    int antifoam_tank_lowlevel;
    int Y01_failure;
    int Y02_failure;
    uint16_t sensor_raw_value;
    uint16_t vac_pressure1_raw_value;
    uint16_t vac_pressure2_raw_value;
    uint32_t liquid_level_eng_value;
    uint32_t vac_ps1_value;
    uint32_t vac_ps2_value;
    int tankbody_ll_liquid_level;
    int tank_l_liquid_level;    
    int tank_h_liquid_level;
    int tank_hh_liquid_level;
    uint32_t dc_pump1_running;
    uint32_t dc_pump1_fault;
    uint32_t dc_pump2_running;
    uint32_t dc_pump2_fault;
    uint32_t dc_pump3_running;
    uint32_t dc_pump3_fault;
    uint32_t dc_pump1_running_nums;
    uint32_t dc_pump1_running_hours;
    uint32_t dc_pump1_running_mins;
    uint32_t dc_pump1_running_secs;
    uint32_t dc_pump2_running_nums;
    uint32_t dc_pump2_running_hours;
    uint32_t dc_pump2_running_mins;
    uint32_t dc_pump2_running_secs;
    uint32_t dc_pump3_running_nums;
    uint32_t dc_pump3_running_hours;
    uint32_t dc_pump3_running_mins;
    uint32_t dc_pump3_running_secs;
    uint32_t dc_valve1_running_nums;
    uint32_t dc_valve1_running_hours;
    uint32_t dc_valve1_running_mins;
    uint32_t dc_valve1_running_secs;
    uint32_t dc_valve2_running_nums;
    uint32_t dc_valve2_running_hours;
    uint32_t dc_valve2_running_mins;
    uint32_t dc_valve2_running_secs;
};

struct cleanwaterStationData
{
    /* data */
    int running_signal_1;
    float frequency_1;  // chuyi 10
    int totaltime_h_1;
    int totaltime_m_1;
    int totaltime_s_1;
    int running_signal_2;
    float frequency_2;
    int totaltime_h_2;
    int totaltime_m_2;
    int totaltime_s_2;
    float watertank1_lvl;
    float watertank2_lvl;
    float outlet1_prs;
    float outlet2_prs;
};

struct flushwaterStationData
{
    /* data */
    int running_signal_1;
    float frequency_1;  // chuyi 10
    int totaltime_h_1;
    int totaltime_m_1;
    int totaltime_s_1;
    int running_signal_2;
    float frequency_2;
    int totaltime_h_2;
    int totaltime_m_2;
    int totaltime_s_2;
    int medrunning_signal_1;
    float medfrequency_1;  // chuyi 10
    int medtotaltime_h_1;
    int medtotaltime_m_1;
    int medtotaltime_s_1;
    int medrunning_signal_2;
    float medfrequency_2;
    int medtotaltime_h_2;
    int medtotaltime_m_2;
    int medtotaltime_s_2;
    int medwatertank1_lvl;
    int medwatertank2_lvl;
    int medoutlet1_prs;
    int medoutlet2_prs;
    float medtank_lvl;
    float watertank1_lvl;
    float watertank2_lvl;
    float outlet1_prs;
    float outlet2_prs;
};

struct JDLC_BZJD
{
    string cameraId;
    string standNo;
    string event;
    string createTime;
    string type;
};

struct JDLC_WGZY
{
    string code;
    string type;
    string standNo;
    string createTime;
    string cameraId;
};

struct BWYD_basic
{
    string dgsid,stand,lockstand;
    int workmode,state,systemerror,adjlock;
};

struct BWYD_guidance
{
    string guidid,dgsid,stand,centerline,flightno,actype;
    int stopline,step,idstate,azstate,effect,alert;
    bool overspeed;
};

struct BWYD_aircraft
{
    string guidid;
    float distance,azimuth,speed;
};

struct BWYD_chock
{
    string chocktime,flightno;
    bool chock;
};


struct controlMessage
{
    string newsid;
    string secretKey;
    string instruction;
    string deviceNumber;
    string seatNumber;
};

class Profinet
{
public:
    Profinet();
    Profinet(const char *PLC_IP);
    ~Profinet();
    void DownloadFlightInfo(int row, flightInfoData flightInfo[], TS7Client *client_);
    void DownloadVolumeInfo(int row, volumeInfoData volumeInfo[], TS7Client *client_);
    flightInfoData InsertFlightInfo(TS7Client *client_, flightInfoData flightInfo);
    flightInfoData DeleteFlightInfo(TS7Client *client_, flightInfoData flightInfo);
    airWellData GetairWellInfo(TS7Client *client_);
    airWellData GetairWellLeftInfo(TS7Client *client_);
    airWellData GetairWellMidInfo(TS7Client *client_);
    airWellData GetairWellRightInfo(TS7Client *client_);
    elecWellData GetelecWellInfo(TS7Client *client_);
    elecWellData GetelecWellDouInfo(TS7Client *client_);
    sewageWellData GetswgWellInfo(TS7Client *client_);
    sewageWellData GetswgWellLeftInfo(TS7Client *client_);
    sewageWellData GetswgWellMidInfo(TS7Client *client_);
    sewageWellData GetswgWellRightInfo(TS7Client *client_);
    powersupplyData GetpowersupplyData(TS7Client *client_);
    powersupplyData Getpowersupply2Data(TS7Client *client_);
    airconditionData GetairconditionData(TS7Client *client_);
    airconditionData Getaircondition2Data(TS7Client *client_);

    sewageStationData GetsewageStationInfo(TS7Client *client_);
    cleanwaterStationData GetcleanwaterStationInfo(TS7Client *client_);
    cleanwaterStationData GetcleanwaterStation2Info(TS7Client *client_);
    flushwaterStationData GetflushwaterStationInfo(TS7Client *client_);
    flushwaterStationData GetflushwaterStation2Info(TS7Client *client_);
    AirwellFaultData GetsingleAirwellFaultData(TS7Client *client_);
    AirwellFaultData GetMixAirwellLeftFaultData(TS7Client *client_);
    AirwellFaultData GetMixAirwellMidFaultData(TS7Client *client_);
    AirwellFaultData GetMixAirwellRightFaultData(TS7Client *client_);
    ElecwellFaultData GetsingleElecwellFaultData(TS7Client *client);
    ElecwellFaultData GetMixElecwellFaultData(TS7Client *client_);
    SwgwellFaultData GetsingleSwgwellFaultData(TS7Client *client_);
    SwgwellFaultData GetMixSwgwellFaultLeftData(TS7Client *client_);
    SwgwellFaultData GetMixSwgwellFaultMidData(TS7Client *client_);
    SwgwellFaultData GetMixSwgwellFaultRightData(TS7Client *client_);
    PowerFaultData GetsinglePowerFaultData(TS7Client *client_);
    PowerFaultData GetDouPowerFaultData(TS7Client *client_);
    AirCondFaultData GetsingleCondFaultData(TS7Client *client_);
    AirCondFaultData GetDouCondFaultData(TS7Client *client_);
    volumeInfoData GetMixvolumeData(TS7Client *client_);

    void mytoupper(string &s);
    uint16_t S7_reverse_endianess(uint16_t value);
    uint32_t S32_reverse_endianess(uint32_t value);
    uint64_t S64_reverse_endianess(uint64_t value);
    float F32_reverse_endianess(float value);
    float Float_reverse_endianess(byte value[]);

private:
    const char *IP;

    wstring uint8ArrayToWString(const uint8_t *arr, size_t size);
    string WString2String(const std::wstring &ws);
    wstring String2WString(const std::string &s);
    string ReadStr(uint8_t str2[]);
    void WriteWStr(string str, int8_t *str1);
    void WriteStr(string str, int8_t *str1);
};