//===========================================
//  PC-BSD source code
//  Copyright (c) 2015, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "sysadm-network.h"
//PLEASE: Keep the functions in the same order as listed in pcbsd-network.h
#include "sysadm-general.h"

using namespace sysadm;
//=====================
//   NETWORK FUNCTIONS
//=====================
/*QList<NetworkEntry> Network::listNetworkEntries(){
  QList<NetworkEntry> out;
  netent *entry = getnetent();
  while(entry!=0){
    //Copy over this data into the output structure
    NetworkEntry tmp;
      tmp.name = QString::fromLocal8Bit(entry->n_name);
      for(int i=0; entry->n_aliases[i] != 0; i++){
        tmp.aliases << QString::fromLocal8Bit(entry->n_aliases[i]);
      }
      tmp.netnum = entry->n_net;
    out << tmp;
    //Now load the next entry
    entry = getnetent();
  }
  endnetent(); //make sure to close the file since we are finished reading it
  return out;
}*/

//---------------------------------------
QStringList Network::readRcConf(){
  static QStringList contents = QStringList();
  static QDateTime lastread;
  if(!lastread.isValid() || contents.isEmpty() || (QFileInfo("/etc/rc.conf").lastModified()> lastread) ){
    lastread = QDateTime::currentDateTime();
    contents = General::readTextFile("/etc/rc.conf");
  }
  return contents;
}

//---------------------------------------
NetDevSettings Network::deviceRCSettings(QString dev){
  QStringList info = Network::readRcConf().filter(dev);
  //Setup the default structure/values
  NetDevSettings set;
  set.device = dev;
  set.wifihost = set.useDHCP = set.wifisecurity = false;
  if(info.isEmpty()){ return set; } //no settings
  //Now load any info associated with this device
  for(int i=0; i<info.length(); i++){
    QString var = info[i].section("=",0,0).simplified();
    QString val = info[i].section("=",1,100).simplified();
      if(val.startsWith("\"")){ val = val.remove(1); }
      if(val.endsWith("\"")){ val.chop(1); }
      val.prepend(" "); val.append(" "); //just to make additional parsing easier later - each "variable" should have whitespace on both sides
    if( val.simplified()==dev && (var.startsWith("vlans_") || var.startsWith("wlans_") )){
      set.asDevice = var.section("_",1,-1);
      if(var.startsWith("wlans_")){ set.wifihost = true; }
    }else if(var==("ifconfig_"+dev)){
      QStringList vals = val.split(" ",QString::SkipEmptyParts);
      //This is the main settings line: lots of things to look for:
      if(!val.contains("DHCP")){
        //Look for the static networking values
	set.useDHCP = false;
	if(val.contains(" netmask ")){ set.staticNetmask = val.section(" netmask ",1,1).section(" ",0,0); }
	if(val.contains(" gateway ")){ set.staticGateway = val.section(" gateway ",1,1).section(" ",0,0); }
	if(val.contains(" inet ")){ set.staticIPv4 = val.section(" inet ",1,1).section(" ",0,0); }
	if(val.contains(" inet6 ")){ set.staticIPv6 = val.section(" inet6 ",1,1).section(" ",0,0); }
	/*if(set.staticIPv4.isEmpty() || set.staticIPv6.isEmpty()){
	  //IPv4/6 address can sometimes not have the "inet(6)" identifier - look through the first few values as well
	  QStringList vals = val.split(" ",QString::SkipEmptyParts);
	  for(int v=0; v<vals.length(); v++){

	  }
	}*/
      }else{ set.useDHCP=true; } //end of DHCP check
      //Wifi Checks
      if(vals.contains("WPA")){ set.wifisecurity=true; }

    } //end variable checks
  } //end loop over rc.conf lines
  return set;
}

//---------------------------------------
/*NetDevSettings Network::deviceIfconfigSettings(QString dev){
  QString info = General::RunCommand("ifconfig "+dev);
  NetDevSettings set;
    if(info.isEmpty() || info.contains("interface "+dev+"does not exist")){ return set; } //empty stucture
  //Now parse all the available info from ifconfig
  info = info.replace("\t"," ").replace("\n"," ").simplified(); //ensure that whitespace is used for parsing
  //if(info.contains("inet ")){ set.staticIPv4 = info.section("inet ",1,1).section(" ",0,0); }
  //if(info.contains("inet6 ")){ set.staticIPv6 = info.section("inet6 ",1,1).section("%",0,0); }
  set.device = dev;
  if(info.contains(" ether ")){ set.etherMac = info.section(" ether ",1,1).section(" ",0,0); }
  if(info.contains(" ssid ")){ set.wifiSSID = info.section(" ssid ",1,1).section(" ",0,0); }
  if(info.contains(" bssid ")){ set.wifiBSSID = info.section(" bssid ",1,1).section(" ",0,0); }
  if(info.contains(" country ")){ set.wifiCountry = info.section(" country ",1,1).section(" ",0,0); }
  if(info.contains(" channel ")){ set.wifiChannel = info.section(" channel ",1,1).section(" ",0,0); }
  return set;
}*/

//=====================
//   NETWORK-ROOT FUNCTIONS
//=====================
/*bool NetworkRoot::saveNetworkEntry(NetworkEntry data){
  netent *entry = getnetbyname(data.name.toLocal8Bit());
  if(entry==0){
    //This entry does not exist yet - need to add it
    return false; //not implemented yet - add it to /etc/networks?
  }else{
    //This entry already exists, update it
    endnetent(); //Make sure to close the file when finished
    return false; //not implemented yet
  }
}*/

//--------------------------------------
bool NetworkRoot::saveRCSettings(NetDevSettings set){
  if(!QFile::exists("/dev/"+set.device)){ return false; } //invalid device
  //Create the lines which need to be put info /etc/rc.conf based on the settings
  QStringList tags, lines, found;
  //Related device lines
  if(!set.asDevice.isEmpty()){
    if(set.wifihost){ tags << "wlans_"+set.asDevice; lines << "wlans_"+set.asDevice+"=\""+set.device+"\""; }
    else{ tags << "vlans_"+set.asDevice; lines << "vlans_"+set.asDevice+"=\""+set.device+"\""; }
  }
  //Main device line
  QString tmp = "ifconfig_"+set.device+"=\"";
  if(set.useDHCP){ tmp.append("DHCP"); }
  else{
    if(!set.staticIPv4.isEmpty()){ tmp.append("inet "+set.staticIPv4+" "); }
    if(!set.staticIPv6.isEmpty()){ tmp.append("inet6 "+set.staticIPv6+" "); }
    if(!set.staticNetmask.isEmpty()){ tmp.append("netmask "+set.staticNetmask+" "); }
    if(!set.staticGateway.isEmpty()){ tmp.append("gateway "+set.staticGateway+" "); }
    tmp = tmp.simplified(); //remove any excess whitespace on end
  }
  QString var = "ifconfig_"+set.device;
    tags << var;
  if(!tmp.isEmpty()){ lines << var+"=\""+tmp+"\""; }
  else{ lines << ""; } //delete the line

  //Now read rc.conf and adjust contents as needed
  QStringList info = Network::readRcConf();
  for(int i=0; i<info.length(); i++){
    if(info[i].simplified().isEmpty()){ continue; }
    QString var = info[i].section("=",0,0).simplified();
    if( tags.contains(var) ){
      int index = tags.indexOf(var);
      info[i] = lines[index];
      lines.removeAt(index);
      found << tags.takeAt(index);
    }else if(found.contains(var)){
      //Duplicate line - remove this since it was already handled
      info.removeAt(i); i--;
    }
  }
  //Now add any lines which were not already found to the end of the file
  for(int i=0; i<lines.length(); i++){
    info << lines[i];
  }
  return General::writeTextFile("/etc/rc.conf", info, true);
}

//--------------------------------------
bool NetworkRoot::setIfconfigSettings(NetDevSettings){
  return false;
}
