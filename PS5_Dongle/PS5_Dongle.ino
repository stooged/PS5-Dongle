#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <FS.h>
#include <WiFi.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ESPmDNS.h>
#include <Update.h>
#include <DNSServer.h>
#include "ESPAsyncWebServer.h"
#include "SD_MMC.h"
#include "USB.h"
#include "USBMSC.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "etahen.h"


#define USETFT false  // use dongle lcd screen

#if USETFT
#include <TFT_eSPI.h>  // https://github.com/stooged/TFT_eSPI
TFT_eSPI tft = TFT_eSPI();  
long tftCnt = 0;
bool tftOn = true;
#endif


//-------------------DEFAULT SETTINGS------------------//

                       // use config.ini [ true / false ]
#define USECONFIG true // this will allow you to change these settings below via the admin webpage or the config.ini file.
                       // if you want to permanently use the values below then set this to false.

// create access point
boolean startAP = true;
String AP_SSID = "PS5_WEB_AP";
String AP_PASS = "password";
IPAddress Server_IP(10, 1, 1, 1);
IPAddress Subnet_Mask(255, 255, 255, 0);

// connect to wifi
boolean connectWifi = false;
String WIFI_SSID = "Home_WIFI";
String WIFI_PASS = "password";
String WIFI_HOSTNAME = "ps5.local";

// load etaHen automatically
boolean autoHen = false;

// ESP sleep after x minutes
boolean espSleep = false;
int TIME2SLEEP = 30; // minutes

//-----------------------------------------------------//

USBMSC MSC;
#define MOUNT_POINT "/sdcard"
#define PDESC "PS5-Dongle"
#define MDESC "T-D-S3"
#define firmwareVer "1.0"
sdmmc_card_t *card;
#include "Pages.h"
DNSServer dnsServer;
using namespace httpsserver;
SSLCert *cert;
HTTPSServer *secureServer;
AsyncWebServer server(80);
File upFile;
long bootTime = 0;
String henBin = "";
boolean isRebooting = false;
//USBCDC USBSerial;



String split(String str, String from, String to)
{
  String tmpstr = str;
  tmpstr.toLowerCase();
  from.toLowerCase();
  to.toLowerCase();
  int pos1 = tmpstr.indexOf(from);
  int pos2 = tmpstr.indexOf(to, pos1 + from.length());
  String retval = str.substring(pos1 + from.length(), pos2);
  return retval;
}

bool instr(String str, String search)
{
  int result = str.indexOf(search);
  if (result == -1)
  {
    return false;
  }
  return true;
}

String formatBytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0) + " KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String(bytes / 1024.0 / 1024.0) + " MB";
  }
  else
  {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
  }
}

void findHen(String directory)
{
  File dir = SD_MMC.open(directory, "r");
  while (dir)
  { 
    File file = dir.openNextFile();
    if (!file)
    {
      dir.close();
      break;
    }
    String fName = file.name();
    if (!fName.startsWith("System Volume Information"))
    {
      if (file.isDirectory()){
        findHen("/" + fName);
      }else{
        String lcfName = fName;
        lcfName.toLowerCase();
        if (lcfName.startsWith("etahen"))
        {
          henBin = fName;
        }
      }
    }
    file.close();
    if (henBin.length() > 0){
      dir.close();
      break;
    }
  }
}

String getMimeType(String filename)
{
  if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".mjs"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".bin"))
    return "application/octet-stream";
  else if (filename.endsWith(".manifest"))
    return "text/cache-manifest";
  return "text/plain";
}

void handleInfo(AsyncWebServerRequest *request)
{
  float flashFreq = (float)ESP.getFlashChipSpeed() / 1000.0 / 1000.0;
  FlashMode_t ideMode = ESP.getFlashChipMode();
  String mcuType = CONFIG_IDF_TARGET;
  mcuType.toUpperCase();
  String output = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>System Information</title><link rel=\"stylesheet\" href=\"style.css\"></head>";
  output += "<hr>###### Software ######<br><br>";
  output += "Firmware version " + String(firmwareVer) + "<br>";
  output += "SDK version: " + String(ESP.getSdkVersion()) + "<br><hr>";
  output += "###### Board ######<br><br>";
  output += "MCU: " + mcuType + "<br>";
  output += "Board: LilyGo T-Dongle-S3<br>";
  output += "Chip Id: " + String(ESP.getChipModel()) + "<br>";
  output += "CPU frequency: " + String(ESP.getCpuFreqMHz()) + "MHz<br>";
  output += "Cores: " + String(ESP.getChipCores()) + "<br><hr>";
  output += "###### Flash chip information ######<br><br>";
  output += "Flash chip Id: " + String(ESP.getFlashChipMode()) + "<br>";
  output += "Estimated Flash size: " + formatBytes(ESP.getFlashChipSize()) + "<br>";
  output += "Flash frequency: " + String(flashFreq) + " MHz<br>";
  output += "Flash write mode: " + String((ideMode == FM_QIO? "QIO": ideMode == FM_QOUT? "QOUT": ideMode == FM_DIO? "DIO": ideMode == FM_DOUT? "DOUT": "UNKNOWN")) + "<br><hr>";
  output += "###### Storage information ######<br><br>";
  output += "Storage Device: SD<br>";
  output += "Total Size: " + formatBytes(SD_MMC.totalBytes()) + "<br>";
  output += "Used Space: " + formatBytes(SD_MMC.usedBytes()) + "<br>";
  output += "Free Space: " + formatBytes(SD_MMC.totalBytes() - SD_MMC.usedBytes()) + "<br><hr>";
  output += "###### Ram information ######<br><br>";
  output += "Ram size: " + formatBytes(ESP.getHeapSize()) + "<br>";
  output += "Free ram: " + formatBytes(ESP.getFreeHeap()) + "<br>";
  output += "Max alloc ram: " + formatBytes(ESP.getMaxAllocHeap()) + "<br><hr>";
  output += "###### Sketch information ######<br><br>";
  output += "Sketch hash: " + ESP.getSketchMD5() + "<br>";
  output += "Sketch size: " + formatBytes(ESP.getSketchSize()) + "<br>";
  output += "Free space available: " + formatBytes(ESP.getFreeSketchSpace() - ESP.getSketchSize()) + "<br><hr>";
  output += "</html>";
  request->send(200, "text/html", output);
}

void handleReboot(AsyncWebServerRequest *request)
{
  //USBSerial.print("Rebooting ESP");
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", rebooting_gz, sizeof(rebooting_gz));
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
  delay(1000);
  isRebooting = true;
}

void handlePayload(AsyncWebServerRequest *request)
{
  henBin = "";
  findHen("/");
  if (henBin.length() >0)
  {
    if (henBin.endsWith(".gz"))
    {
      henBin.replace(".gz", "");
    }
  }
  else
  {
    henBin = "etahen.bin";
  }
  String output = "const payload_map =\r\n[";
  output += "{\r\n";
  output += "displayTitle: 'etaHEN',\r\n";
  output += "description: 'Runs With 3.xx and 4.xx. FPKG enabler For FW 3.xx / 4.03-4.51 Only.',\r\n";  
  output += "fileName: '" + henBin + "',\r\n";
  output += "author: 'LightningMods_, sleirsgoevy, ChendoChap, astrelsky, illusion',\r\n";
  output += "source: 'https://github.com/LightningMods/etaHEN',\r\n";
  output += "version: 'auto'\r\n}\r\n";
  output += "\r\n];";
  request->send(200, "application/javascript", output);
}

#if USECONFIG
void handleConfigHtml(AsyncWebServerRequest *request)
{
  String tmpUa = "";
  String tmpCw = "";
  String tmpAh = "";
  String tmpSlp = "";
  if (startAP)
  {
    tmpUa = "checked";
  }
  if (connectWifi)
  {
    tmpCw = "checked";
  }
  if (autoHen)
  {
    tmpAh = "checked";
  }
  if (espSleep)
  {
    tmpSlp = "checked";
  }
  String htmStr = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>Config Editor</title><style type=\"text/css\">body {background-color: #1451AE; color: #ffffff; font-size: 14px;font-weight: bold;margin: 0 0 0 0.0;padding: 0.4em 0.4em 0.4em 0.6em;}input[type=\"submit\"]:hover {background: #ffffff;color: green;}input[type=\"submit\"]:active{outline-color: green;color: green;background: #ffffff; }table {font-family: arial, sans-serif;border-collapse: collapse;}td {border: 1px solid #dddddd;text-align: left;padding: 8px;}th {border: 1px solid #dddddd; background-color:gray;text-align: center;padding: 8px;}</style></head><body><form action=\"/config.html\" method=\"post\"><center><table><tr><th colspan=\"2\"><center>Access Point</center></th></tr><tr><td>AP SSID:</td><td><input name=\"ap_ssid\" value=\"" + AP_SSID + "\"></td></tr><tr><td>AP PASSWORD:</td><td><input name=\"ap_pass\" value=\"********\"></td></tr><tr><td>AP IP:</td><td><input name=\"web_ip\" value=\"" + Server_IP.toString() + "\"></td></tr><tr><td>SUBNET MASK:</td><td><input name=\"subnet\" value=\"" + Subnet_Mask.toString() + "\"></td></tr><tr><td>START AP:</td><td><input type=\"checkbox\" name=\"useap\" " + tmpUa + "></td><tr><th colspan=\"2\"><center>Wifi Connection</center></th></tr><tr><td>WIFI SSID:</td><td><input name=\"wifi_ssid\" value=\"" + WIFI_SSID + "\"></td></tr><tr><td>WIFI PASSWORD:</td><td><input name=\"wifi_pass\" value=\"********\"></td></tr><tr><td>WIFI HOSTNAME:</td><td><input name=\"wifi_host\" value=\"" + WIFI_HOSTNAME + "\"></td></tr><tr><td>CONNECT WIFI:</td><td><input type=\"checkbox\" name=\"usewifi\" " + tmpCw + "></td></tr><tr><th colspan=\"2\"><center>AutoHen</center></th></tr><tr><td>Enable AutoHen:</td><td><input type=\"checkbox\" name=\"autohen\" " + tmpAh + "></td></tr><tr><th colspan=\"2\"><center>ESP Sleep Mode</center></th></tr><tr><td>ENABLE SLEEP:</td><td><input type=\"checkbox\" name=\"espsleep\" " + tmpSlp + "></td></tr><tr><td>TIME TO SLEEP(minutes):</td><td><input name=\"sleeptime\" value=\"" + TIME2SLEEP + "\"></td></tr></table><br><input id=\"savecfg\" type=\"submit\" value=\"Save Config\"></center></form></body></html>";
  request->send(200, "text/html", htmStr);
}

void handleConfig(AsyncWebServerRequest *request)
{
  if (request->hasParam("ap_ssid", true) && request->hasParam("ap_pass", true) && request->hasParam("web_ip", true) && request->hasParam("subnet", true) && request->hasParam("wifi_ssid", true) && request->hasParam("wifi_pass", true) && request->hasParam("wifi_host", true))
  {
    AP_SSID = request->getParam("ap_ssid", true)->value();
    if (!request->getParam("ap_pass", true)->value().equals("********"))
    {
      AP_PASS = request->getParam("ap_pass", true)->value();
    }
    WIFI_SSID = request->getParam("wifi_ssid", true)->value();
    if (!request->getParam("wifi_pass", true)->value().equals("********"))
    {
      WIFI_PASS = request->getParam("wifi_pass", true)->value();
    }
    String tmpip = request->getParam("web_ip", true)->value();
    String tmpsubn = request->getParam("subnet", true)->value();
    String WIFI_HOSTNAME = request->getParam("wifi_host", true)->value();
    String tmpua = "false";
    String tmpcw = "false";
    String tmpah = "false";
    String tmpslp = "false";
    if (request->hasParam("useap", true))
    {
      tmpua = "true";
    }
    if (request->hasParam("usewifi", true))
    {
      tmpcw = "true";
    }
    if (tmpua.equals("false") && tmpcw.equals("false"))
    {
      tmpua = "true";
    }
    if (request->hasParam("autohen", true))
    {
      tmpah = "true";
    }
    if (request->hasParam("espsleep", true))
    {
      tmpslp = "true";
    }
    int TIME2SLEEP = request->getParam("sleeptime", true)->value().toInt();
    File iniFile = SD_MMC.open("/config.ini", "w");
    if (iniFile)
    {
      iniFile.print("NOTE: after making changes to this file you must reboot the dongle for the changes to take effect\r\n\r\nAP_SSID=" + AP_SSID + "\r\nAP_PASS=" + AP_PASS + "\r\nWEBSERVER_IP=" + tmpip + "\r\nSUBNET_MASK=" + tmpsubn + "\r\nWIFI_SSID=" + WIFI_SSID + "\r\nWIFI_PASS=" + WIFI_PASS + "\r\nWIFI_HOST=" + WIFI_HOSTNAME + "\r\nUSEAP=" + tmpua + "\r\nCONWIFI=" + tmpcw + "\r\nAUTOHEN=" + tmpah + "\r\nESPSLEEP=" + tmpslp + "\r\nSLEEPTIME=" + TIME2SLEEP + "\r\n");
      iniFile.close();
    }
    String htmStr = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"8; url=/info.html\"><style type=\"text/css\">#loader {z-index: 1;width: 50px;height: 50px;margin: 0 0 0 0;border: 6px solid #f3f3f3;border-radius: 50%;border-top: 6px solid #3498db;width: 50px;height: 50px;-webkit-animation: spin 2s linear infinite;animation: spin 2s linear infinite; } @-webkit-keyframes spin {0%{-webkit-transform: rotate(0deg);}100%{-webkit-transform: rotate(360deg);}}@keyframes spin{0%{ transform: rotate(0deg);}100%{transform: rotate(360deg);}}body {background-color: #1451AE; color: #ffffff; font-size: 20px; font-weight: bold; margin: 0 0 0 0.0; padding: 0.4em 0.4em 0.4em 0.6em;} #msgfmt {font-size: 16px; font-weight: normal;}#status {font-size: 16px; font-weight: normal;}</style></head><center><br><br><br><br><br><p id=\"status\"><div id='loader'></div><br>Config saved<br>Rebooting</p></center></html>";
    request->send(200, "text/html", htmStr);
    delay(1000);
    isRebooting = true;
  }
  else
  {
    request->redirect("/config.html");
  }
}
#endif

void sendwebmsg(AsyncWebServerRequest *request, String htmMsg)
{
  String tmphtm = "<!DOCTYPE html><html><head><link rel=\"stylesheet\" href=\"style.css\"></head><center><br><br><br><br><br><br>" + htmMsg + "</center></html>";
  request->send(200, "text/html", tmphtm);
}

void handleFwUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    String path = request->url();
    if (path != "/update.html")
    {
      request->send(500, "text/plain", "Internal Server Error");
      return;
    }
    if (!filename.equals("fwupdate.bin"))
    {
      sendwebmsg(request, "Invalid update file: " + filename);
      return;
    }
    if (!filename.startsWith("/"))
    {
      filename = "/" + filename;
    }
    //USBSerial.printf("Update Start: %s\n", filename.c_str());
    if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
    {
      Update.printError(Serial);
      sendwebmsg(request, "Update Failed: " + String(Update.errorString()));
    }
  }
  if (!Update.hasError())
  {
    if (Update.write(data, len) != len)
    {
      Update.printError(Serial);
      sendwebmsg(request, "Update Failed: " + String(Update.errorString()));
    }
  }
  if (final)
  {
    if (Update.end(true))
    {
      //USBSerial.printf("Update Success: %uB\n", index+len);
      String tmphtm = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"8; url=/info.html\"><style type=\"text/css\">body {background-color: #1451AE; color: #ffffff; font-size: 20px; font-weight: bold; margin: 0 0 0 0.0; padding: 0.4em 0.4em 0.4em 0.6em;}</style></head><center><br><br><br><br><br><br>Update Success, Rebooting.</center></html>";
      request->send(200, "text/html", tmphtm);
      delay(1000);
      isRebooting = true;
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

void handleHTTPS(HTTPRequest *req, HTTPResponse *res)
{
  String path = req->getRequestString().c_str();
  if (instr(path, "/document/") && instr(path, "/ps5/"))
  {
    std::string serverHost = WIFI_HOSTNAME.c_str();
    res->setStatusCode(301);
    res->setStatusText("Moved Permanently");
    res->setHeader("Location", "http://" + serverHost + "/index.html");
    res->println("Moved Permanently");
  }
  else if (instr(path, "/update/") && instr(path, "/ps5/"))
  {
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->setHeader("Content-Type", "application/xml");
    res->println("<?xml version=\"1.0\" ?><update_data_list><region id=\"us\"><force_update><system auto_update_version=\"00.00\" sdk_version=\"01.00.00.09-00.00.00.0.0\" upd_version=\"01.00.00.00\"/></force_update><system_pup auto_update_version=\"00.00\" label=\"20.02.02.20.00.07-00.00.00.0.0\" sdk_version=\"02.20.00.07-00.00.00.0.0\" upd_version=\"02.20.00.00\"><update_data update_type=\"full\"></update_data></system_pup></region></update_data_list>");
  }
  else
  {
    res->setStatusCode(404);
    res->setStatusText("Not Found");
    res->setHeader("Content-Type", "text/plain");
    res->println("Not Found");
  }
}

#if USECONFIG
void writeConfig()
{
  File iniFile = SD_MMC.open("/config.ini", "w");
  if (iniFile)
  {
    String tmpua = "false";
    String tmpcw = "false";
    String tmpah = "false";
    String tmpslp = "false";
    if (startAP)
    {
      tmpua = "true";
    }
    if (connectWifi)
    {
      tmpcw = "true";
    }
    if (espSleep)
    {
      tmpslp = "true";
    }
    if (autoHen){tmpah = "true";}
    iniFile.print("NOTE: after making changes to this file you must reboot the dongle for the changes to take effect\r\n\r\nAP_SSID=" + AP_SSID + "\r\nAP_PASS=" + AP_PASS + "\r\nWEBSERVER_IP=" + Server_IP.toString() + "\r\nSUBNET_MASK=" + Subnet_Mask.toString() + "\r\nWIFI_SSID=" + WIFI_SSID + "\r\nWIFI_PASS=" + WIFI_PASS + "\r\nWIFI_HOST=" + WIFI_HOSTNAME + "\r\nUSEAP=" + tmpua + "\r\nCONWIFI=" + tmpcw + "\r\nAUTOHEN=" + tmpah + "\r\nESPSLEEP=" + tmpslp + "\r\nSLEEPTIME=" + TIME2SLEEP + "\r\n");
    iniFile.close();
  }
}
#endif

void setup_SD(void)
{
  esp_err_t ret;
  const char mount_point[] = MOUNT_POINT;
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {.format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};
  sdmmc_host_t host = {
      .flags = SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_DDR,
      .slot = SDMMC_HOST_SLOT_1,
      .max_freq_khz = SDMMC_FREQ_DEFAULT,
      .io_voltage = 3.3f,
      .init = &sdmmc_host_init,
      .set_bus_width = &sdmmc_host_set_bus_width,
      .get_bus_width = &sdmmc_host_get_slot_width,
      .set_bus_ddr_mode = &sdmmc_host_set_bus_ddr_mode,
      .set_card_clk = &sdmmc_host_set_card_clk,
      .do_transaction = &sdmmc_host_do_transaction,
      .deinit = &sdmmc_host_deinit,
      .io_int_enable = sdmmc_host_io_int_enable,
      .io_int_wait = sdmmc_host_io_int_wait,
      .command_timeout_ms = 0,
  };
  sdmmc_slot_config_t slot_config = {
      .clk = (gpio_num_t)12,
      .cmd = (gpio_num_t)16,
      .d0 = (gpio_num_t)14,
      .d1 = (gpio_num_t)17,
      .d2 = (gpio_num_t)21,
      .d3 = (gpio_num_t)18,
      .cd = SDMMC_SLOT_NO_CD,
      .wp = SDMMC_SLOT_NO_WP,
      .width = 4,
      .flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP,
  };
  gpio_set_pull_mode((gpio_num_t)16, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)14, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)17, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)21, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)18, GPIO_PULLUP_ONLY);
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
  uint32_t count = (bufsize / card->csd.sector_size);
  sdmmc_write_sectors(card, buffer + offset, lba, count);
  return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
  uint32_t count = (bufsize / card->csd.sector_size);
  sdmmc_read_sectors(card, buffer + offset, lba, count);
  return bufsize;
}

void startAccessPoint()
{
  if (startAP)
  {
    WiFi.softAPConfig(Server_IP, Server_IP, Subnet_Mask);
    WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str());
    //USBSerial.println("WIFI AP started");
    dnsServer.setTTL(30);
    dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    dnsServer.start(53, "*", Server_IP);
#if USETFT
    tft.print("AP: ");
    tft.println(AP_SSID);
    if (!connectWifi)
    {
      tft.print("Host: ");
      tft.println(WIFI_HOSTNAME);
    }
    tft.print("IP: ");
    tft.println(Server_IP.toString());
#endif
  }
}

void connectToWIFI()
{
  if (connectWifi && WIFI_SSID.length() > 0 && WIFI_PASS.length() > 0)
  {
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.hostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    //USBSerial.println("WIFI connecting");
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      //USBSerial.println("Wifi failed to connect");
#if USETFT
      tft.setTextColor(TFT_RED, TFT_BLACK);          
      tft.println("Failed to connect to:");
      tft.setTextColor(TFT_WHITE, TFT_BLACK);  
      tft.println(WIFI_SSID);
#endif      
    }
    else
    {
      IPAddress LAN_IP = WiFi.localIP();
      if (LAN_IP)
      {
        String mdnsHost = WIFI_HOSTNAME;
        mdnsHost.replace(".local", "");
        MDNS.begin(mdnsHost.c_str());
#if USETFT        
        tft.print("Host: ");
        tft.println(WIFI_HOSTNAME);
        tft.print("IP: ");
        tft.println(LAN_IP.toString()); 
#endif
        if (!startAP)
        {
          dnsServer.setTTL(30);
          dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
          dnsServer.start(53, "*", LAN_IP);
        }
      }
    }
  }
}

void setup()
{
  //USBSerial.begin(115200);
  //USBSerial.println("Version: " + String(firmwareVer));
  
  gpio_hold_dis((gpio_num_t)38);
  gpio_hold_dis((gpio_num_t)0);
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH); 

#if USETFT   
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, 160, 320, logo);
  digitalWrite(38, LOW);
#endif 
  
  setup_SD();
  MSC.vendorID(MDESC);
  MSC.productID(PDESC);
  MSC.productRevision(firmwareVer);
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.mediaPresent(true);
  MSC.begin(card->csd.capacity, card->csd.sector_size);
  USB.productName(PDESC);
  USB.manufacturerName(MDESC);
  USB.begin();

  SD_MMC.setPins(12, 16, 14, 17, 21, 18);
  if (SD_MMC.begin())
  {
#if USECONFIG
    if (SD_MMC.exists("/config.ini"))
    {
      File iniFile = SD_MMC.open("/config.ini", "r");
      if (iniFile)
      {
        String iniData;
        while (iniFile.available())
        {
          char chnk = iniFile.read();
          iniData += chnk;
        }
        iniFile.close();

        if (instr(iniData, "AP_SSID="))
        {
          AP_SSID = split(iniData, "AP_SSID=", "\r\n");
          AP_SSID.trim();
        }

        if (instr(iniData, "AP_PASS="))
        {
          AP_PASS = split(iniData, "AP_PASS=", "\r\n");
          AP_PASS.trim();
        }

        if (instr(iniData, "WEBSERVER_IP="))
        {
          String strwIp = split(iniData, "WEBSERVER_IP=", "\r\n");
          strwIp.trim();
          Server_IP.fromString(strwIp);
        }

        if (instr(iniData, "SUBNET_MASK="))
        {
          String strsIp = split(iniData, "SUBNET_MASK=", "\r\n");
          strsIp.trim();
          Subnet_Mask.fromString(strsIp);
        }

        if (instr(iniData, "WIFI_SSID="))
        {
          WIFI_SSID = split(iniData, "WIFI_SSID=", "\r\n");
          WIFI_SSID.trim();
        }

        if (instr(iniData, "WIFI_PASS="))
        {
          WIFI_PASS = split(iniData, "WIFI_PASS=", "\r\n");
          WIFI_PASS.trim();
        }

        if (instr(iniData, "WIFI_HOST="))
        {
          WIFI_HOSTNAME = split(iniData, "WIFI_HOST=", "\r\n");
          WIFI_HOSTNAME.trim();
        }

        if (instr(iniData, "USEAP="))
        {
          String strua = split(iniData, "USEAP=", "\r\n");
          strua.trim();
          if (strua.equals("true"))
          {
            startAP = true;
          }
          else
          {
            startAP = false;
          }
        }

        if (instr(iniData, "CONWIFI="))
        {
          String strcw = split(iniData, "CONWIFI=", "\r\n");
          strcw.trim();
          if (strcw.equals("true"))
          {
            connectWifi = true;
          }
          else
          {
            connectWifi = false;
          }
        }

        if (instr(iniData, "AUTOHEN=")) {
          String strah = split(iniData, "AUTOHEN=", "\r\n");
          strah.trim();
          strah.toLowerCase();
          if (strah.equals("true")) {
            autoHen = true;
          } else {
            autoHen = false;
          }
        }

        if (instr(iniData, "ESPSLEEP="))
        {
          String strsl = split(iniData, "ESPSLEEP=", "\r\n");
          strsl.trim();
          if (strsl.equals("true"))
          {
            espSleep = true;
          }
          else
          {
            espSleep = false;
          }
        }

        if (instr(iniData, "SLEEPTIME="))
        {
          String strslt = split(iniData, "SLEEPTIME=", "\r\n");
          strslt.trim();
          TIME2SLEEP = strslt.toInt();
        }
      }
    }
    else
    {
      writeConfig();
    }
#endif
  }
  else
  {
    //USBSerial.println("Filesystem failed to mount");
  }


#if USETFT  
  delay(3000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1); //16 col 5 row
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);   
  tft.setCursor(0, 0, 2);
#endif

  startAccessPoint();
  connectToWIFI();

  if (SD_MMC.exists("/pk.pem") && SD_MMC.exists("/cert.der"))
  {
    uint8_t *cBuffer;
    uint8_t *kBuffer;
    unsigned int clen = 0;
    unsigned int klen = 0;
    File certFile = SD_MMC.open("/cert.der", "r");
    clen = certFile.size();
    cBuffer = (uint8_t *)malloc(clen);
    certFile.read(cBuffer, clen);
    certFile.close();
    File pkFile = SD_MMC.open("/pk.pem", "r");
    klen = pkFile.size();
    kBuffer = (uint8_t *)malloc(klen);
    pkFile.read(kBuffer, klen);
    pkFile.close();
    cert = new SSLCert(cBuffer, clen, kBuffer, klen);
  }
  else
  {
    cert = new SSLCert();
    String keyInf = "CN=" + WIFI_HOSTNAME + ",O=" + PDESC + ",C=US";
    int createCertResult = createSelfSignedCert(*cert, KEYSIZE_1024, (std::string)keyInf.c_str(), "20190101000000", "20300101000000");
    if (createCertResult != 0)
    {
      //USBSerial.printf("Certificate failed, Error Code = 0x%02X\n", createCertResult);
    }
    else
    {
      //USBSerial.println("Certificate created");
      File pkFile = SD_MMC.open("/pk.pem", "w");
      pkFile.write(cert->getPKData(), cert->getPKLength());
      pkFile.close();
      File certFile = SD_MMC.open("/cert.der", "w");
      certFile.write(cert->getCertData(), cert->getCertLength());
      certFile.close();
    }
  }


  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/plain", "Microsoft Connect Test"); });
  server.on("/config.ini", HTTP_ANY, [](AsyncWebServerRequest *request) { request->send(404); });
  server.on("/cert.der", HTTP_ANY, [](AsyncWebServerRequest *request){ request->send(404); });
  server.on("/pk.pem", HTTP_ANY, [](AsyncWebServerRequest *request){ request->send(404); });
  server.on("/update.html", HTTP_POST, [](AsyncWebServerRequest *request) {}, handleFwUpdate);
  server.on("/info.html", HTTP_GET, [](AsyncWebServerRequest *request){ handleInfo(request); });

  server.on("/admin.html", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", admin_gz, sizeof(admin_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/reboot.html", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", reboot_gz, sizeof(reboot_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/reboot.html", HTTP_POST, [](AsyncWebServerRequest *request){ handleReboot(request); });

  server.on("/update.html", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", update_gz, sizeof(update_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon, sizeof(favicon));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

#if USECONFIG
  server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest *request){ handleConfigHtml(request); });
  server.on("/config.html", HTTP_POST, [](AsyncWebServerRequest *request){ handleConfig(request); });
#endif

  if (autoHen)
  {
    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_gz, sizeof(index_gz));
      response->addHeader("Content-Encoding", "gzip");
      request->send(response); });

    server.on("/exploit.js", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", exploit_gz, sizeof(exploit_gz));
      response->addHeader("Content-Encoding", "gzip");
      request->send(response); });

    server.on("/payload_map.js", HTTP_GET, [](AsyncWebServerRequest *request){ handlePayload(request); });
  }

  server.serveStatic("/", SD_MMC, "/").setDefaultFile("index.html");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.onNotFound([](AsyncWebServerRequest *request){
    String path = request->url();
    if (path.endsWith("index.htm") || path.endsWith("/")) {
      request->redirect("http://" + WIFI_HOSTNAME + "/index.html");
      return;
    }
    if (path.endsWith("index.html")) {
      sendwebmsg(request, "You need to add a index.html file to the dongles storage.");
      return;
    }
    if (path.endsWith("style.css")) {
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", style_gz, sizeof(style_gz));
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
      return;
    }
    if (SD_MMC.exists(path + ".gz"))
    {
      AsyncWebServerResponse *response = request->beginResponse(SD_MMC, path + ".gz", getMimeType(path));
      response->addHeader("Content-Encoding","gzip");
      request->send(response);
      return;
    }
    if (autoHen)
    {
      if (path.endsWith("etahen.bin")) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "application/octet-stream", etahen_gz, sizeof(etahen_gz));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
      }
    }
    //USBSerial.println(request->url());
    request->send(404); 
  });

  server.begin();

  secureServer = new HTTPSServer(cert);
  ResourceNode *nhttps = new ResourceNode("", "ANY", &handleHTTPS);
  secureServer->setDefaultNode(nhttps);
  secureServer->start();

  if (TIME2SLEEP < 5)
  {
    TIME2SLEEP = 5;
  } // min sleep time
  bootTime = millis();
}

void loop()
{
  dnsServer.processNextRequest();
  secureServer->loop();
  if (espSleep)
  {
    if (millis() >= (bootTime + (TIME2SLEEP * 60000)))
    { 
      digitalWrite(38, HIGH);
      gpio_hold_en((gpio_num_t)38);
      gpio_hold_en((gpio_num_t)0);
      gpio_deep_sleep_hold_en();
      esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
      esp_deep_sleep_start();
      return;
    }
  }
  if (isRebooting)
  {
    delay(2000);
    isRebooting = false;
    ESP.restart();
  }
#if USETFT    
  if (millis() >= (tftCnt + 60000) && tftOn){ 
    tftCnt = 0;
    tftOn = false;
    digitalWrite(38, HIGH);
    return;
  }
  if (digitalRead(0) == LOW){
    if (tftCnt == 0){
       tftCnt = millis();
       digitalWrite(38, LOW);
       tftOn = true;
    }
  }
#endif
}

#else
#error "Selected board not supported"
#endif
