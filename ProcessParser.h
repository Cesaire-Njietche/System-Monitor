#pragma once

#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"
#include "util.h"


using namespace std;

class ProcessParser{
private:
    std::ifstream stream;
    public:
    static string getCmd(string pid);
    static vector<string> getPidList();
    static std::string getVmSize(string pid);
    static std::string getCpuPercent(string pid);
    static long int getSysUpTime();
    static std::string getProcUpTime(string pid);
    static string getProcUser(string pid);
    static vector<string> getSysCpuPercent(string coreNumber = "");
    static float getSysRamPercent();
    static string getSysKernelVersion();
    static int getNumberOfCores();
    static int getTotalThreads();
    static int getTotalNumberOfProcesses();
    static int getNumberOfRunningProcesses();
    static string getOSName();
    static std::string PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2);
    static bool isPidExisting(string pid);
};

// TODO: Define all of the above functions below:
vector<string> SplitLine(string line){
    istringstream buf(line);
    istream_iterator<string> beg(buf), end;
    vector<string> values(beg, end);
    return values;
}

string ProcessParser::getVmSize(string pid){
     //Declaring search attribute for file
    string name = "VmData";
    string value, line;
    float result;
    // Opening stream for specific file
    ifstream stream;
    Util::getStream((Path::basePath() + pid + Path::statusPath()), stream);
    while(std::getline(stream, line)){
        // Searching line by line
        
        if (line.compare(0, name.size(),name) == 0) {
            // slicing string line on ws for values using sstream
        
            vector<string> values = SplitLine(line);
            //conversion kB -> GB
            result = (stof(values[1])/float(1024*1024));
            break;
        }
    }
    return to_string(result);
}

string ProcessParser::getCpuPercent(string pid){
    string line;
    string value;
    float result;
    ifstream stream;
    Util::getStream((Path::basePath()+ pid + "/" + Path::statPath()), stream);
    getline(stream, line);
    vector<string> values = SplitLine(line);
    // acquiring relevant times for calculation of active occupation of CPU for selected process
    float utime = stof(ProcessParser::getProcUpTime(pid));
    float stime = stof(values[14]);
    float cutime = stof(values[15]);
    float cstime = stof(values[16]);
    float starttime = stof(values[21]);
    float uptime = ProcessParser::getSysUpTime();
    float freq = sysconf(_SC_CLK_TCK);
    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime/freq);
    result = 100.0*((total_time/freq)/seconds);
    return to_string(result);
}

string ProcessParser::getProcUpTime(string pid){
    ifstream stream;
    Util::getStream(Path::basePath() + pid + "/" + Path::statPath(), stream);
    string line;
    getline(stream, line);
    vector<string> values = SplitLine(line);
    return to_string(stof(values[13])/sysconf(_SC_CLK_TCK));
}

long int ProcessParser::getSysUpTime(){
    ifstream stream;
    Util::getStream(Path::basePath() + Path::upTimePath(), stream);
    string line;
    getline(stream, line);
    vector<string> values = SplitLine(line);

    return stoi(values[0]);
}

string ProcessParser::getProcUser(string pid){
    ifstream stream;
    Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);
    string key = "Uid:";
    string value = "", line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            vector<string> values = SplitLine(line);
            value = values[1];
            break;
        }
    }
    key = "x:"+value;
    stream.close();
    Util::getStream("/etc/passwd", stream);
    while(getline(stream, line)){
        if(line.find(key) != string::npos){
            value = line.substr(0, line.find(":"));
            return value;
        }
    }
    return "";
}

vector<string> ProcessParser::getPidList(){
    vector<string> container;
    DIR* dir;

    if(!(dir = opendir("/proc"))){
        throw std::runtime_error(std::strerror(errno));
    }
    while(dirent * dirp=readdir(dir)){
        if(dirp->d_type != DT_DIR){
            continue;
        }
        if(all_of(dirp->d_name, dirp->d_name + strlen(dirp->d_name), [](char c){return isdigit(c);})){
            container.push_back(dirp->d_name);
        }
    }
    if(closedir(dir)){
        throw std::runtime_error(std::strerror(errno));
    }
    return container;
}

bool ProcessParser::isPidExisting(string pid){
    bool found = false;
    for(string p: ProcessParser::getPidList()){
        if(p.compare(pid) == 0)
        {
            found = true;
            break;
        }
    }
    return found;
}

string ProcessParser::getCmd(string pid){
    ifstream stream;
    Util::getStream(Path::basePath() + pid + Path::cmdPath(), stream);
    string line;
    getline(stream, line);
    return line;
}

int ProcessParser::getNumberOfCores(){
    ifstream stream;
    Util::getStream(Path::basePath() +"cpuinfo",stream);
    string key = "cpu cores";
    string line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            vector<string> values = SplitLine(line);
            return stoi(values[3]);
        }
    }

}

vector<string> ProcessParser::getSysCpuPercent(string core){
    ifstream stream;
    Util::getStream(Path::basePath() + Path::statPath(), stream);
    string key = "cpu"+core;
    string line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            vector<string> values = SplitLine(line);
            return values;
        }
    }
    return vector<string>();
}

float get_sys_active_cpu_time(vector<string> values)
{
    return (stof(values[S_USER]) +
            stof(values[S_NICE]) +
            stof(values[S_SYSTEM]) +
            stof(values[S_IRQ]) +
            stof(values[S_SOFTIRQ]) +
            stof(values[S_STEAL]) +
            stof(values[S_GUEST]) +
            stof(values[S_GUEST_NICE]));
}

float get_sys_idle_cpu_time(vector<string>values)
{
    return (stof(values[S_IDLE]) + stof(values[S_IOWAIT]));
}

string ProcessParser::PrintCpuStats(vector<string> value1, vector<string> value2){
    float active = get_sys_active_cpu_time(value2) - get_sys_active_cpu_time(value1);
    float idle = get_sys_idle_cpu_time(value2) - get_sys_idle_cpu_time(value1);
    float total = active + idle;
    float result = 100.0*active/total;

    return to_string(result);
}

float ProcessParser::getSysRamPercent(){
    ifstream stream;
    Util::getStream(Path::basePath() + Path::memInfoPath(), stream);
    string key1 = "MemAvailable";
    string key2 = "MemFree";
    string key3 = "Buffers";
    string line;
    float mem_a;
    float mem_f;
    float buff;
    int found_all = 0;
    while(getline(stream, line)){
        if(line.compare(0, key1.size(), key1) == 0){
            vector<string> values = SplitLine(line);
            mem_a = stof(values[1]);
            found_all++;
        }
        if(line.compare(0, key2.size(), key2) == 0){
            vector<string> values = SplitLine(line);
            mem_f = stof(values[1]);
            found_all++;
        }
        if(line.compare(0, key3.size(), key3) == 0){
            vector<string> values = SplitLine(line);
            buff = stof(values[1]);
            found_all++;
        }
        if(found_all == 3)
            break;
    }
    return 100.0*(1 - (mem_f/(mem_a - buff)));
}
string ProcessParser::getSysKernelVersion(){
    ifstream stream;
    Util::getStream(Path::basePath() + Path::versionPath(), stream);
    string key = "Linux version";
    string line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            vector<string> values = SplitLine(line);
            return values[2];
        }
    }
}

string ProcessParser::getOSName(){
    ifstream stream;
    Util::getStream("/etc/os-release", stream);
    string key = "PRETTY_NAME";
    string line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            int pos = line.find("=");
            string result = line.substr(pos + 2, line.size() - pos - 3);
            return result;
        }
    }
}

int ProcessParser::getTotalThreads(){
    ifstream stream;
    string key = "Threads:";
    string line;
    int number = 0;
    for(string pid : ProcessParser::getPidList()){
        Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);
        while(getline(stream, line)){
            if(line.compare(0, key.size(), key) == 0){
                istringstream buf(line);
                istream_iterator<string> beg(buf), end;
                vector<string> values(beg, end);
                number += stoi(values[1]);
                break;
            }
        }
    }
    return number;
}

int ProcessParser::getTotalNumberOfProcesses(){
    ifstream stream;
    Util::getStream(Path::basePath() + Path::statPath(), stream);
    string key = "processes";
    string line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            return stoi(values[1]);
        }
    }
}

int ProcessParser::getNumberOfRunningProcesses(){
    ifstream stream;
    Util::getStream(Path::basePath() + Path::statPath(), stream);
    string key = "procs_running";
    string line;
    while(getline(stream, line)){
        if(line.compare(0, key.size(), key) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            return stoi(values[1]);
        }
    }
}