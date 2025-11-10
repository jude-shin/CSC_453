#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include "secret.h"

/* TODO: ask
   1) Why do we need to make sure there is a buffer external to the sys_safe...
   2) Is are there more manpages on iov things?
   3) If transfer() is called with a message larger than the max_size, or called
    with negative or zero size, is there a pareticular error to raise? The hello
    driver went along without doing anything, just returning OK
   4) Should writing to a file that already has a secret raise an error, or just return OK?
      (example gives "cannot create /dev/Secret: no space left on device")
*/

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

/* Whether the file contains a secret or not (you can only read a secret once!
   if 0, then the file has nothing in it, anything else means there is a
   secret*/
PRIVATE int empty;


PRIVATE char* secret_name(void) {
  #ifdef DEBUG
  printf("[debug] secret_name()\n");
  #endif 

  return "secret";
}

PRIVATE int secret_open(struct driver* d, message* m) {
  #ifdef DEBUG
  printf("[debug] secret_open(). Called %d time(s).\n", ++open_counter);
  #endif 

  return OK;
}

PRIVATE int secret_close(struct driver* d, message* m) {
  #ifdef DEBUG
  printf("[debug] secret_close()\n");
  #endif 

  return OK;
}

/* TODO: I dont know what this does. I think that this should handle the
   permissions before anything is opened, closed, or read? */
PRIVATE int secret_ioctl(struct driver* d, message* m) {
  int ret;
  uid_t grantee; /* the uid of teh new owner of the secret. */

  #ifdef DEBUG
  printf("[debug] secret_ioctl()\n");
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
  printf("[debug] secret_secret_prepare()\n");
  #endif 

  secret_device.dv_base.lo = 0;
  secret_device.dv_base.hi = 0;

  /* TODO: should this be the size of the buffer macro? */
  /* secret_device.dv_size.lo = strlen(GATHER_MSG); */
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
  
  /* Then number of overall bytes written or read */
  size_t bytes;

  /* The buffer holding our read/write data */
  char buffer[SECRET_SIZE];

  /* The number of bytes read or written so far */
  size_t count;
   
  /* The return value */
  int ret;

  #ifdef DEBUG 
  printf("[debug] secret_transfer() ");
  #endif

  switch (opcode) {
    /* When cat /dev/Secret is called: READ from the device */
    case DEV_GATHER_S:
      #ifdef DEBUG 
      printf("called with DEV_GATHER_S\n");
      #endif

      /* If there is nothing to be read, then nothing is to be done. */
      if (!empty) {
        #ifdef DEBUG 
        printf("[debug] WARNING: trying to read from an empty secret!\n");
        #endif

        return OK;
      }

      /* TODO: check the permissions to see if you were the one who wrote to =
         the full device. */ 
      if (0) {
        /* foo */
      }

      bytes = count - position.lo;
      if (bytes > iov->iov_size) {
        bytes = iov->iov_size;
      }

      if (bytes <= 0) {
        /* TODO: hello just went along without doing anything...
           might be based to raise an error. */
        /* Should never be calling with bytes as a negative number or 0. */
        return OK;
      }

      ret = sys_safecopyto(
          proc_nr,                                /* dest proc          */
          iov->iov_addr,                          /* dest buff          */
          0,                                      /* offset dest buff   */
          (vir_bytes) (buffer + position.lo),     /* virt add of src    */
          bytes,                                  /* no. bytes to copy  */
          D);                                     /* mem segment (D)    */

      if (ret == OK) {
        /* Update the input/output vector's size. */
        iov->iov_size -= bytes;

        /* Change the status of the device to "full". */
        empty = TRUE;
      }
      else {
        /* Something went wrong? how can I handle this? */
      }

      break;

    /* When echo "foo" > /dev/Secret is called: WRITE to the device */
    case DEV_SCATTER_S:
      #ifdef DEBUG 
      printf("called with DEV_SCATTER_S\n");
      #endif

      /* Return an error if you are trying to write a full secret. */
      if (!empty) {
        #ifdef DEBUG 
        printf("[debug] ERROR: cannot write to a full secret. \n");
        #endif

        return ENOSPC;
      }

      bytes = iov->iov_size;

      if (position.lo + bytes > SECRET_SIZE) {
        /* TODO: Raise an error... you are trying to overflow a buffer! */
      }

      if (bytes <= 0) {
        /* TODO: hello just went along without doing anything...
           might be based to raise an error. */
        /* Should never be calling with bytes as a negative number or 0. */
        return OK;
      }

      ret = sys_safecopyfrom(
          proc_nr,                                /* src proc           */
          iov->iov_addr,                          /* src buff           */
          0,                                      /* offset src buff    */
          (vir_bytes) (buffer + position.lo),     /* virt add of src    */
          bytes,                                  /* no. bytes to copy  */
          D);                                     /* mem segment (D)    */

      if (ret == OK) {
        /* Update the input/output vector's size. */
        iov->iov_size -= bytes;
        
        if (position.lo + bytes > count) {
          count = position.lo + bytes;
        }

        /* Change the status of the device to "full". */
        empty = FALSE;
      }
      else {
        /* Something went wrong? how can I handle this? */
      }

      break;

    default:
      return EINVAL;
  }

  return ret;
}

PRIVATE void secret_geometry(struct partition* entry) {
  #ifdef DEBUG
  printf("[debug] secret_geometry()\n");
  #endif 

  entry->cylinders = 0;
  entry->heads     = 0;
  entry->sectors   = 0;
}

PRIVATE int sef_cb_lu_state_save(int state) {
  /* TODO: make sure these are updated with whatever global states you 
     have before SUBMITTING */

  #ifdef DEBUG
  printf("[debug] sef_cb_lu_state_save()\n");
  #endif 

  /* remove this publish for open_counter */ 
  ds_publish_mem("open_counter", &open_counter, sizeof(open_counter), DSF_OVERWRITE);
  /* ds_publish_mem("last_read", &last_read, sizeof(last_read), DSF_OVERWRITE);
  ds_publish_mem("last_write", &last_write, sizeof(last_write), DSF_OVERWRITE); */
  ds_publish_mem("empty", &empty, sizeof(empty), DSF_OVERWRITE);

  return OK;
}

PRIVATE int lu_state_restore(void) {
  /* TODO: make sure these are updated with whatever global states you 
     have before SUBMITTING */

  size_t s;

  #ifdef DEBUG
  printf("[debug] lu_state_restore()\n");
  #endif 

  /* Restore the state. */
  s = sizeof(open_counter);
  ds_retrieve_mem("open_counter", (char*)&open_counter, &s);
  ds_delete_mem("open_counter");
  
  /* s = sizeof(last_read);
  ds_retrieve_mem("last_read", (char*)&last_read, &s);
  ds_delete_mem("last_read");

  s = sizeof(last_write);
  ds_retrieve_mem("last_write", (char*)&last_write, &s);
  ds_delete_mem("last_write"); */

  s = sizeof(empty);
  ds_retrieve_mem("empty", (char*)&empty, &s);
  ds_delete_mem("empty");

  return OK;
}

PRIVATE void sef_local_startup() {
  #ifdef DEBUG
  printf("[debug] sef_local_startup()\n");
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
  printf("[debug] sef_cb_init()\n");
  #endif 


  open_counter = 0;
  switch(type) {
    case SEF_INIT_FRESH:
      #ifdef DEBUG
      printf("[debug] %s driver sef_cp_init() called with SEF_INIT_FRESH\n", name);
      #endif 

      break;

    case SEF_INIT_LU:
      /* Restore the state. */
      lu_state_restore();
      do_announce_driver = FALSE;

      #ifdef DEBUG
      printf("[debug] %s driver sef_cp_init() called with SEF_INIT_LU\n", name);
      #endif 

      break;

    case SEF_INIT_RESTART:
      #ifdef DEBUG
      printf("[debug] %s driver sef_cp_init() called with SEF_INIT_RESTART\n", name);
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
