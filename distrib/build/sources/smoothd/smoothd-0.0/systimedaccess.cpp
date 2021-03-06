/* SysTimedAccess Module for the SmoothWall SUIDaemon                      */
/* For bringing external interface up or taking it down                    */
/* (c) 2007,2008 SmoothWall Ltd and Steven L. Pittman                      */
/* ----------------------------------------------------------------------  */
/* Original Author  : Lawrence Manning                                     */
/* Translated to C++: M. W. Houston                                        */
/* Refactored by    : Steven L. Pittman                                    */
/* Modified to add logging of rejected packets                             */
/* 09-07-08         : Steven L. Pittman                                    */
/* 10-06-26  add check for red interface active                            */
/*                  : Steven L. Pittman                                    */
/* 14-09-25 Convert to use netfilter's '-m time' feature                   */
/*                  : Neal Murphy                                          */

/* include the usual headers.  iostream gives access to stderr (cerr)      */
/* module.h includes vectors and strings which are important               */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>

#include "module.h"
#include "ipbatch.h"
#include "setuid.h"

extern "C" {
	int load(std::vector<CommandFunctionPair> & );
	int set_timed_access(std::vector<std::string> & parameters, std::string & response);
}

int load(std::vector<CommandFunctionPair> & pairs)
{
	/* CommandFunctionPair name("command", "function"); */
	CommandFunctionPair set_timed_access_function("settimedaccess", "set_timed_access", 0, 4);
	
	pairs.push_back(set_timed_access_function);
	
	return (0);
}

int set_timed_access(std::vector<std::string> & parameters, std::string & response)
{
	int error = 0;
	ConfigVAR settings("/var/smoothwall/timedaccess/settings");
	ConfigCSV config("/var/smoothwall/timedaccess/machines");
	std::vector<std::string>ipb;
	std::string::size_type n;
        std::string daysOfWeek = "";
	
	// Send 'em to John
	ipb.push_back("iptables -F timedaccess\n");

	// If enabled, generate the rules
	if (settings["ENABLE"] == "on")
	{
		// Days of the week the rule applies
		if (settings["DAY_0"]  == "on") daysOfWeek += ",Mon";
		if (settings["DAY_1"]  == "on") daysOfWeek += ",Tue";
		if (settings["DAY_2"]  == "on") daysOfWeek += ",Wed";
		if (settings["DAY_3"]  == "on") daysOfWeek += ",Thu";
		if (settings["DAY_4"]  == "on") daysOfWeek += ",Fri";
		if (settings["DAY_5"]  == "on") daysOfWeek += ",Sat";
		if (settings["DAY_6"]  == "on") daysOfWeek += ",Sun";
		if (daysOfWeek.length() > 0)
		{
			daysOfWeek[0] = ' ';
			daysOfWeek = " --weekdays" + daysOfWeek;
		}

		// For each IP, generate the rules
		for (int line = config.first(); line == 0; line = config.next())
		{
			const std::string &ip = config[0];
			
			if ((n = ip.find_first_not_of(IP_NUMBERS)) != std::string::npos) 
			{
				// illegal characters
				response = "Bad IP: " + ip;
				error = 1;
				return error;
			}
			if (settings["MODE"] == "ALLOW")
			{
				int startTime, stopTime;
				std::ostringstream ssStartHour, ssStartMin, ssStopHour, ssStopMin;

				// Convert the times to decimal; a day has 1440 minutes (0-1439).
				startTime = (strtol (settings["START_HOUR"].c_str(), NULL, 10)*60 +
					strtol (settings["START_MIN"].c_str(), NULL, 10)) - 1;
				if (startTime < 0) startTime = 0;
				stopTime = (strtol (settings["END_HOUR"].c_str(), NULL, 10)*60 +
					strtol (settings["END_MIN"].c_str(), NULL, 10)) + 1;
				if (stopTime > 1439) stopTime = 1439;
				// startTime and stopTime can now be used as end and start times when ALLOW must
				// be inverted to make two pairs of rules.
				ssStartHour << startTime/60;
				ssStartMin << startTime%60;
				ssStopHour << stopTime/60;
				ssStopMin << stopTime%60;
		
				if (startTime > 0)
				{
			ipb.push_back("iptables -A timedaccess -s " + ip + " -m time" +
						" --timestart 00:00:00" +
						" --timestop " + ssStartHour.str() + ":" + ssStartMin.str() + ":59" +
						daysOfWeek + " -j timedaction");
			ipb.push_back("iptables -A timedaccess -d " + ip + " -m time" +
						" --timestart 00:00:00" +
						" --timestop " + ssStartHour.str() + ":" + ssStartMin.str() + ":59" +
						daysOfWeek + " -j timedaction");
		}
				if (stopTime < 1439)
				{
					ipb.push_back("iptables -A timedaccess -s " + ip + " -m time" + 
						" --timestart " + ssStopHour.str() + ":" + ssStopMin.str() +
						" --timestop 23:59:59" +
						daysOfWeek + " -j timedaction");
					ipb.push_back("iptables -A timedaccess -d " + ip + " -m time" + 
						" --timestart " + ssStopHour.str() + ":" + ssStopMin.str() +
						" --timestop 23:59:59" +
						daysOfWeek + " -j timedaction");
				}
			} else {
				ipb.push_back("iptables -A timedaccess -s " + ip + " -m time" +
					" --timestart " + settings["START_HOUR"] + ":" + settings["START_MIN"] +
					" --timestop " + settings["END_HOUR"] + ":" + settings["END_MIN"] + ":59" +
					daysOfWeek + " -j timedaction");
				ipb.push_back("iptables -A timedaccess -d " + ip + " -m time" +
					" --timestart " + settings["START_HOUR"] + ":" + settings["START_MIN"] +
					" --timestop " + settings["END_HOUR"] + ":" + settings["END_MIN"] + ":59" +
					daysOfWeek + " -j timedaction");
			}
		}
	}
	
	error = ipbatch(ipb);
	if (error) response = "ipbatch failure when setting chain timedaccess";
	else response = "timed access set";

	return error;
}
