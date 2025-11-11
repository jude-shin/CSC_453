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

FORWARD _PROTOTYPE( int reading,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );

FORWARD _PROTOTYPE( int writing,  (int procnr, int opcode,
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

/* ============= */
/* GLOBAL STATES */
/* ============= */

/* Represents the /dev/secret device. */
PRIVATE struct device secret_device;

/* Whether the file contains a secret or not (you can only read a secret once!*/
PRIVATE int empty;

/* The number of opens the secret file has. */
/* TODO: I don't know what this is really supposed to do at the moment. */
PRIVATE int open_fds;

/* The count for the last read. */
PRIVATE size_t read_bytes;

/* The count for the last write. */
PRIVATE size_t write_bytes;

/* The buffer holding our read/write data */
PRIVATE char buffer[SECRET_SIZE];

/* The user id that has written the secret to the device last. */
PRIVATE uid_t owner;

/* ================ */
/* HELPER FUNCTIONS */
/* ================ */
PRIVATE int reading(
    int proc_nr, 
    int opcode,
    u64_t position, 
    iovec_t* iov,
    unsigned nr_req) {

  /* The return value */
  int ret;
  
  read_bytes = write_bytes - position.lo;
  if (read_bytes > iov->iov_size) {
    read_bytes = iov->iov_size;
  }

  /* Should never be calling with bytes as a negative number or 0. */
  if (read_bytes < 0) {
    empty = TRUE;
    return EFAULT;
  }

  /* Nothing is to be done. */
  if (read_bytes == 0) {
    empty = TRUE;
    return OK;
  }

  ret = sys_safecopyto(
      proc_nr,                                /* dest proc          */
      iov->iov_addr,                          /* dest buff          */
      0,                                      /* offset dest buff   */
      (vir_bytes) (buffer + position.lo),     /* virt add of src    */
      read_bytes,                             /* no. bytes to copy  */
      D);                                     /* mem segment (D)    */

  if (ret == OK) {
    /* Update the input/output vector's size. */
    iov->iov_size -= read_bytes;
  }
  
  return ret;
}

PRIVATE int writing(
    int proc_nr, 
    int opcode,
    u64_t position, 
    iovec_t* iov,
    unsigned nr_req) {

  /* The return value */
  int ret;

  write_bytes = iov->iov_size;

  /* The position is larger than the max buffer. */
  if (position.lo + write_bytes > SECRET_SIZE) {
    return ENOSPC;
  }

  /* Should never be calling with bytes as a negative number. */
  if (write_bytes < 0) {
    empty = FALSE;
    return EFAULT;
  }

  /* Nothing is to be done. */
  if (write_bytes == 0) {
    return OK;
  }

  ret = sys_safecopyfrom(
      proc_nr,                                /* src proc           */
      iov->iov_addr,                          /* src buff           */
      0,                                      /* offset src buff    */
      (vir_bytes) (buffer + position.lo),     /* virt add of src    */
      write_bytes,                            /* no. bytes to copy  */
      D);                                     /* mem segment (D)    */

  /* check the return value for this function  (I don't think this is just ok)*/
  if (ret == OK) {
    /* Update the input/output vector's size. */
    iov->iov_size -= write_bytes;

    if (position.lo + write_bytes > write_bytes) {
      write_bytes = position.lo + write_bytes;
    }

    /* Change the status of the device to "full". */
    empty = FALSE;
  }

  return ret;
}

PRIVATE char* secret_name(void) {
  #ifdef DEBUG
  printf("[debug] secret_name()\n");
  #endif 

  return "secretkeeper";
}

PRIVATE int secret_open(struct driver* d, message* m) {
  int ret;
  struct ucred u;
  int permission_flags;

  #ifdef DEBUG
  printf("[debug] secret_open()\n");
  #endif 

  printf("owner: %d\n", owner);
  
  /* bitfield that encoudes all the flags passed into open. */
  permission_flags = m->COUNT;

  /* If open(2) is called with WRITE permissions... */
  if (permission_flags & W_BIT) {
    /* Ensure the device is empty */
    if (!empty) {
      #ifdef DEBUG 
      printf("[debug] ERROR: cannot write to a full secret. \n");
      #endif

      return ENOSPC;
    }

    /* Set the owner of the process to the one who just wrote it. */
    if (getnucred(m->IO_ENDPT, &u) == -1) {
      #ifdef DEBUG 
      printf("[debug] ERROR: trying to getnucred of process.\n");
      #endif
      return EFAULT;
    }
    owner = u.uid;
  }

  /* If open(2) is called with READ permissions... */
  if (permission_flags & R_BIT) {
    /* Ensure that the device is full*/
    if (empty) {
      #ifdef DEBUG 
      printf("[debug] WARNING: trying to read from an empty secret!\n");
      #endif
      
      return EFAULT;
    }

    /* Then check to make sure that the owner is the one trying to read from it*/
    /* Get the 's uid */
    if (getnucred(m->IO_ENDPT, &u) == -1) {
      #ifdef DEBUG 
      printf("[debug] ERROR: trying to getnucred of process.\n");
      #endif
      return EFAULT;
    }

    if (u.uid != owner) {
      #ifdef DEBUG 
      printf("[debug] ERROR: YOU DON'T HAVE PERMISSIONS!!!\n");
      #endif
      return EACCES;
    }
  }

  /* Increment the open_fds count. */
  open_fds++;

  return OK;
}

PRIVATE int secret_close(struct driver* d, message* m) {
  #ifdef DEBUG
  printf("[debug] secret_close()\n");
  #endif 
  
  /* Decrement the open_fds count. */
  open_fds--;

  /* Change the status of the device to "empty" only if there is nobody 
     else writing to it. */

  return OK;
}

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

  if (ret == OK) {
    owner = grantee;
  }

  return ret;
}

PRIVATE struct device* secret_prepare(int dev) {
  #ifdef DEBUG
  printf("[debug] secret_prepare()\n");
  #endif 

  secret_device.dv_base.lo = 0;
  secret_device.dv_base.hi = 0;

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
  
  /* The return value */
  int ret;

  switch (opcode) {
    /* When cat /dev/Secret is called: READ from the device */
    case DEV_GATHER_S:
      #ifdef DEBUG 
      printf("[debug] secret_transfer() called with DEV_GATHER_S\n");
      #endif

      ret = reading(proc_nr, opcode, position, iov, nr_req);
      break;

    /* When echo "foo" > /dev/Secret is called: WRITE to the device */
    case DEV_SCATTER_S:
      #ifdef DEBUG 
      printf("[debug] secret_transfer() called with DEV_SCATTER_S\n");
      #endif

      ret = writing(proc_nr, opcode, position, iov, nr_req);
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

  ds_publish_mem("empty", &empty, sizeof(empty), DSF_OVERWRITE);
  ds_publish_mem("open_fds", &open_fds, sizeof(open_fds), DSF_OVERWRITE);
  ds_publish_mem("read_bytes", &read_bytes, sizeof(read_bytes), DSF_OVERWRITE);
  ds_publish_mem("write_bytes", &write_bytes, sizeof(write_bytes), DSF_OVERWRITE);
  ds_publish_mem("buffer", &buffer, sizeof(buffer), DSF_OVERWRITE);

  return OK;
}

PRIVATE int lu_state_restore(void) {
  /* TODO: make sure these are updated with whatever global states you 
     have before SUBMITTING */

  size_t s;

  #ifdef DEBUG
  printf("[debug] lu_state_restore()\n");
  #endif 

  s = sizeof(empty);
  ds_retrieve_mem("empty", (char*)&empty, &s);
  ds_delete_mem("empty");

  s = sizeof(open_fds);
  ds_retrieve_mem("open_fds", (char*)&open_fds, &s);
  ds_delete_mem("open_fds");

  s = sizeof(read_bytes);
  ds_retrieve_mem("read_bytes", (char*)&read_bytes, &s);
  ds_delete_mem("read_bytes");

  s = sizeof(write_bytes);
  ds_retrieve_mem("write_bytes", (char*)&write_bytes, &s);
  ds_delete_mem("write_bytes");

  s = sizeof(buffer);
  ds_retrieve_mem("buffer", (char*)&buffer, &s);
  ds_delete_mem("buffer");

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
  char* name;
  int do_announce_driver, i;

  /* Initialize the secret driver. */
  name = secret_name();

  /* If you want to announce the driver. */
  do_announce_driver = TRUE;

  #ifdef DEBUG
  printf("[debug] sef_cb_init()\n");
  #endif 

  /* Start out with no secret written. */
  empty = TRUE; 
  
  /* Start with nobody accessing the device. */
  open_fds = 0;
  
  /* Set these read/write bytes to 0 for sanity. */
  read_bytes = 0;
  write_bytes = 0;

  /* Initialize all the buffer to NULL. */
  buffer[SECRET_SIZE];
  for (i=0; i < SECRET_SIZE; i++) {
    buffer[i] = '\0';
  }

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
