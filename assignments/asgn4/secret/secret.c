#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include "secret.h"

/* Function prototypes for the secret driver. */
FORWARD _PROTOTYPE( char * secret_name,   (void) );
FORWARD _PROTOTYPE( int secret_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_ioctl,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * secret_prepare, (int device) );
FORWARD _PROTOTYPE( int secret_transfer,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );
FORWARD _PROTOTYPE( void secret_geometry, (struct partition *entry) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Entry points to the secret driver. */
PRIVATE struct driver secret_tab = {
  secret_name,
  secret_open,
  secret_close,
  secret_ioctl, /* This has been replaced! (was nop_ioctl) */
  secret_prepare,
  secret_transfer,
  nop_cleanup,
  secret_geometry,
  nop_alarm,
  nop_cancel,
  nop_select,
  nop_ioctl,
  do_nop,
};

/* Represents the /dev/secret device. */
PRIVATE struct device secret_device;

/* State variable to count the number of times the device has been opened. */
PRIVATE int open_counter;

/* The location of where the driver last read from. */
/* TODO: I don't know what datatype this should be... */
PRIVATE int *last_read;

/* The location of where the driver last wrote from. */
/* TODO: I don't know what datatype this should be... */
PRIVATE int *last_write;

/* Whether the file is readable or not (you can only read a secret once! */
PRIVATE int readable;


PRIVATE char* secret_name(void) {
  #ifdef DEBUG
  printf("secret_name()\n");
  #endif 

  return "secret";
}

PRIVATE int secret_open(struct driver* d, message* m) {
  #ifdef DEBUG
  printf("secret_open(). Called %d time(s).\n", ++open_counter);
  #endif 

  return OK;
}

PRIVATE int secret_close(struct driver* d, message* m) {
  #ifdef DEBUG
  printf("secret_close()\n");
  #endif 

  return OK;
}

/* TODO: I dont know what this does. I think that this should handle the
   permissions before anything is opened, closed, or read? */
PRIVATE int secret_ioctl(struct driver* d, message* m) {
  int ret;
  uid_t grantee; /* the uid of teh new owner of the secret. */

  #ifdef DEBUG
  printf("secret_ioctl()\n");
  #endif 

  ret = sys_safecopyfrom(
      m->IO_ENDPT,
      (vir_bytes)m->IO_GRANT,
      0,
      (vir_bytes)&grantee,
      sizeof(grantee),
      D);

  /* TODO: I think we need to do some more things... */
  /* TODO: am I supposed to just return OK, or should I return ret? */
  return ret;
}

PRIVATE struct device* secret_prepare(int dev) {
  #ifdef DEBUG
  printf("secret_secret_prepare()\n");
  #endif 

  secret_device.dv_base.lo = 0;
  secret_device.dv_base.hi = 0;

  /* TODO: should this be the size of the buffer macro? */
  /* secret_device.dv_size.lo = strlen(HELLO_OG); */
  secret_device.dv_size.lo = SECRET_SIZE;
  secret_device.dv_size.hi = 0;
  return &secret_device;
}

PRIVATE int secret_transfer(
    int proc_nr, 
    int opcode,
    u64_t position, 
    iovec_t* iov,
    unsigned nr_req) {

  int bytes, ret;

  #ifdef DEBUG
  printf("secret_transfer()\n");
  #endif 

  switch (opcode) {
    /* reading */
    case DEV_GATHER_S:
      /* TODO: Get the size of the input? Or it should just be the max
         buffer size?*/
      bytes = strlen(HELLO_OG) - position.lo < iov->iov_size ?
        strlen(HELLO_OG) - position.lo : iov->iov_size;
      if (bytes <= 0) {
        return OK;
      }

      ret = sys_safecopyto(
          proc_nr,                                /* src proc           */
          iov->iov_addr,                          /* src buff           */
          0,                                      /* offset dest buff   */
          (vir_bytes) (HELLO_OG + position.lo),   /* virt add of dest   */
          bytes,                                  /* bytes to copy      */
          D);                                     /* mem segment (D)    */

      iov->iov_size -= bytes;
      break;

      /* TODO: I added this case for when something is transferring data
        into the device*/
      /* writing */
    case DEV_SCATTER_S:
      /* TODO: Get the size of the input? Or it should just be the max buffer size?*/
      bytes = strlen(HELLO_IN) - position.lo < iov->iov_size ?
        strlen(HELLO_IN) - position.lo : iov->iov_size;
      if (bytes <= 0) {
        return OK;
      }

      /* TODO: these are not the paremeters... look up safecopyfrom */
      ret = sys_safecopyfrom(
          proc_nr,                                /* dest proc          */
          iov->iov_addr,                          /* dest buff          */
          0,                                      /* offset src buff    */
          (vir_bytes) (HELLO_IN + position.lo),   /* virt add of src    */
          bytes,                                  /* bytes to copy      */
          D);                                     /* mem segment (D)    */

      iov->iov_size -= bytes;
      break;

    default:
      return EINVAL;
  }

  return ret;
}

PRIVATE void secret_geometry(struct partition* entry) {
  #ifdef DEBUG
  printf("secret_geometry()\n");
  #endif 

  entry->cylinders = 0;
  entry->heads     = 0;
  entry->sectors   = 0;
}

PRIVATE int sef_cb_lu_state_save(int state) {
  /* TODO: make sure these are updated with whatever global states you 
     have before SUBMITTING */

  #ifdef DEBUG
  printf("sef_cb_lu_state_save()\n");
  #endif 

  /* remove this publish for open_counter */ 
  ds_publish_mem("open_counter", &open_counter, sizeof(open_counter), DSF_OVERWRITE);
  ds_publish_mem("last_read", &last_read, sizeof(last_read), DSF_OVERWRITE);
  ds_publish_mem("last_write", &last_write, sizeof(last_write), DSF_OVERWRITE);
  ds_publish_mem("readable", &readable, sizeof(readable), DSF_OVERWRITE);

  return OK;
}

PRIVATE int lu_state_restore(void) {
  /* TODO: make sure these are updated with whatever global states you 
     have before SUBMITTING */

  size_t s;

  #ifdef DEBUG
  printf("lu_state_restore()\n");
  #endif 

  /* TODO: switch from **_u32 to **_mem? like the manual says */

  /* Restore the state. */
  s = sizeof(open_counter);
  ds_retrieve_mem("open_counter", (char*)&open_counter, &s);
  ds_delete_mem("open_counter");
  
  s = sizeof(last_read);
  ds_retrieve_mem("last_read", (char*)&last_read, &s);
  ds_delete_mem("last_read");

  s = sizeof(last_write);
  ds_retrieve_mem("last_write", (char*)&last_write, &s);
  ds_delete_mem("last_write");

  s = sizeof(readable);
  ds_retrieve_mem("readable", (char*)&readable, &s);
  ds_delete_mem("redable");

  return OK;
}

PRIVATE void sef_local_startup() {
  #ifdef DEBUG
  printf("sef_local_startup()\n");
  #endif 

  /* Register init callbacks. Use the same function for all event types */
  sef_setcb_init_fresh(sef_cb_init);
  sef_setcb_init_lu(sef_cb_init);
  sef_setcb_init_restart(sef_cb_init);

  /* Register live update callbacks.  */
  /* - Agree to update immediately when LU is requested in a valid state. */
  sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
  /* - Support live update starting from any standard state. */
  sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
  /* - Register a custom routine to save the state. */
  sef_setcb_lu_state_save(sef_cb_lu_state_save);

  /* Let SEF perform startup. */
  sef_startup();
}

PRIVATE int sef_cb_init(int type, sef_init_info_t *info) {
  /* Initialize the secret driver. */
  char* name = secret_name();
  int do_announce_driver = TRUE;

  #ifdef DEBUG
  printf("sef_cb_init()\n");
  #endif 


  open_counter = 0;
  switch(type) {
    case SEF_INIT_FRESH:
      #ifdef DEBUG
      printf("%s driver sef_cp_init() called with SEF_INIT_FRESH\n", name);
      #endif 

      break;

    case SEF_INIT_LU:
      /* Restore the state. */
      lu_state_restore();
      do_announce_driver = FALSE;

      #ifdef DEBUG
      printf("%s driver sef_cp_init() called with SEF_INIT_LU\n", name);
      #endif 

      break;

    case SEF_INIT_RESTART:
      #ifdef DEBUG
      printf("%s driver sef_cp_init() called with SEF_INIT_RESTART\n", name);
      #endif 

      break;
  }

  /* Announce we are up when necessary. */
  if (do_announce_driver) {
    driver_announce();
  }

  /* Initialization completed successfully. */
  return OK;
}

PUBLIC int main(int argc, char *argv) {
    /* Perform initialization.  */
    sef_local_startup();

    /* Run the main loop. */
    driver_task(&secret_tab, DRIVER_STD);
    return OK;
}
