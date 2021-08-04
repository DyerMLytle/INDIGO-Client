#define INDIGO_LINUX 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>

#define SBIG_CCD "SBIG ST-10 CCD #0"
#define SBIG_GUIDER_CCD "SBIG ST-10 Guider CCD #0"
#define SBIG_GUIDER_PORT "SBIG ST-10 Guider Port #0"
#define SBIG_FILTER_WHEEL "SBIG CFW-10 #0"
#define SBIG_ETHERNET "SBIG Ethernet Device"

static bool connected = false;
static bool keepRunning = true;
double ccd_temp = 0.0;

static void intHandler(int dummy) {
  keepRunning = false;
  printf("exiting\n");
}

static indigo_result client_attach(indigo_client* client) {
  indigo_log("attached to INDIGO bus...");
  indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
  return INDIGO_OK;
}

static indigo_result client_define_property(indigo_client* client,
  indigo_device* device, indigo_property* property,
  const char* message) {
  printf("define property: device = %s, name = %s\n",
    property->device, property->name);
  if(!strcmp(property->name, CCD_COOLER_PROPERTY_NAME)) {
    printf("cool prop count = %d\n", property->count);
    printf("cool prop name = %s\n", property->items[0].name);
    printf("cool prop name 2 = %s\n", property->items[1].name);
  }
  return INDIGO_OK;
}

static indigo_result client_update_property(indigo_client* client,
  indigo_device* device, indigo_property* property,
  const char* message) {
  printf("update property: device = %s, name = %s\n",
    property->device, property->name);
  if(strcmp(property->device, SBIG_CCD))
    return INDIGO_OK;
  if(!strcmp(property->name, CCD_TEMPERATURE_PROPERTY_NAME)) {
    printf("temp prop count = %d\n", property->count);
    ccd_temp = property->items[0].number.value;
  }
  return INDIGO_OK;
}

static indigo_result client_detach(indigo_client* client) {
  indigo_log("detached from INDIGO bus...");
  return INDIGO_OK;
}

static indigo_client client = {
    "Dynamic driver client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
    client_attach,
    client_define_property,
    client_update_property,
    NULL,
    NULL,
    client_detach };

int main(int argc, const char* argv[]) {
  indigo_main_argc = argc;
  indigo_main_argv = argv;

  struct sigaction act;
  act.sa_handler = intHandler;
  sigaction(SIGINT, &act, NULL);

  indigo_set_log_level(INDIGO_LOG_INFO);
  indigo_start();

  indigo_driver_entry* driver;
  indigo_attach_client(&client);
  bool myok = indigo_load_driver("indigo_ccd_sbig", true, &driver);
  indigo_usleep(8000000);
  printf("Connecting to the SBIG CCD now\n");
  indigo_device_connect(&client, SBIG_CCD);
  indigo_usleep(8000000);

  static const char* itemsx[] = { CCD_COOLER_ON_ITEM_NAME };
  static bool valuesx[] = { true };
  indigo_change_switch_property(&client, SBIG_CCD, CCD_COOLER_PROPERTY_NAME,
    1, itemsx, valuesx);
  printf("Setting temp to -10.0\n");
  static const char* items[] = { CCD_TEMPERATURE_ITEM_NAME };
  static double values[] = { -10.0 };
  indigo_change_number_property(&client, SBIG_CCD, CCD_TEMPERATURE_PROPERTY_NAME,
          1, items, values);

  while(keepRunning) {
    printf("ccd_temp = %f\n", ccd_temp);
    indigo_usleep(1000000);
  }

  printf("Disconnecting from the SBIG CCD now\n");
  indigo_device_disconnect(&client, SBIG_CCD);
  indigo_usleep(2000000);

  indigo_remove_driver(driver);
  indigo_detach_client(&client);
  indigo_stop();
  return 0;
}
