/**********************************************************************
 * DWM STATUS by Antonios Tsolis
 **********************************************************************/
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>              //errno definitions
#include <ctype.h>              //toupper
#include <time.h>               //tm, time, localtime, strftime
#include <unistd.h>             //sleep, close
#include <X11/Xlib.h>           //XOpenDisplay, XStoreName, XSync
#include <X11/XKBlib.h>
#include <arpa/inet.h>          //inet_addr
#include <sys/types.h>          //getnameinfo, gai_strerror
#include <sys/select.h>         //select
#include <sys/socket.h>         //socket, getsockname, getnameinfo
#include <netinet/in.h>         //sockaddr_in
#include <netlink/netlink.h>    //lots of netlink functions
#include <netlink/genl/genl.h>  //genl_connect, genlmsg_put
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  //genl_ctrl_resolve
#include <linux/nl80211.h>      //NL80211 definitions
#include <netdb.h>              //getnameinfo

#ifndef   NI_MAXHOST
 #define   NI_MAXHOST 1025
#endif

#define BAR_HEIGHT 15
#define CPUTEMP_FILE "/sys/class/hwmon/hwmon1/temp1_input"
#define LOADAVG_FILE "/proc/loadavg"
#define MEMINFO_FILE "/proc/meminfo"
#define INTERNET_TESTSERVER "8.8.4.4" //google dns server
#define TESTSERVER_PORT 53 //dns port

/* COLORS AND SPACE */
#define RED "^c#FF0000^"
#define WHITE "^c#FFFFFF^"
#define GREEN "^c#00FF00^"
#define YELLOW "^c#FFFF00^"
#define ORANGE "^c#FFA500^"
#define DARKRED "^c#550000^"
#define DARKGREEN "^c#005500^"
#define DARKYELLOW "^c#555500^"
#define GREENYELLOW "^c#ADFF2F^"
#define SPACE "^f10^"

typedef enum { false, true } bool;

static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
  [NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
  [NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
  [NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
  [NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
  [NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
  [NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
  [NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
  [NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
  [NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
  [NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
 };

static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
  [NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
  [NL80211_RATE_INFO_MCS] = { .type = NLA_U8 },
  [NL80211_RATE_INFO_40_MHZ_WIDTH] = { .type = NLA_FLAG },
  [NL80211_RATE_INFO_SHORT_GI] = { .type = NLA_FLAG },
 };

char buf[20];
Display *dpy;
char* status = NULL;
char* cpustatus = NULL;
char* memstatus = NULL;
char* datestatus = NULL;
char* keysstatus = NULL;
char* wifistatus = NULL;
struct nl_sock* nlsocket = NULL;
struct nl_msg* msg1 = NULL;
struct nl_msg* msg2 = NULL;
struct nl_cb* cb1 = NULL;
struct nl_cb* cb2 = NULL;
int initnl = 0;
int result1 = 0;
int result2 = 0;
int nl80211_id = -1;
int wifi_index = -1;
int wifi_bitrate = 0;
int wifi_signal = 0;
char wifi_iface[20] = "--";
bool XKB_SUPPORTED = false;

/* *******************************************************************
 * FUNCTIONS
 ******************************************************************* */
static void cleaning() { 
  free(keysstatus);
  free(datestatus);
  free(cpustatus);
  free(memstatus);
  free(wifistatus);
  free(status);
  nl_cb_put(cb1);
  nl_cb_put(cb2);
  nlmsg_free(msg1);
  nlmsg_free(msg2);
  nl_close(nlsocket);
  nl_socket_free(nlsocket);
  return;
}

static void strupr(char* p) { for ( ; *p; ++p) *p = toupper(*p); }

static void strcpy_uptodelim(char* dest, const char* source, const char delimeters[]) {
    int n = strlen(delimeters);
    while ((*dest++ = *source++)) 
      for (int i = 0; i < n; i++) 
        if (*source == delimeters[i]) { *dest = '\0'; return; }
}

static int finish_handler(struct nl_msg *msg, void *arg) {
  int *ret = arg;
  *ret = 0;
  //nl_msg_dump(msg, stdout);
  return NL_SKIP;
}

static char* vBar(int percent, int w, int h, char* fg_color, char* bg_color) 
{
  char *value;
  if((value = (char*) malloc(sizeof(char)*128)) == NULL)
    {
      fprintf(stderr, "Cannot allocate memory for buf.\n");
      exit(1);
    }
  char* format = "^c%s^^r0,%d,%d,%d^^c%s^^r0,%d,%d,%d^";

  int bar_height = (percent*h)/100;
  int y = (BAR_HEIGHT - h)/2;
  snprintf(value, 128, format, bg_color, y, w, h, fg_color, y + h-bar_height, w, bar_height);
  return value; 
}

static char* hBar(int percent, int w, int h, char* fg_color, char* bg_color) 
{
  char *value;
  if((value = (char*) malloc(sizeof(char)*128)) == NULL)
    {
      fprintf(stderr, "Cannot allocate memory for buf.\n");
      exit(1);
    }
  char* format = "^c%s^^r0,%d,%d,%d^^c%s^^r0,%d,%d,%d^";

  int bar_width = (percent*w)/100;
  int y = (BAR_HEIGHT - h)/2;
  snprintf(value, 128, format, bg_color, y, w, h, fg_color, y, bar_width, h);
  return value; 
}

static int hBar2(char *string, size_t size, int percent, int w, int h, char *fg_color, char *bg_color)
{
  char *format = "^c%s^^r0,%d,%d,%d^^c%s^^r%d,%d,%d,%d^";
  int bar_width = (percent*w)/100;

  int y = (BAR_HEIGHT - h)/2;
  return snprintf(string, size, format, fg_color, y, bar_width, h, bg_color, bar_width, y, w - bar_width, h);
}

static int hBarBordered(char *string, size_t size, int percent, int w, int h, char *fg_color, char *bg_color, char *border_color)
{
 char tmp[128];
 hBar2(tmp, 128, percent, w - 2, h -2, fg_color, bg_color);
 int y = (BAR_HEIGHT - h)/2;
 char *format = "^c%s^^r0,%d,%d,%d^^f1^%s";
 return snprintf(string, size, format, border_color, y, w, h, tmp);
}

static void setStatus(Display *dpy, char *str) 
{
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

static void percentColorGeneric(char* string, int percent, int invert)
{
 char *format = "#%X0%X000";
 int a = (percent*15)/100;
 int b = 15 - a;
  if(!invert) {
    snprintf(string, 8, format, b, a);
  }
  else {
    snprintf(string, 8, format, a, b);
  }
}

static void percentColor(char* string, int percent) {
  percentColorGeneric(string, percent, 0);
}

static int getWifiIface_callback(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL); 
    if (tb_msg[NL80211_ATTR_IFNAME]) { strcpy(wifi_iface, nla_get_string(tb_msg[NL80211_ATTR_IFNAME])); }
    if (tb_msg[NL80211_ATTR_IFINDEX]) { wifi_index = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]); }
    strupr(wifi_iface);
    return NL_SKIP;
}

static int getWifiInfo_callback(struct nl_msg *msg, void *arg)
{
 struct nlattr *tb[NL80211_ATTR_MAX + 1];
 struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
 struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
 struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
 //nl_msg_dump(msg, stdout);
 nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
 /*
  * TODO: validate the interface and mac address!
  * Otherwise, there's a race condition as soon as
  * the kernel starts sending station notifications.
  */
 if (!tb[NL80211_ATTR_STA_INFO])
   { fprintf(stderr, "sta stats missing!"); return NL_SKIP; }
 if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX, tb[NL80211_ATTR_STA_INFO], stats_policy))
   { fprintf(stderr, "failed to parse nested attributes!"); return NL_SKIP; }
 if (sinfo[NL80211_STA_INFO_SIGNAL]) 
   { wifi_signal = 100+(int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]); }
 if (sinfo[NL80211_STA_INFO_TX_BITRATE]){  
  if (nla_parse_nested(rinfo, NL80211_RATE_INFO_MAX, sinfo[NL80211_STA_INFO_TX_BITRATE], rate_policy)) {
    fprintf(stderr, "failed to parse nested rate attributes!"); } 
  else {
    if (rinfo[NL80211_RATE_INFO_BITRATE])
      { wifi_bitrate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE])/10 >> 3; }
  }
 }
 return NL_SKIP;
}

static int initNl80211() {
    nlsocket = nl_socket_alloc(); 
    if (!nlsocket) { 
       fprintf(stderr, "Failed to allocate netlink socket.\n");
       return -ENOMEM; }
    nl_socket_set_buffer_size(nlsocket, 8192, 8192);
    if (genl_connect(nlsocket)) { 
       fprintf(stderr, "Failed to connect to netlink socket.\n"); 
       nl_close(nlsocket); nl_socket_free(nlsocket); return -ENOLINK; }
    nl80211_id = genl_ctrl_resolve(nlsocket, "nl80211");
    if (nl80211_id < 0) {
       fprintf(stderr, "Nl80211 interface not found.\n");
       nl_close(nlsocket); nl_socket_free(nlsocket); return -ENOENT;  }
    cb1 = nl_cb_alloc(NL_CB_DEFAULT);
    cb2 = nl_cb_alloc(NL_CB_DEFAULT);
    if ((!cb1) || (!cb2)) { 
       fprintf(stderr, "Failed to allocate netlink callback.\n"); 
       nl_close(nlsocket); nl_socket_free(nlsocket); return -3; }
    nl_cb_set(cb1, NL_CB_VALID , NL_CB_CUSTOM, getWifiIface_callback, NULL);
    nl_cb_set(cb1, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &result1);
    nl_cb_set(cb2, NL_CB_VALID , NL_CB_CUSTOM, getWifiInfo_callback, NULL);
    nl_cb_set(cb2, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &result2);
    return 1;
}

static int checkInternet() {
    int err, internet = 1;
    char node[NI_MAXHOST];
    struct sockaddr_in serv;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) { perror("Socket error"); return -3; }
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(INTERNET_TESTSERVER);
    serv.sin_port = htons(TESTSERVER_PORT);
    err = getnameinfo((struct sockaddr*)&serv, sizeof(serv), node, sizeof(node), NULL, 0, 0);
    if (err) //Is DNS lookup NOT working?
      { 
       fprintf(stderr, "%s\n", gai_strerror(err)); internet = -1;
       if (connect(sock, (const struct sockaddr*)&serv, sizeof(serv)) < 0) //Is IP connection NOT working?
         { perror("Connection error"); internet = -2; }
      }
    close(sock);
    return internet;
    /*** GET LOCAL IP ***
    struct sockaddr_in name; socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);
    char buffer[100]; const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);
    if(p != NULL) { printf("Local ip is : %s \n" , buffer); */
}

static void getWifiStatus() {
  int internet = -100;
  char bgcolor[20], fgcolor[20]; 
  /* invalidate variables */
  wifi_index = -1;
  wifi_bitrate = 0;
  wifi_signal = 0;
  if (initnl != 1)
    {
     initnl = initNl80211(); 
     if (initnl != 1) { strcpy(wifistatus, RED"NETLINK ERROR"); return; }
    } 
  result1 = 1;
  msg1 = nlmsg_alloc();
  if (!msg1) { fprintf(stderr, "Failed to allocate netlink message.\n"); strcpy(wifistatus, RED"NLMSG_ALLOC ERROR"); return; }
  genlmsg_put(msg1, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);
  nl_send_auto(nlsocket, msg1);
  while (result1 > 0) { nl_recvmsgs(nlsocket, cb1); }
  nlmsg_free(msg1);
  //printf("msg1 exit\n");
  if (wifi_index < 0) { strcpy(wifistatus, RED"NO WIFI");  return; }
  result2 = 1;
  msg2 = nlmsg_alloc();
  if (!msg2) { fprintf(stderr, "Failed to allocate netlink message.\n"); strcpy(wifistatus, RED"NLMSG_ALLOC ERROR"); return; }
  genlmsg_put(msg2, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
  nla_put_u32(msg2, NL80211_ATTR_IFINDEX, wifi_index); 
  nl_send_auto(nlsocket, msg2); 
  while (result2 > 0) { nl_recvmsgs(nlsocket, cb2); }
  nlmsg_free(msg2);
  //printf("msg2 exit\n");
  if (wifi_signal > 0) { internet = checkInternet(); }
  if (internet < -1)     { strcpy(bgcolor,    DARKRED); strcpy(fgcolor,    RED); } //No Internet connection
  else if (internet < 0) { strcpy(bgcolor, DARKYELLOW); strcpy(fgcolor, YELLOW); } //No DNS lookup 
  else                   { strcpy(bgcolor,  DARKGREEN); strcpy(fgcolor,  GREEN); } //OK
  strcpy(wifistatus, ORANGE);
  strcat(wifistatus, wifi_iface);
  strcat(wifistatus, ":");
  strcat(wifistatus, fgcolor);
  if (wifi_signal <  1) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,14,2,2^^f3^");
  if (wifi_signal < 20) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,12,2,4^^f3^");
  if (wifi_signal < 25) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,10,2,6^^f3^"); 
  if (wifi_signal < 30) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,8,2,8^^f3^");
  if (wifi_signal < 35) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,6,2,10^^f3^");
  if (wifi_signal < 40) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,4,2,12^^f3^");
  if (wifi_signal < 45) { strcat(wifistatus, bgcolor); } strcat(wifistatus, "^r0,2,2,14^" SPACE WHITE);
  sprintf(buf, "%dMB/s", wifi_bitrate); strcat(wifistatus, buf);
  return;
}

static void getMemStatus() {
  unsigned long int availmem; // = sysconf(_SC_AVPHYS_PAGES)*sysconf(_SC_PAGESIZE)>>20;
  
  FILE *fd = fopen(MEMINFO_FILE, "r");
  if(fd == NULL) { perror("Error opening " MEMINFO_FILE); strcpy(memstatus, RED"MEM ERROR"); return; }
  fscanf(fd, "MemTotal:%*[ ]%*u kB\nMemFree:%*[ ]%*u kB\nMemAvailable:%*[ ]%lu", &availmem);
  fclose(fd);
  
  strcpy(memstatus, ORANGE"MEM:");
  availmem >>= 10; //Bytes --> MegaBytes
  if      (availmem > 500) { strcat(memstatus,  WHITE); } //normal color
  else if (availmem < 100) { strcat(memstatus,    RED); } //danger color
  else                     { strcat(memstatus, YELLOW); } //warning color
  sprintf(buf, "%luMB", availmem); strcat(memstatus, buf);
  return;
}

static void getCPUStatus() {
  FILE *fd;
  int cputemp;
  float loadavg_1m, loadavg_5m, loadavg_15m;

  strcpy(cpustatus, ORANGE"CPU:");
  fd = fopen(LOADAVG_FILE, "r");
  if(fd == NULL) {
    perror("Error opening " LOADAVG_FILE); strcat(cpustatus, RED"ERR"); }
  else {
    fscanf(fd, "%f %f %f", &loadavg_1m, &loadavg_5m, &loadavg_15m); fclose(fd);
    if      (loadavg_1m < 1) { strcat(cpustatus,  WHITE); } //normal color
    else if (loadavg_1m > 2) { strcat(cpustatus,    RED); } //danger color
    else                     { strcat(cpustatus, YELLOW); } //warning color
    sprintf(buf, "%.2f", loadavg_1m);  strcat(cpustatus, buf); }
  
  strcat(cpustatus, SPACE SPACE);

  fd = fopen(CPUTEMP_FILE, "r");
  if(fd == NULL) {
    perror("Error opening " CPUTEMP_FILE); strcat(cpustatus, RED"ERR"); }
  else {
    fscanf(fd, "%2d", &cputemp); fclose(fd);
    if      (cputemp < 31) { strcat(cpustatus,  GREEN); } //normal color
    else if (cputemp > 40) { strcat(cpustatus,    RED); } //danger color
    else                   { strcat(cpustatus, YELLOW); } //warning color
    sprintf(buf, "%d°C", cputemp); strcat(cpustatus, buf); }    
  return;
}

static void getDateTime() {
  time_t result;
  struct tm *resulttm;
  result = time(NULL);
  resulttm = localtime(&result);
  if(resulttm == NULL) { perror("Error getting localtime."); strcpy(datestatus, RED"DATE ERROR"); return; } 
  if(!strftime(datestatus, sizeof(char)*300-1, GREENYELLOW "%H:%M" WHITE "^f5^ %a ^f1^ %d/%m", resulttm))
    { perror("strftime is 0."); strcpy(datestatus, RED"DATE ERROR"); return; }
  return;
}

static void getKBLayout() {
    int c = 0;
    char *token;
    char layouts[10][3];
    char delims[] = {'(', ':'};
    XkbStateRec xkbState;
    XkbGetState(dpy, XkbUseCoreKbd, &xkbState);
    XkbDescRec* xkb = XkbAllocKeyboard();
    if (xkb == NULL) { return; }
    XkbGetNames(dpy, XkbSymbolsNameMask, xkb);
    Atom symName = xkb -> names -> symbols;
    if (symName == None) { XkbFreeKeyboard(xkb, XkbNamesMask, True); return; }
    char* layoutString = XGetAtomName(dpy, symName);
    if (layoutString == NULL) { XkbFreeKeyboard(xkb, XkbNamesMask, True); return; }
    //fprintf(stderr, "Layout string: %s\n", layoutString);
    token = strtok(layoutString, "+");
    if (strcmp(token, "pc")==0) { token = strtok(NULL, "+"); } //skip "pc"
    if (token != NULL) { 
      strcpy_uptodelim(layouts[c], token, delims);
      c++;
    }  
    while (token != NULL) {
      if (strchr(token,':') != NULL){
        strcpy_uptodelim(layouts[c], token, delims);
        c++;
      }
      token = strtok(NULL, "+");
    }
    strcpy(keysstatus, "^p");
    strcat(keysstatus, "/usr/local/share/dwm/flags/");
    strcat(keysstatus, layouts[xkbState.locked_group]);
    strcat(keysstatus, ".xpm^");
    XFree(layoutString);
    XkbFreeKeyboard(xkb, XkbNamesMask, True);
    return;
  }

/* *******************************************************************
 * MAIN
 ******************************************************************* */
int main(void) {
  XEvent ev;
  struct timespec ts_1, ts_2;
  int x11_fd, retval;
  int opcode, event, error, major, minor;
  fd_set in_fds;
  struct timeval tv;
  atexit(cleaning);
  if((keysstatus = (char*) malloc(sizeof(char)* 300)) == NULL) { perror("malloc()"); return EXIT_FAILURE; }
  if((datestatus = (char*) malloc(sizeof(char)* 300)) == NULL) { perror("malloc()"); return EXIT_FAILURE; }
  if((cpustatus  = (char*) malloc(sizeof(char)* 300)) == NULL) { perror("malloc()"); return EXIT_FAILURE; }
  if((memstatus  = (char*) malloc(sizeof(char)* 300)) == NULL) { perror("malloc()"); return EXIT_FAILURE; }
  if((wifistatus = (char*) malloc(sizeof(char)* 300)) == NULL) { perror("malloc()"); return EXIT_FAILURE; }
  if((status     = (char*) malloc(sizeof(char)*2000)) == NULL) { perror("malloc()"); return EXIT_FAILURE; }
  if (!(dpy = XOpenDisplay(NULL))) { perror("XOpenDisplay()"); return EXIT_FAILURE; }
  if (!XkbQueryExtension(dpy, &opcode, &event, &error, &major, &minor))
    { XKB_SUPPORTED = false; fprintf(stderr, "XKB extension not supported by X-server\n"); }
  else 
    { XKB_SUPPORTED = true ; }  
  if (XKB_SUPPORTED) {
    XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
    getKBLayout(); 
  } else {
    strcpy(keysstatus, "err");
  }  
  x11_fd = ConnectionNumber(dpy);
  getDateTime();
  getCPUStatus();
  getMemStatus();
  getWifiStatus();
  clock_gettime(CLOCK_MONOTONIC, &ts_1);
  while(1)  {
    FD_ZERO(&in_fds);
    FD_SET(x11_fd, &in_fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    retval = select(x11_fd+1, &in_fds, 0, 0, &tv);
    if (retval == -1)
      { perror("select()"); }
    else if (retval > 0)
      { 
        while(XPending(dpy)) { XNextEvent(dpy, &ev);  if (XKB_SUPPORTED) { getKBLayout(); } }
      }
    clock_gettime(CLOCK_MONOTONIC, &ts_2);
    if (ts_2.tv_sec - ts_1.tv_sec >= 5)
      { 
       //printf("Time: %d\n", ts_2.tv_sec - ts_1.tv_sec);
       clock_gettime(CLOCK_MONOTONIC, &ts_1);
       if (XKB_SUPPORTED) { getKBLayout(); }
       getDateTime();
       getCPUStatus();
       getMemStatus();
       getWifiStatus();
      }
    snprintf(status, 2000, "%s %s %s %s %s %s %s %s %s %s %s %s",
             SPACE SPACE SPACE, keysstatus, SPACE SPACE SPACE, datestatus, SPACE,
             SPACE, cpustatus, SPACE, memstatus,
             SPACE, wifistatus, SPACE SPACE SPACE SPACE SPACE);
    setStatus(dpy, status);
  }
}
