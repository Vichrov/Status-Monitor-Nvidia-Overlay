// Host check of the production NvidiaOverlay, using STB metrics/rasterization.
// The Switch services are fixtures; this does not emulate Horizon or Ultrahand.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <sys/stat.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "../lib/libtesla/include/stb_truetype.h"
using u16=uint16_t; using u32=uint32_t;
struct HidTouchState {}; struct JoystickPosition {};
int mutex_Misc=0,mutex_BatteryChecker=0;
void mutexInit(int*) {} void mutexLock(int*) {} void mutexUnlock(int*) {}
bool FullMode=true,deactivateOriginalFooter=false,GameRunning=true,useOldFPSavg=false;
int alphabackground=0,TeslaFPS=5;
uint64_t systemtickfrequency=19200000,idletick0=1000000,idletick1=2000000,idletick2=1000000,idletick3=500000;
int nvCheck=0,i2cCheck=0,Hinted=0,pwmCheck=0,psmCheck=0;
#define R_SUCCEEDED(x) ((x)==0)
float FPSavg=59.9f,FPSavg_old=59.8f,SOC_temperatureF=61,PCB_temperatureF=43,Rotation_Duty=34,PowerConsumption=-6.8f;
void* NxFps=reinterpret_cast<void*>(1);
unsigned GPU_Load_u=925,GPU_Hz=768000000,CPU_Hz=1785000000,RAM_Hz=1600000000,realGPU_Hz=0,realCPU_Hz=0,realRAM_Hz=0;
uint64_t RAM_Used_application_u=2500000000,RAM_Used_applet_u=100000000,RAM_Used_system_u=500000000,RAM_Used_systemunsafe_u=100000000;
uint64_t RAM_Total_application_u=3200000000,RAM_Total_applet_u=200000000,RAM_Total_system_u=600000000,RAM_Total_systemunsafe_u=100000000;
std::string keyCombo="ZL+ZR",filename="test.ovl",folderpath="/";
uint64_t MapButtons(const std::string&) {return 3;} bool isKeyComboPressed(uint64_t h,uint64_t,uint64_t b){return h==b;}
void StartThreads(void*) {} void CloseThreads() {} void svcSleepThread(uint64_t) {}
uint64_t svcGetSystemTick(){return 1000000000;} uint64_t armTicksToNs(uint64_t n){return n;}
namespace tsl {
namespace gfx {
struct Color {unsigned r,g,b,alpha;Color(unsigned raw):r(raw&15),g((raw>>4)&15),b((raw>>8)&15),alpha((raw>>12)&15){} Color(int r,int g,int b,int a):r(r),g(g),b(b),alpha(a){} };
class Renderer {
 std::vector<unsigned char> regular,bold; stbtt_fontinfo rf{},bf{};
public:
 int fw=1280,fh=80; std::vector<unsigned char> pixels;
 std::vector<std::string> labels;
 Renderer(){
  auto load=[](const char* p,std::vector<unsigned char>& data,stbtt_fontinfo& f){FILE* h=fopen(p,"rb");assert(h);fseek(h,0,SEEK_END);data.resize(ftell(h));rewind(h);assert(fread(data.data(),1,data.size(),h)==data.size());fclose(h);assert(stbtt_InitFont(&f,data.data(),0));};
  load("config/status-monitor/fonts/regular.ttf",regular,rf);load("config/status-monitor/fonts/bold.ttf",bold,bf);
 }
 static Renderer& getRenderer(){static Renderer r;return r;}
 Color a(Color c){return c;} void setLayerPos(int,int){}
 void reset(int w,int h){fw=w;fh=h;pixels.assign(w*h*3,30);labels.clear();}
 void pixel(int x,int y,Color c,float coverage=1){if(x<0||y<0||x>=fw||y>=fh)return;float a=c.alpha/15.f*coverage;unsigned rgb[]={c.r,c.g,c.b};for(int i=0;i<3;++i)pixels[(y*fw+x)*3+i]=unsigned(pixels[(y*fw+x)*3+i]*(1-a)+rgb[i]*17*a);}
 void drawRect(int x,int y,int w,int h,Color c){assert(x>=0&&y>=0&&x+w<=fw&&y+h<=fh);for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)pixel(xx,yy,c);}
 std::pair<unsigned,unsigned> drawString(const char* str,bool,int x,int y,float size,Color c,bool weight=false){
  auto* f=weight?&bf:&rf;float scale=stbtt_ScaleForPixelHeight(f,size);int start=x,prev=0;
  if(c.alpha)labels.emplace_back(str);
  const unsigned char* s=(const unsigned char*)str;
  while(*s){int cp=*s++;if(cp>=192){int count=(cp<224?1:cp<240?2:3);cp&=(1<<(6-count))-1;while(count--)cp=(cp<<6)|(*s++&63);}
   x+=int(scale*stbtt_GetCodepointKernAdvance(f,prev,cp));int adv,lsb;stbtt_GetCodepointHMetrics(f,cp,&adv,&lsb);
   if(c.alpha){int w,h,xo,yo;unsigned char* bitmap=stbtt_GetCodepointBitmap(f,scale,scale,cp,&w,&h,&xo,&yo);assert(x+xo>=0&&y+yo>=0&&x+xo+w<=fw&&y+yo+h<=fh);for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)pixel(x+xo+xx,y+yo+yy,c,bitmap[yy*w+xx]/255.f);stbtt_FreeBitmap(bitmap,nullptr);}
   x+=int(adv*scale); // Tesla truncates each glyph advance.
  }
  return {unsigned(x-start),unsigned(size)};
 }
 void save(const char* path){FILE* f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",fw,fh);fwrite(pixels.data(),1,pixels.size(),f);fclose(f);}
};
}
namespace hlp {void requestForeground(bool){}}
namespace elm {
struct Element {virtual ~Element(){};virtual void render()=0;};
struct CustomDrawer:Element {std::function<void(gfx::Renderer*,u16,u16,u16,u16)> fn;CustomDrawer(decltype(fn) f):fn(f){}void render()override{fn(&gfx::Renderer::getRenderer(),0,0,0,0);}};
struct OverlayFrame:Element{Element* child=nullptr;OverlayFrame(const char*,const char*){}~OverlayFrame(){delete child;}void setContent(Element* e){child=e;}void render()override{child->render();}};
}
struct Gui {virtual ~Gui(){};virtual elm::Element* createUI()=0;virtual void update(){};virtual bool handleInput(uint64_t,uint64_t,const HidTouchState&,JoystickPosition,JoystickPosition){return false;}};
struct Overlay {static Overlay* get(){static Overlay o;return &o;}void close(){}};
void setNextOverlay(const std::string&,const std::string&){}
}
#include "../source/modes/Nvidia.hpp"
int main(){
 auto& r=tsl::gfx::Renderer::getRenderer();int count=0;
 for(int ru=0;ru<2;++ru)for(int mode=0;mode<4;++mode)for(int corner=0;corner<4;++corner)for(int size=16;size<=28;size+=2)for(int available=0;available<2;++available){
  nvstyle::settings.russian=ru;nvstyle::settings.corner=corner;nvstyle::settings.size=size;nvstyle::launchMode=mode;
  GameRunning=available;nvCheck=i2cCheck=pwmCheck=psmCheck=Hinted=available?0:1;
  r.reset(mode==3?448:1280,mode==3?720:80);
  NvidiaOverlay overlay;auto* ui=overlay.createUI();ui->render();delete ui;
  for(const auto& label:r.labels){assert(label.find("LAT")==std::string::npos);assert(label.find("Reflex")==std::string::npos);}
  if(ru==1&&corner==1&&size==20&&available){std::string p="/tmp/nvidia-mode-"+std::to_string(mode)+".ppm";r.save(p.c_str());}
  ++count;
 }
 printf("PASS: %d layouts, RU/EN, four modes/corners, seven sizes, present/missing services.\n",count);
}
