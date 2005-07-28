// C++ interface between AF4 and tc/netfilter statistics collection
extern "C" {
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <linux/pkt_sched.h>
#include <resolv.h>
#include <libnetlink.h>
#include <ll_map.h>
#include <rtm_map.h>
    // has to be C linkage, as iptables stuff wont compile with C++
#include "trafstats_iptables.h"
}

#include <string>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
#include "configvar.hpp"
#include "trafstats_core.hpp"



traf_stat_collection_item::traf_stat_collection_item() {
    Vtraf_stat in,out,tmp;
    Vtraf_iterator ia;
    StrSet_iterator d;
        
    // oh this would be sooo much easier in perl
    
    // make sure these are all empty
    collection_start.t.tv_sec = 0;
    collection_start.t.tv_usec = 0;
    collection_end.t.tv_sec = 0;
    collection_end.t.tv_usec = 0;
    
    int_devs.clear();
    ext_devs.clear();
    classes.clear();
    rules.clear();
    stats.clear(); 
    
    in.clear(); 
    out.clear();
    in = list_rules("prerouting-1");
    out = list_rules("postrouting-1");
    
    if(in.size() == 0 || out.size() == 0) {
        // odd we have no interfaces!
        // std::cerr << __LINE__ << " no internal interfaces!" << std::endl;
        // cant do anything sensible
        return; 
    }
    
    // timestamp this lot
    gettimeofday(&collection_start.t, NULL);
    for(ia = in.begin(); ia != in.end(); ia++) {
        // use a ref so debugger can see this stuff!
        const std::string &sdev = ia->dev;
        if(sdev.size() > 0) {
            int_devs.insert(sdev); // remember dev
            std::string label = sdev + std::string("_int_in"); 
            traf_stat &item = *ia;
            stats[label] = item;
        }
        else {
            // std::cerr << __LINE__ << " no dev in record" << std::endl;
        }
        
    } 
    
    for(ia = out.begin(); ia != out.end(); ia++) {
        const std::string &sdev = ia->dev;
        if(sdev.size() > 0) {
            // e.g ethA_int_out
            std::string label = sdev + std::string("_int_out");  
            traf_stat &item = *ia;
            stats[label] = item;
        }
        else {
            // std::cerr << __LINE__ << " no dev in record" << std::endl;
        }
    }
    
    // external interfaces should exist in iptables even if
    // not up.
    in.clear();
    in = list_rules("prerouting-2");
    for(ia = in.begin(); ia != in.end(); ia++) { 
        const std::string &sdev = ia->dev;
        if(sdev.size() > 0) {
            ext_devs.insert(sdev); // remember dev
            std::string label = sdev + std::string("_ext_in");
            traf_stat &item = *ia;  
            stats[label] = item;
        }
        else {
            // std::cerr << __LINE__ << " no dev in record" << std::endl;
        }
    } 
    out.clear();
    
    out = list_rules("postrouting-2");
    for(ia = out.begin(); ia != out.end(); ia++) {
        const std::string &sdev = ia->dev;
        if(sdev.size() > 0) {                
            std::string label = sdev + std::string("_ext_out");
            traf_stat &item = *ia;           
            stats[label] = item;
        }
        else {
            // std::cerr << __LINE__ << " no dev in record" << std::endl;
        }
        
    }
    
    if(int_devs.size() == 0 || ext_devs.size() == 0) {
        // std::cerr << __LINE__ << " devs are empty" << std::endl;
    }
    else 
        // go through internal then external devices
        for(d = int_devs.begin(); d != ext_devs.end(); d++) {
            // we get all classes for this device 
            if(d == int_devs.end())
                d = ext_devs.begin();
            if(d == ext_devs.end())
                break;
            
            
            tmp.clear();
            const std::string &dev = *d;
            tmp = list_rules_for_dev(dev);
            if(tmp.size() == 0) { 
                // std::cerr << __LINE__ << " rules are empty for " << dev << std::endl;
            }
            else 
                for(ia = tmp.begin(); ia != tmp.end(); ia++) {
                    // and store them 
                    std::string rule_name = traf_config.rule_name(ia->rule_num);
                    rules.insert(rule_name);
                    std::string label = *d + "_rule_" + rule_name; 
                    traf_stat &item = *ia;
                    stats[label] = item;
                    
                }
            // only try for classes if had any rules, otherwise traffic not there
            if(tmp.size() > 0) {
                tmp.clear();
                tmp = list_class(dev);
                if(tmp.size() == 0) { 
                    // std::cerr << __LINE__ << " classes are empty for " << dev << std::endl;
                }
                else
                    for(ia = tmp.begin(); ia != tmp.end(); ia++) {
                        // and store them
                        std::string class_name = traf_config.class_name(ia->classid);
                        classes.insert(class_name);                
                        std::string label = *d + "_class_" + class_name; 
                        traf_stat &item = *ia; 
                        stats[label] = item;
                    }
            }
            
        }
        
    // this should not have taken too long...
    gettimeofday(&collection_end.t, NULL);
    
}

Vstring traf_stat_collection_item::class_indexes_in_order() {
    traf_stat_hash_iterator si;
        
    Vstring idxes;
    Vstring_iterator fs;
    Vtraf_stat classstats; // tmp so can sort
    Vtraf_iterator i;
    unsigned int pos; 
        
    for(si = stats.begin(); si != stats.end(); si++) { 
        pos = si->first.find("_class_");
        if(pos != std::string::npos) {
            // a class 
            classstats.push_back(si->second);
        }
    }
    sort(classstats.begin(),classstats.end());
    for(i = classstats.begin(); i != classstats.end(); i++) {
        std::string classname = traf_config.class_name(i->classid);
        fs = find(idxes.begin(), idxes.end(), classname);
        if(fs == idxes.end())
            // only one copy of each
            idxes.push_back(classname);
    }
    return idxes;
    
}

// keeps latest known stats for each label in the latest_stats global.

void traf_stat_collection_item::compress () {
    traf_stat_hash_iterator si;
    StrSet erasethese;
    StrSet_iterator e;
    for(si = stats.begin(); si != stats.end(); si++) {
        const std::string &label = si->first;
        traf_stat &data =  si->second;
        
        if(latest_stats.count(label)) {

            traf_stat &latest = latest_stats[label];
            // see if things have changed since last time
            // have we wrapped around? Most likley new rules have been added
            if(data.bytes() < latest.bytes()) {
                std::ostringstream log;
                
                // log << "counter wrap around for " << label << " old = ("  << latest <<  ") new = (" << data << ")" << std::endl;
                // syslog(LOG_WARNING,log.str().c_str());
                
                // zero the older counters, get the last part of the
                // wrap around at least
                latest.stats.bytes = 0; 
                latest.stats.packets = 0; 
            }

            // now normal data cases
            if(data.bytes() == latest.bytes()) {
                // nothing has changed - dont keep this
                // std::cout << "erasing " << label << std::endl;
                erasethese.insert(label);
                
            }
            else {
                // update the latest to be this
                calculate_rates(latest, data); 
                // std::cout << "delta " << data.bytes() - latest.bytes() << " for " << label << std::endl;
                
                latest_stats[label] = data;
            }
        }
        else {
            // there is no latest so we are it
            // std::cout << "1st sample for " << label << std::endl;
            latest_stats[label] = data;
        }
    }
    // do erasing once iteration has finished - otherwise messes up loop
    for(e = erasethese.begin(); e != erasethese.end(); e++) {
        stats.erase(*e);
    }
    // are we empty now - so can loose the dev maps too
    // all there is left is the timestamp,
    // but that is enough to note that we did look but nothing
    // changed
        
    if(stats.size() == 0) {
        // std::cout << "now empty sample " << std::endl;
        int_devs.clear();
        ext_devs.clear();
        classes.clear();
        rules.clear();
    }
}

// indicate if the rules we need are minimally in place
// this uses an arrangement of IPtables with the SmoothWall AF
// the 'prerouting-1' mangle table is a point for counting all
// traffic comming from an external interface.
// if this is not there then we assume that none of the iptables
// infrastructure we need has been set up yet.
// constructor - go fetch current stats!

bool traffic_iptables_missing() {
    struct iptable_data res[1];
    
    // If the INTERNAL tables have no entres yet then inhibit all stats
    // collection.
    // try to fetch just one item from this table, dont care
    // about the readings, just care it the table reading fails.
    // fetch_counts is from trafstats_iptables.c - as the iptables
    // code will NOT compile directly into a C++ program.
    return(fetch_counts("prerouting-1", res, 1) == 0);
}


// This converts a numeric classid into a string representation
// with special treatment for "root" and "none"
// otherwise the classid is shown as top 16bits : bottom 16 bits
// if either of these is 0 it is omitted so top part only set 
// looks like 1: and bottom part only set looks like :1

std::string traf_stat::tc_classid(__u32 h) const {
    std::ostringstream out;
    if(h == TC_H_ROOT)
        out << "root";
    else if(h == TC_H_UNSPEC) 
        out << "none";
    else if (TC_H_MAJ(h) == 0)
        out << std::hex << TC_H_MIN(h);
    else if (TC_H_MIN(h) == 0)
        out << std::hex << (TC_H_MAJ(h)>>16) << ":";
    else
        out << std::hex << (TC_H_MAJ(h)>>16) << ":" << std::hex << TC_H_MIN(h);
    return out.str();
}
// need a < operator
// so can sort traf_stat in classid/parent order
// so each parent is followed by immediate children as that is what
// makes sense when stats are printed out.
// the purpose of this is to return true if a < b, false otherwise.
// 1st key is dev, then classid with "root" considered lowest and "none" highest
bool operator < (const traf_stat &a, const traf_stat &b) {

    // std::cout << "A= " << a << " B = " << b << std::endl;

    if(a.classid == b.classid && a.parentid == b.parentid &&
       a.dev == b.dev)
        return false; // if equal a not < b
    // do initial sort on dev, only carry  on if dev same
    if(a.dev != b.dev) {
        // std::cout << (a.dev < b.dev) << std::endl;
        return (a.dev < b.dev);
    }
       
    if(b.classid == "root") {
        // std::cout << "false as b root" << std::endl;
        return false; // no way a can be < b
    }
    if(b.classid == "none") {
        // std::cout << (a.classid != "none") << " as b none" << std::endl;
        return (a.classid != "none"); // a <= b
    }
    if(a.classid == "root") {
        // std::cout << "true as a root" << std::endl;
        return true; // root is first
    }
    if(a.parentid == "root" && b.parentid != "root") {
        // std::cout << "true as parent root" << std::endl;
        return true; // we are 1st level, other isnt so a less
    }
    if(b.parentid == "root" && a.parentid != "root") {
        // std::cout << "false as b parent root" << std::endl;
        return false; // other is  1st level,
    }
    if(a.parentid < b.parentid) {
        // std::cout << "true as < parent" << std::endl;
        return true; // smaller parents
    }
    if(a.classid < b.classid) {
        // std::cout << "true as < class" << std::endl;
        return true;
    }

    // std::cout << "false" << std::endl;
    return false; // give up
}

// need a traf_stat operator << so can print out everything.
// if the special error element has been set then print that,
// otherwise print the object
// Symbolic names, speeds etc are attached to the object with
// reference to the traf_config object.
std::ostream& operator << (std::ostream& out, const traf_stat& i) {
    if(i.error != "") {
        out << "ERROR: " << i.error << std::endl;
    }
    else if(i.dev != "" && i.dev != "unknown") {
        out << "dev=" << i.dev;
        double ispeed = traf_config.interface_speed(i.dev);
        if(ispeed == 0.0) {
            // have separate up and download speeds
            ispeed = traf_config.interface_speed(i.dev, "upload");
            out << "(upload=" << format_rate(ispeed);
            ispeed = traf_config.interface_speed(i.dev, "download"); 
            out << ", download=" << format_rate(ispeed) << ") ";
        }
        else {
            out << "(speed=" << format_rate(ispeed) << ") ";
        }
        out << std::endl;
    }

    if(i.is_class()) {
        out << "Class " << traf_config.class_name(i.classid) << "(" << i.classid << ") ";
        if(i.has_parent()) {
            out << traf_config.class_name(i.parentid) << "(" << i.parentid << ") ";
        }        
        if(i.rate >= 1.0) {
            out << " rate=" << format_rate(i.rate) << " ceiling=" << format_rate(i.ceiling) << " ";
        }
        out << std::endl;
        
    }
    else if(i.is_rule()) { 
        if(i.has_class()) {
            // rule names only relivant for rules attached to a class
            out << "Rule " << traf_config.rule_name(i.rule_num) << "(" << i.rule_num << ") ";
            out << traf_config.class_name(i.classid) << "(" << i.classid << ") ";
        }        
        else {
            out << "Rule (" << i.rule_num << ") ";
        }
        out << std::endl;
        
    }
    
    // now the calculated stats, floating point
    out << "bits per sec=" << i.bits_per_sec << " bytes_per_sec= " << i.bytes_per_sec << " pkts_per_sec= " << i.pkts_per_sec;
    // and the raw data, integers
    out << " raw data( bytes=" << i.bytes() << " packets=" << i.packets() << " drops=" << i.drops() << " overlimits=" << i.overlimits() << " bps=" << i.bps() << " pps=" << i.pps() << " qlen=" << i.qlen() << " backlog= " << i.backlog() << ")"; 
    out << std::endl;
    return out;
}

// print a vector of traf_stat by using the copy algorithm 
// to apply the << operator for traf_stat to each item in the vector
std::ostream& operator << (std::ostream& out, const Vtraf_stat & i) {
    std::copy(i.begin(), i.end(), std::ostream_iterator<traf_stat>(out, "\n"));
    return out;
}
// somehow the passed traf_stat_collection_item and everything in it is considerd as const
// this interferes with finding map methods

 std::ostream& operator << (std::ostream& out, const traf_stat_collection_item & i) { 
     
     traf_stat_hash_const_iterator e;
     out << std::endl << "---------------------------------------------" << std::endl;
     out << "collection from " << i.collection_start.t.tv_sec << "." << i.collection_start.t.tv_usec << " took " << time_interval(i.collection_start,i.collection_end) << " microsecs" << std::endl;
     out << "ext devs (" ;
     std::copy(i.ext_devs.begin(), i.ext_devs.end(), std::ostream_iterator<std::string>(out, ","));
     out << ")" << std::endl;
     out << "int devs (" ;
     std::copy(i.int_devs.begin(), i.int_devs.end(), std::ostream_iterator<std::string>(out, ","));
     out << ")" << std::endl;
     out << "classes (" ;
     std::copy(i.classes.begin(), i.classes.end(), std::ostream_iterator<std::string>(out, ","));
    
     out << ")" << std::endl;
     out << "rules (" ; 
     std::copy(i.rules.begin(), i.rules.end(), std::ostream_iterator<std::string>(out, ","));
     
     out << ")" << std::endl;

     out << "stats " << std::endl;
     
     for(e = i.stats.begin(); e != i.stats.end(); e++) 
         out << e->first << " = " << e->second << std::endl;
     
     out << "---------------------------------------------" << std::endl;
     return out;
 }

        
 
// callback from the tc code to get queue stats   
// unlike iptables this compiles OK in C++
   
extern "C" int tc_callback(struct sockaddr_nl *who, struct nlmsghdr *n, void *s) {

    // passed pointer to vector of traffic stats
    Vtraf_stat *trafstats = (Vtraf_stat *)s;
    // container for current readings
    traf_stat thisone;

    
    struct tcmsg *t = (struct tcmsg *)NLMSG_DATA(n);
    int len = n->nlmsg_len;
    struct rtattr * tb[TCA_MAX+1];


    // sanity checks
    if (n->nlmsg_type != RTM_NEWTCLASS && n->nlmsg_type != RTM_DELTCLASS) {
	thisone.error = "Not a class";
        goto END;
	
    }
    len -= NLMSG_LENGTH(sizeof(*t));
    if (len < 0) {
        thisone.error = "Wrong len " + len;
	goto END;
    }

   
    memset(tb, 0, sizeof(tb));
    parse_rtattr(tb, TCA_MAX, TCA_RTA(t), len);

    // see what we have here
    if (tb[TCA_KIND] == NULL) {
        thisone.error = "NULL kind\n";
        goto END;
    }
    if (n->nlmsg_type == RTM_DELTCLASS) { 
        thisone.error = "deleted\n";
        goto END;
    }
    if (t->tcm_handle) {
        thisone.classid = thisone.tc_classid(t->tcm_handle);
        thisone.dev = ll_index_to_name(t->tcm_ifindex);
        thisone.parentid = thisone.tc_classid(t->tcm_parent);
        thisone.info = t->tcm_info;
        // htb stuff? - hardcoded for hdb here
        if (tb[TCA_OPTIONS]) {
            struct rtattr *htb[TCA_HTB_RTAB+1];
            struct tc_htb_opt *hopt;

            memset(htb,0, sizeof(htb));
            parse_rtattr(htb,TCA_HTB_RTAB, (struct rtattr *)RTA_DATA(tb[TCA_OPTIONS]), 
                         RTA_PAYLOAD(tb[TCA_OPTIONS]));

            if (htb[TCA_HTB_PARMS]) {

                hopt = (tc_htb_opt*)RTA_DATA(htb[TCA_HTB_PARMS]);
	    
                thisone.rate = hopt->rate.rate;
                thisone.ceiling = hopt->ceil.rate;
            }
        }
        // do we have stats?
        if (tb[TCA_STATS]) {
            if (RTA_PAYLOAD(tb[TCA_STATS]) < sizeof(struct tc_stats))
                thisone.error = "statistics truncated";
            else {
                
                memcpy(&thisone.stats, 
                       RTA_DATA(tb[TCA_STATS]), sizeof(struct tc_stats));
            }
        }
    }
 END:
    // put on end of the list
    trafstats->push_back(thisone);
    return 0;
    
}

   
// return a string consisting of all classes associated with dev
Vtraf_stat list_class(std::string dev) {
    // just call the C string variant of the same
    return list_class(dev.c_str());
}
// and the C string variant...
// messy stuff with the netlink into the kernel.

Vtraf_stat list_class(const char *dev) {
    Vtraf_stat stats;
    traf_stat errorstat;

    struct tcmsg t;

    // defined in libnetlink.h pair of local socket addresses and a file descriptor
    struct rtnl_handle rth;
    
    stats.clear(); // start with empty collection of stats
    memset(&t, 0, sizeof(t));
    t.tcm_family = AF_UNSPEC;

    // can we open the netlink socket at all?
    if (rtnl_open(&rth, 0) < 0) {
        errorstat.error = "Cannot open rtnetlink";
        stats.push_back(errorstat);
        return stats;
    }

    // this requests a dump of current state and then runs ll_remember_index
    // across the returned result stream.
    // there are 16 possible slots in the static idxmap struct within libnetlink.a

    ll_init_map(&rth);
    // now we select the dev specificaly
    if ((t.tcm_ifindex = ll_name_to_index((char *)dev)) == 0) {
        // thats cool - not a problem, just no content
	
        rtnl_close(&rth);
        return stats;
    }

    if (rtnl_dump_request(&rth, RTM_GETTCLASS, &t, sizeof(t)) < 0) {
        errorstat.error = "Cant send dump request";
        stats.push_back(errorstat);
        rtnl_close(&rth);
        return(stats);
    }
    
    
    if (rtnl_dump_filter(&rth, tc_callback, (void *)&stats, NULL, NULL) < 0) {
	errorstat.error = "dump terminated";
        stats.push_back(errorstat);
        rtnl_close(&rth);
        return(stats);
    }  
    rtnl_close(&rth);
    
    // now we sort it, using implicit operator < for the object
    sort(stats.begin(),stats.end());
    // and return the current class hierachy and readings for this device.
    return stats;
}

// Iptables interface:
// general version of list_rules,
// wrapper round the C core code fetch_counts.

Vtraf_stat list_rules(std::string chain) {
 
    Vtraf_stat stats;
    struct iptable_data res[MAX_RULES] = {0};
    
    stats.clear();
    unsigned int num = fetch_counts(chain.c_str(), res, MAX_RULES);

   
    unsigned int i;
    if(!strncmp("traf-tots-",  chain.c_str(), 10)) {
        for(i = 0; i < num; i++) {
            traf_stat thisstat;

            // go from positions in the table back to meaningful
            // rule numbers (and therefore names) using info put
            // in place by trafficloader script
            thisstat.rule_num = traf_config.pos_to_rulenum(i);
        
       
            thisstat.classid = traf_config.rule_to_classid(thisstat.rule_num);
            // rule does not need to contain dev, implicit in chain name
            thisstat.dev = chain.substr(strlen("traf-tots-"));
             // iptables can only tell us bytes and packets
            // not the rich set of stats that the traffic queues do   
            thisstat.stats.bytes = res[i].bcnt;
            thisstat.stats.packets = res[i].pcnt;
            stats.push_back(thisstat);
        }
    }
    else { 
        if(num == 0) {
            // std::cerr << __LINE__ << " fetch_counts fail for " << chain << std::endl;
            return stats;
        }
        // Assume its non traffic stats (pre|post)routing-(1|2)
        for(i = 0; i < num; i++) {
            traf_stat thisstat;
            thisstat.rule_num = i;
            // must get dev from the rule, its either attached to
            // incomming or outgoing, never both
            if(strlen(res[i].iniface)) {
                thisstat.dev = res[i].iniface;
            }
            else if (strlen(res[i].outiface)) {
                thisstat.dev = res[i].outiface;
            }
            else {
                thisstat.dev = "unknown"; // should never happen in (pre|post)routing(1|2)
                // std::cerr << __LINE__ << " rule " << i << " unknown dev" << std::endl;
            } 
            // iptables can only tell us bytes and packets
            // not the rich set of stats that the traffic queues do   
            thisstat.stats.bytes = res[i].bcnt;
            thisstat.stats.packets = res[i].pcnt;
            stats.push_back(thisstat);
            
        }
        
    }
    
    return stats;
}
                
 
// optomised for traffic stats - provide just a device so assume
// want to see traffic rules for that device
          
Vtraf_stat list_rules_for_dev(std::string dev) {
    return list_rules_for_dev(dev.c_str());
}

// which means mangle table traf-tots-? chain
Vtraf_stat list_rules_for_dev(const char *dev) {

    return list_rules(std::string("traf-tots-") + dev);
}


// takes a difference between timestampss as double floating number of microsecs
// 1 million microsecs in a second
// if the old or new times are not valid then interval is 0
// 

double time_interval( const timestamp &oldtime, const timestamp &newtime) {
    double res = 0.0;
    if(oldtime.t.tv_sec == 0 || newtime.t.tv_sec == 0)
        return res; // can't do difference against start of time

    if(newtime.t.tv_sec > oldtime.t.tv_sec)
        res = (newtime.t.tv_sec-oldtime.t.tv_sec)*1000000;
    if(newtime.t.tv_usec < oldtime.t.tv_usec) {
        // have wrapped, so rest of old part plus new part
        res += (1000000 - oldtime.t.tv_usec) + newtime.t.tv_usec;
    }
    else 
        res += (newtime.t.tv_usec - oldtime.t.tv_usec);
    return res;
}

// take an older and a newer traf_stat and 
// calculate rates, putting results in refs
void calculate_rates(const traf_stat & older, const traf_stat & newer,
                     double &bytes_per_sec, 
                     double &bits_per_sec, 
                     double &pkts_per_sec) {
    double interval = time_interval(older.tstamp,newer.tstamp);
    if(interval > 0.0 && newer.stats.bytes > older.stats.bytes) {
        bytes_per_sec = (double)(newer.stats.bytes - older.stats.bytes) /
            (interval/1000000.0);
        bits_per_sec = bytes_per_sec * 8; 
        pkts_per_sec = (double)(newer.stats.packets - older.stats.packets) /
            (interval/1000000.0);
       
    }
    else 
        bytes_per_sec = bits_per_sec = pkts_per_sec = 0.0;
    
}

// calculate rates for a pair of samples 
// updating the rates IN the newer sample
void calculate_rates(traf_stat & older, traf_stat & newer) {
    calculate_rates(const_cast<const traf_stat &>(older),
                    const_cast<const traf_stat &>(newer), 
                    newer.bytes_per_sec,newer.bits_per_sec,newer.pkts_per_sec);
}

// run calculate_rates over a whole hash of stats

void calculate_rates(traf_stat_hash *oldh, traf_stat_hash *newh) {
    traf_stat_hash_iterator o,n;
    
    
    // go through the indexes in no particular order
    for(o = oldh->begin();
        o != oldh->end();
        o++) {
        if(newh->count(o->first) > 0) {
            // only if this is present in both old and new
            calculate_rates(o->second, (*newh)[o->first]);
        }
        else {
            // std::cout << "Cant find " << o->first << std::endl;
        }
    }
}

// call the individual calculate_rates for all our various sample sets
// this takes the calculate_rates method right up to whole 
// traf_stat_collection_items
void calculate_rates(traf_stat_collection_item & older,
                     traf_stat_collection_item & newer) {
    calculate_rates(&older.stats, &newer.stats);    
}

// collect a single sample - true if valid data in it
bool collect_a_sample(Vtraf_stat_samples &s) {
    traf_stat_collection_item thisone;
    s.push_back(thisone);
    return thisone.ext_devs.size() > 0;
}

// collect some samples, add to the end of what we have 
int collect_samples(int number, int interval, Vtraf_stat_samples &s) {
    int i;
    
    if(interval <= 0 || number < 1|| traffic_iptables_missing())
        return 0; // nothing to do, have to have at least 2 samples
    for(i = 0; i < number; i++) {
        collect_a_sample(s);
        usleep(interval);
    }
    return number;
}
 

// do averages over time
// find timespan we are interested in and 
double average_bit_rate(Vtraf_stat_samples &samples, const std::string &idx,  const timestamp &start, const  timestamp &end) { 
   
    // find the first sample that is > than start and the last sample that is < end
    // have to do own bsearch here as generic one needs a whole sample
    // object and dont want to do that
    unsigned int middle,min,max,s,e;
    
    // 1st find the lowest sample > than start that has something for idx
    for(min=0, max = (samples.size()-1), middle = (min+max)/2; 
        min < max;
        middle = (min+max)/2) {
  
        if(samples[middle].collection_start.t.tv_sec > start.t.tv_sec) {
            // this sample newer than we want
            max = middle; // so look lower
        }
        else if(samples[middle].collection_start.t.tv_sec < start.t.tv_sec) {
            // this sample is older than we want
            min = middle+1; // so look higher
        }
        else {
            // we have found one with the second we are looking for
            break; // so can do early exit
        }
            
    }
    
    // bsearch could have landed us anywhere in our range
    // adjust down to give the first of the matching records

    while(middle > 0 && 
          samples[middle-1].collection_start.t.tv_sec >= start.t.tv_sec)
        middle--;
    // middle is the nearest thing to start time but does it have content for idx?
    while(samples[middle].stats.count(idx) == 0 && samples[middle].collection_start.t.tv_sec < end.t.tv_sec)
        middle++;

    s = middle;

    // start from there to find end time
    
    for(min=s, max = (samples.size()-1), middle = (min+max)/2; 
        min < max;
        middle = (min+max)/2) {
    
        if(samples[middle].collection_start.t.tv_sec > end.t.tv_sec) {
            // sample newer than we want
            max = middle;
        }
        else if(samples[middle].collection_start.t.tv_sec < end.t.tv_sec) {
            // this sample is older than we want
            min = middle+1;
        }
        else {
            // we have found one with the second we are looking for
            break;
        }
        
    } 
    while(middle < (samples.size()-1) && samples[middle+1].collection_start.t.tv_sec <= end.t.tv_sec)
        middle++; // get the last possible matching value
    // middle is the nearest thing to end time but does it have content for idx?
    while(samples[middle].stats.count(idx) == 0 && samples[middle].collection_start.t.tv_sec >= start.t.tv_sec)
        middle--;
    e = middle;

    double byterate,bitrate,pktrate;
    unsigned start_sample_count = samples[s].stats.count(idx);
    unsigned end_sample_count = samples[e].stats.count(idx);
    if(s < e && start_sample_count > 0 && end_sample_count > 0) {
        // this should be true
        // so can get a rate over the widest possible range
        // data for this label may well be missing in some (or most)
        // of the data but the trouble we have been through above
        // means that we have the farthest possible apart pair of samples
        // to calculate the rate from 
        const traf_stat &start = samples[s].stats[idx];
        const traf_stat &end = samples[e].stats[idx];
        byterate = bitrate = pktrate = 0;
        calculate_rates(start, end, byterate, bitrate, pktrate);
       
        return bitrate;
    }
    else
        return 0.0; // no samples so no average
}
    

// remove samples older than certain time
// As vector is in time order just need to start from the beginning
// only care about seconds not microsec
void truncate_sample_set(Vtraf_stat_samples &samples, const timestamp &oldest) {
    Vtraf_stat_samples_iterator i = samples.begin();
    if(i == samples.end())
        return; // empty set anyway
    while(i != samples.end() && i->collection_start.t.tv_sec < oldest.t.tv_sec) 
        i++;
    // iterator now set at first item new enough to keep (or end)
    
    // never remove all the samples!
    // have to have a previous one to calculate rates from
    // even if line has otherwise been inactive for a 'long' time
    if(i == samples.end()) 
        i--; // back onto last item
    if(i != samples.begin()) {
        // erase to i-1 so always keep at least one
        samples.erase(samples.begin(), i-1);
        
    }
}

// nice user firendly rate - 2 decimal places kbit mbit or gbit
// Then followed by same as bytes
std::string format_rate(double t1, double t2) {
    return format_rate(t1) + " " + format_rate(t2);
} 
std::string format_rate(double tmp) {
    std::ostringstream out;
    char floatbuf[20];
    
    if(rint(tmp) != 0) {
        if(tmp >= 1024 * 1024 * 1023) {
            sprintf(floatbuf, "%*.2f", 7,tmp/(1024*1024*1024));
            out << floatbuf << " Gbit"; 
        }
        else if(tmp >= 1024 * 1023) {
            sprintf(floatbuf, "%*.2f", 7,tmp/(1024*1024));
            out << floatbuf << " Mbit"; 
        }
        else {
            sprintf(floatbuf, "%*.2f", 7,tmp/(1024));
            out << floatbuf << " Kbit";
        }
    

        tmp /= 8;
        out << ' ';
        if(rint(tmp) != 0) {
            if(tmp >= 1024 * 1024 * 1023) {
                sprintf(floatbuf, "%*.2f", 7,tmp/(1024*1024*1024));
                out << floatbuf << " GB"; 
            }
            else if(tmp >= 1024 * 1023) {
                sprintf(floatbuf, "%*.2f", 7,tmp/(1024*1024));
                out << floatbuf << " MB"; 
            }
            else {
                sprintf(floatbuf, "%*.2f", 7,tmp/(1024));
                out << floatbuf << " KB";
            }
        }
        else {
            sprintf(floatbuf, "%*.2f", 7,0.0);
            out << floatbuf<< " KB";
        }
    }
    else {
        sprintf(floatbuf, "%*.2f", 7,0.0); 
        out << floatbuf << " Kbit";
        out << ' ';
        sprintf(floatbuf, "%*.2f", 7,0.0);
        out << floatbuf << " KB";
    }
    return out.str();
}       
