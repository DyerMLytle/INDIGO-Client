#define INDIGO_LINUX 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>

#define CCD_SIMULATOR "CCD Imager Simulator"
#define SBIG_CCD "SBIG ST-10 CCD #0"

static bool connected = false;
static int count = 2;

static indigo_result client_attach(indigo_client* client) {
  indigo_log("attached to INDIGO bus...");
  indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
  return INDIGO_OK;
}

static indigo_result client_define_property(indigo_client* client,
  indigo_device* device, indigo_property* property,
  const char* message) {
  printf("prop-device = %s\n", property->device);
  printf("prop-name = %s\n", property->name);
  if(strcmp(property->device, SBIG_CCD))
    return INDIGO_OK;
  if(!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
    if(indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
      connected = true;
      indigo_log("already connected...");
      static const char* items[] = { CCD_EXPOSURE_ITEM_NAME };
      static double values[] = { 0.05 };
      indigo_change_number_property(client, SBIG_CCD, CCD_EXPOSURE_PROPERTY_NAME,
        1, items, values);
    } else {
      printf("Connecting to the ccd simulator now\n");
      indigo_device_connect(client, SBIG_CCD);
      return INDIGO_OK;
    }
  }
  if(!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
    printf("version = %u\n", device->version);
    printf("version = %u\n", INDIGO_VERSION_2_0);
    if(device->version >= INDIGO_VERSION_2_0) {
      printf("enable blob URL\n");
      indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_URL);
    } else {
      printf("enable blob ALSO\n");
      indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_ALSO);
    }
  }
  if(!strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
    static const char* items[] = { CCD_IMAGE_FORMAT_FITS_ITEM_NAME };
    static bool values[] = { true };
    indigo_change_switch_property(client, SBIG_CCD, CCD_IMAGE_FORMAT_PROPERTY_NAME,
      1, items, values);
  }
  return INDIGO_OK;
}

static indigo_result client_update_property(indigo_client* client,
  indigo_device* device, indigo_property* property,
  const char* message) {
  printf("prop-device = %s\n", property->device);
  printf("prop-name = %s\n", property->name);
  if(strcmp(property->device, SBIG_CCD))
    return INDIGO_OK;
  static const char* items[] = { CCD_EXPOSURE_ITEM_NAME };
  static double values[] = { 0.05 };
  if(!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
    if(indigo_get_switch(property, CONNECTION_CONNECTED_ITEM_NAME)) {
      if(!connected) {
        connected = true;
        indigo_log("connected...");
        indigo_change_number_property(client, SBIG_CCD, CCD_EXPOSURE_PROPERTY_NAME,
          1, items, values);
      }
    } else {
      if(connected) {
        indigo_log("disconnected...");
        connected = false;
      }
    }
    return INDIGO_OK;
  }
  if(!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
    /* URL blob transfer is available only in client - server setup.
       This will never be called in case of a client loading a driver. */
    if(*property->items[0].blob.url && indigo_populate_http_blob_item(&property->items[0]))
      indigo_log("image URL received (%s, %d bytes)...", property->items[0].blob.url,
        property->items[0].blob.size);

    if(property->items[0].blob.value) {
      char name[32];
      sprintf(name, "img_%02d.fits", count);
      FILE* f = fopen(name, "wb");
      fwrite(property->items[0].blob.value, property->items[0].blob.size, 1, f);
      fclose(f);
      indigo_log("image saved to %s...", name);
      /* In case we have URL BLOB transfer we need to release the blob ourselves */
      if(*property->items[0].blob.url) {
        free(property->items[0].blob.value);
        property->items[0].blob.value = NULL;
      }
    }
  }
  if(!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
    if(property->state == INDIGO_BUSY_STATE) {
      indigo_log("exposure %gs...", property->items[0].number.value);
    } else if(property->state == INDIGO_OK_STATE) {
      indigo_log("exposure done...");
      if(--count > 0) {
        indigo_change_number_property(client, SBIG_CCD, CCD_EXPOSURE_PROPERTY_NAME,
          1, items, values);
      } else {
        indigo_device_disconnect(client, SBIG_CCD);
      }
    }
    return INDIGO_OK;
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

  indigo_set_log_level(INDIGO_LOG_INFO);
  indigo_start();

  indigo_driver_entry* driver;
  indigo_attach_client(&client);
  if(indigo_load_driver("indigo_ccd_sbig", true, &driver) == INDIGO_OK) {
    while(count > 0) {
      indigo_usleep(ONE_SECOND_DELAY);
    }
  }
  indigo_remove_driver(driver);
  indigo_detach_client(&client);
  indigo_stop();
  return 0;
}
