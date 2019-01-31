#include<bits/stdc++.h>
#include<unistd.h>
#include "../../bdlaunch/launch/mods.h"
#include "../../BedrockMod/include/base.h"
#include "../fklib/lib.hpp"
       #include <sys/stat.h>
       #include <sys/types.h>
#include <minecraft/command/Command.h>
#include <minecraft/command/CommandMessage.h>
#include <minecraft/command/CommandOutput.h>
#include <minecraft/command/CommandParameterData.h>
#include <minecraft/command/CommandRegistry.h>
#include <minecraft/command/CommandVersion.h>
#include<fcntl.h>
extern "C"{
	void mod_init();
};
struct land{
  int x,y,lx,ly;
  std::string owner;
};
std::string land_string(land& a){
  char buf[512];
  sprintf(buf,"%d:%d:%d:%d:%s",a.x,a.y,a.lx,a.ly,a.owner.c_str());
  return std::string(buf);
}
land to_land(std::string& dat){
  char buf[512];
  land a;
  sscanf(dat.c_str(),"%d:%d:%d:%d:%s",&a.x,&a.y,&a.lx,&a.ly,buf);
  a.owner=std::string(buf);
  return a;
}
static std::forward_list<land> list;
bool checkland(land& a,int x,int z){
  int t1=x-a.x;
  int t2=z-a.y;
  if(t1<a.lx || t2<a.ly) return true;
  return false;
}
std::pair<bool,std::string> checkperm(int x,int z,std::string& name){
  for(auto a:list){
    if(checkland(a,x,z) && a.owner!=name){
      return std::make_pair(false,a.owner);
    }
  }
  return std::make_pair(true,"");
}

void load(){
  int fd=open("land/land.txt",O_CREAT,S_IRWXU);
  close(fd);
  std::ifstream banf("land/land.txt");
  filetolist<std::forward_list<land>,land >(list,banf,to_land);
}
void save(){
  std::ofstream fk("land/land.txt");
  listtofile<std::forward_list<land>,land >(list,fk,land_string);
}
Player* (*FindPlayer)(std::string* pp);
void (*sendMessage)(Player* player, std::string* msg);
std::set<std::string> g_start,g_end;
struct Vc2{
  int x,z;
};
std::unordered_map<std::string,Vc2> pStart,pEnd;
bool buyLand(Player* pp,Vc2 st,Vc2 ed){
  land tmp;
  tmp.owner=*pp->getPlayerName();
  tmp.x=st.x;
  tmp.y=st.z;
  tmp.lx=ed.x-st.x+1;
  tmp.ly=ed.z-st.z+1;
  list.push_front(tmp);
  save();
  return true;
}
struct LandCmd : Command
{
  CommandMessage op;
    ~LandCmd() override = default;
    static void setup(CommandRegistry &registry)
    {
        registry.registerCommand("land", "land Command", (CommandPermissionLevel)0, (CommandFlag)16, (CommandFlag)32);
        registry.registerOverload<LandCmd>("land", CommandVersion(1, INT_MAX),CommandParameterData(CommandMessage::type_id(), &CommandRegistry::parse<CommandMessage>, "oper", (CommandParameterDataType)0, nullptr, offsetof(LandCmd, op), false, -1));
    }
    void execute(CommandOrigin const &origin, CommandOutput &outp) override
    {
        if(origin.getOriginType()==0){
          std::string sb=op.getMessage(origin);
          Player* pp=(Player*)origin.getEntity();
            if(sb=="start"){
              g_start.insert(*pp->getPlayerName());
            }
            if(sb=="end"){
              g_start.erase(*pp->getPlayerName());
              g_end.insert(*pp->getPlayerName());
            }
            if(sb=="buy"){
              g_end.erase(*pp->getPlayerName());
              if(!buyLand(pp,pStart[*pp->getPlayerName()],pEnd[*pp->getPlayerName()])){
                outp.addMessage("Failed buying land");
              }else{
                outp.addMessage("Successed buying land");
              }
          }
          outp.addMessage(op.getMessage(origin));
          outp.success();
        }
    }
};

void (*addCmdhook)(fn_6 hook);
static void cmdSetup(CommandRegistry* a){
  printf("[Land] loaded\n");
  LandCmd::setup(*a);
}

fn_666 useblock_orig;
fn_66666 destroy_orig;
fn_666666 place_orig;
bool permHelper(Player* pp,BlockPos* bp){
  auto res=checkperm(bp->x,bp->z,*pp->getPlayerName());
  if(res.first==false){
    char buf[256];
    //printf("nmsl %s\n",res.second.c_str());
    sprintf(buf,"it's %s's land!!!",res.second.c_str());
    std::string bufx(buf);
    sendMessage(pp,&bufx);
    return false;
  }
  return true;
}
u64 useblock(u64 thi,Player* pp,BlockPos* bp){
  if(!permHelper(pp,bp)) return 0;
  if(g_start.count(*pp->getPlayerName())){
    pStart[*pp->getPlayerName()]=(Vc2){bp->x,bp->z};
    return 0;
  }
  if(g_end.count(*pp->getPlayerName())){
    pEnd[*pp->getPlayerName()]=(Vc2){bp->x,bp->z};
    return 0;
  }
  return useblock_orig(thi,pp,bp);
}
u64 destroy(u64 thi,Actor* ppa,BlockPos* bp,ItemInstance* it,u64 bl){
  if(ppa->getEntityTypeId()==1 && !permHelper((Player*)ppa,bp)){
    return 0;
  }
  else
  return destroy_orig(thi,ppa,bp,it,bl);
}
u64 place(u64 thi,Actor* ppa,BlockPos* bp,u64 unk,ItemInstance* it,u64 bl){
  if(ppa->getEntityTypeId()==1 && !permHelper((Player*)ppa,bp)){
    return 0;
  }
  else
  return place_orig(thi,ppa,bp,unk,it,bl);
}
void mod_init(){
  addCmdhook=getFuncEx("addCmdhook");
  FindPlayer=getFuncEx("FindPlayer");
  sendMessage=getFuncEx("sendMessage");
  addCmdhook(cmdSetup);
  mkdir("land",S_IRWXU);
  load();
  MCHok("_ZNK5Block3useER6PlayerRK8BlockPos",useblock,useblock_orig);
  MCHok("_ZN11BlockSource28checkBlockDestroyPermissionsER5ActorRK8BlockPosRK12ItemInstanceb",destroy,destroy_orig);
  MCHok("_ZN11BlockSource21checkBlockPermissionsER5ActorRK8BlockPosaRK12ItemInstanceb",place,place_orig);
}
