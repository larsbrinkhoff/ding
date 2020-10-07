#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <hidapi/hidapi.h>

#define MICROCHIP_VID 0x04d8
#define MCP2200_PID 0x00dd

/* Complain and exit. */
static void die (hid_device *device, const char *message)
{
  fprintf (stderr, "Error: %s", message);
  if (device)
    fprintf (stderr, " (%ls)", hid_error (device));
  fputc ('\n', stderr);
  exit (1);
}

/* Do the HID command/responce dance, with error checking. */
static void hid (hid_device *device, unsigned char *buf)
{
  unsigned char cmd = buf[0];
  int n;

  /* Write data to device. */
  n = hid_write (device, buf, 64);
  if (n != 64)
    die (device, "hid: write");

  /* Get response from device. */
  memset (buf, 0, 64);
  n = hid_read (device, buf, 64);
  if (n == -1)
    die (device, "hid: read error");
  if (n != 64)
    die (device, "hid: read size");
  if (buf[0] != cmd)
    die (NULL, "hid: wrong response");
  if (buf[1] != 0x00)
    die (NULL, "hid: error code");
}

/* Set GPIO configuration.  Writes to flash. */
static void configure (hid_device *device, int pin)
{
  unsigned char settings[64];

  memset (settings, 0, 64);
  settings[0] = 0xB1; //Write flash data.
  settings[1] = 0x01; //Write GP settings.
  settings[2 + pin] = 0x00; //GP<pin> = GPIO, output, low.
  hid (device, settings);

  memset (settings, 0, 64);
  settings[0] = 0x60; //Set SRAM settings.
  settings[7] = 0x01; //Alter GP designation.
  settings[8 + pin] = 0x00; //GP<pin> = GPIO, output, low.
  hid (device, settings);
}

/* Set GPIO output value. */
static void frob (hid_device *device, int pin, int value)
{
  unsigned char data[64];

  memset (data, 0, 64);
  data[0] = 0x50; //Set GPIO output values.
  data[4 * pin + 2] = 0x01;
  data[4 * pin + 3] = value;
  hid (device, data);
}

/* Make GPIO go high for a specified time. */
static void ding (hid_device *device, int microseconds)
{
  frob (device, 0, 1);
  usleep (microseconds);
  frob (device, 0, 0);
  usleep (100000);
}

int main (void)
{
  hid_device *device;
  int us = 5000;

  if (hid_init () == -1)
    die (NULL, "init");

  device = hid_open (MICROCHIP_VID, MCP2200_PID, NULL);
  if (device == NULL)
    die (NULL, "open");

  configure (device, 0);
  frob (device, 0, 0);

  for (;;) {
    int c = getchar ();
    if (c == EOF)
      exit (0);
    if (c != 0)
      ding (device, us);
  }
}
