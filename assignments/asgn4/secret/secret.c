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

/* Represents the /dev/secret device. */
PRIVATE struct device secret_device;

/* ============= */
/* GLOBAL STATES */
/* ============= */
/* Whether the file contains a secret or not (you can only read a secret once!*/
PRIVATE int been_read;

/* The number of opens the secret file has. */
PRIVATE int open_fds;

/* Whether there is a secret being held or not. */
PRIVATE int empty;

/* The count for the last read. */
PRIVATE size_t r_bytes;

/* The count for the last write. */
PRIVATE size_t w_bytes;

/* The buffer holding our read/write data */
PRIVATE char buffer[SECRET_SIZE];

/* The user id that has written the secret to the device last. */
PRIVATE uid_t owner;

/* ================ */
/* HELPER FUNCTIONS */
/* ================ */
/* Reads the secret from the device.
   @param proc_nr Process Number.
   @param opcode Operation Code.
   @param position Current read position.
   @param iov Input Output Vector.
   @param nr_req Number of requests.
   @return status of this function call */
PRIVATE int reading(
    int proc_nr, 
    int opcode,
    u64_t position, 
    iovec_t* iov,
    unsigned nr_req) {

  /* The return value */
  int ret;
  size_t bytes_to_read;

  /* If we are trying to read past what has been written OVERALL, don't fail, 
     just don't return an OK status. */
  if (position.lo >= w_bytes) {
    return OK;
  }

  /* Don't read more than the caller requested. */
  bytes_to_read = w_bytes - position.lo;
  if (bytes_to_read > iov->iov_size) {
    bytes_to_read = iov->iov_size;
  }

  /* Should never be calling with bytes as a negative number or 0. */
  if (bytes_to_read <= 0) {
    return OK;
  }

  ret = sys_safecopyto(
      proc_nr,                                /* dest proc          */
      iov->iov_addr,                          /* dest buff          */
      0,                                      /* offset dest buff   */
      (vir_bytes) (buffer + position.lo),     /* virt add of src    */
      bytes_to_read,                          /* no. bytes to copy  */
      D);                                     /* mem segment (D)    */

  if (ret == OK) {
    /* Update the input/output vector's size. */
    iov->iov_size -= bytes_to_read;
  }
  
  return ret;
}

/* Reads the secret from the device.
   @param proc_nr Process Number.
   @param opcode Operation Code.
   @param position Current write position.
   @param iov Input Output Vector.
   @param nr_req Number of requests.
   @return status of this function call */
PRIVATE int writing(
    int proc_nr, 
    int opcode,
    u64_t position, 
    iovec_t* iov,
    unsigned nr_req) {

  /* The return value */
  int ret;
  size_t bytes_to_write;
  
  /* Make sure you are not writing past the buffer. */
  if (position.lo >= SECRET_SIZE) {
    return ENOSPC;
  }

  bytes_to_write = iov->iov_size;

  /* The position is larger than the max buffer. */
  if (position.lo + bytes_to_write > SECRET_SIZE) {
      bytes_to_write = SECRET_SIZE - position.lo;
  }

  /* Should never be calling with bytes as a negative number. */
  if (bytes_to_write < 0) {
    return EFAULT;
  }

  /* Nothing is to be done. */
  if (bytes_to_write == 0) {
    return OK;
  }

  ret = sys_safecopyfrom(
      proc_nr,                                /* src proc           */
      iov->iov_addr,                          /* src buff           */
      0,                                      /* offset src buff    */
      (vir_bytes) (buffer + position.lo),     /* virt add of src    */
      bytes_to_write,                         /* no. bytes to copy  */
      D);                                     /* mem segment (D)    */

  /* check the return value for this function  (I don't think this is just ok)*/
  if (ret == OK) {
    /* Update the input/output vector's size. */
    iov->iov_size -= bytes_to_write;

    if (position.lo + bytes_to_write > w_bytes) {
      w_bytes = position.lo + bytes_to_write;
    }
  }

  return ret;
}

/* Gets the name of the driver
   @param void.
   @return char* the name of the driver. */
PRIVATE char* secret_name(void) {
  #ifdef DEBUG
  printf("[debug] secret_name()\n");
  #endif 

  return DRIVER_NAME; 
}

/* Callback for when the device is opened. Checks a bunch of conditions for if
   the device can be read or not.
   @param d The driver that this callback if for.
   @param m Some information that comes with the callback.
   @return status of this funciton call. */
PRIVATE int secret_open(struct driver* d, message* m) {
  int w, r;
  struct ucred u;
  int permission_flags;

  #ifdef DEBUG
  printf("[debug] secret_open()\n");
  #endif 

  /* bitfield that encoudes all the flags passed into open. */
  permission_flags = m->COUNT;
  w = (permission_flags & W_BIT);
  r = (permission_flags & R_BIT);

  /* Device can only have one secret at a time.*/
  /* If empty */
  if (empty) {
    /* If open(2) is called to exclusively write */
    if (w && !r) {
      /* Mark this to be ready to read. */
      been_read = FALSE;

      /* Mark this as having a full secret. */
      empty = FALSE;
      
      /* Set the owner of the process to the one who just wrote it. */
      if (getnucred(m->IO_ENDPT, &u) == -1) {
        #ifdef DEBUG 
        printf("[debug] ERROR: trying to getnucred of process.\n");
        #endif
        return ENOSPC;
      }
      owner = u.uid;
    }
    /* If open(2) is called to exclusively read */
    else if (r && !w) {
      open_fds++;
      been_read = TRUE;
      return OK;
    }
    /* If we reached this point then we are trying to access using a bad
       combination of read, write, or access permissions */
    else {
      return EACCES;
    }
  }
  /* If full */
  else {
    /* you may not open for writing once it is holding a secret (full,
       aka, not empty, aka !empty) */
    /* If open(2) is called to exclusively write */
    if (w && !r) {
      return ENOSPC;
    }
    /* If open(2) is called to exclusively read */
    else if (r && !w) {
      /* Then check to make sure that the secret's owner is the reader */
      /* Get the 's uid */
      if (getnucred(m->IO_ENDPT, &u) == -1) {
        #ifdef DEBUG 
        printf("[debug] ERROR: trying to getnucred of process.\n");
        #endif
        return ENOSPC;
      }

      if (u.uid != owner) {
        #ifdef DEBUG 
        printf("[debug] ERROR: YOU DON'T HAVE PERMISSIONS!!!\n");
        #endif
        return EACCES;
      }

      /* Indicate that we have opened the file... Whether we actually read
         any of the contents is up to the programmer, but the secret has been
         exposed. */
      been_read = TRUE;
    }
    /* If we reached this point then we are trying to access using a bad

       combination of read, write, or access permissions */
    else {
      return EACCES;
    }
  }

  /* Increment the open_fds count. */
  open_fds++;

  return OK;
}

/* Callback for when the device is closed. Notes if the device is full or not.
   @param d The driver that this callback if for.
   @param m Some information that comes with the callback.
   @return status of this funciton call. */
PRIVATE int secret_close(struct driver* d, message* m) {
  #ifdef DEBUG
  printf("[debug] secret_close()\n");
  #endif 

  /* Decrement the open_fds count. */
  open_fds--;

  if (open_fds == 0 && been_read) {
    empty = TRUE;
  }
  
  return OK;
}

/* Callback for when permission is transferred to someone else.
   @param d The driver that this callback if for.
   @param  m Some information that comes with the callback.
   @return the status of this funciton call. */
PRIVATE int secret_ioctl(struct driver* d, message* m) {
  int ret;
  uid_t grantee; /* the uid of the new owner of the secret. */

  #ifdef DEBUG
  printf("[debug] secret_ioctl()\n");
  #endif 
  
  /* The onlu request that should be allowed is SSGRANT. */
  if (m->REQUEST != SSGRANT) {
    return ENOTTY;
  }

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

/* TODO: comments*/
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

/* TODO: comments*/
PRIVATE int secret_transfer(
    int proc_nr, 
    int opcode,
    u64_t position, 
    iovec_t* iov,
    unsigned nr_req) {
  
  /* The return value */
  int ret;

  switch (opcode) {
    /* READ from the device: ex) When cat /dev/Secret is called. */
    case DEV_GATHER_S:
      #ifdef DEBUG 
      printf("[debug] secret_transfer() called with DEV_GATHER_S\n");
      #endif
  
      /* Try to read the data from the device. 
         Any errors in this function will be bubbled up to the caller. */
      ret = reading(proc_nr, opcode, position, iov, nr_req);
      break;

    /* WRITE to the device: ex) When echo "foo" > /dev/Secret is called. */
    case DEV_SCATTER_S:
      #ifdef DEBUG 
      printf("[debug] secret_transfer() called with DEV_SCATTER_S\n");
      #endif
      
      /* Try to write the data to the device. 
         Any errors in this function will be bubbled up to the caller. */
      ret = writing(proc_nr, opcode, position, iov, nr_req);
      break;

    default:
      return EINVAL;
  }

  return ret;
}

/* Reports the geometry of the devicd back to the filesystem. 
   @param entry the struct that will hold all of the information.
   @return void. */
PRIVATE void secret_geometry(struct partition* entry) {
  #ifdef DEBUG
  printf("[debug] secret_geometry()\n");
  #endif 

  entry->cylinders = 0;
  entry->heads     = 0;
  entry->sectors   = 0;
}

/* Saves the global states in this driver. 
   @param state the state of the driver.
   @return  status of the function call. */
PRIVATE int sef_cb_lu_state_save(int state) {
  #ifdef DEBUG
  printf("[debug] sef_cb_lu_state_save()\n");
  #endif 

  ds_publish_mem("been_read", &been_read, sizeof(been_read), DSF_OVERWRITE);
  ds_publish_mem("empty", &empty, sizeof(empty), DSF_OVERWRITE);
  ds_publish_mem("open_fds", &open_fds, sizeof(open_fds), DSF_OVERWRITE);
  ds_publish_mem("r_bytes", &r_bytes, sizeof(r_bytes), DSF_OVERWRITE);
  ds_publish_mem("w_bytes", &w_bytes, sizeof(w_bytes), DSF_OVERWRITE);
  ds_publish_mem("buffer", &buffer, sizeof(buffer), DSF_OVERWRITE);
  ds_publish_mem("owner", &owner, sizeof(owner), DSF_OVERWRITE);

  return OK;
}

/* Restores the global states in this driver. 
   @param void.
   @return status of the function call. */
PRIVATE int lu_state_restore(void) {
  size_t s;

  #ifdef DEBUG
  printf("[debug] lu_state_restore()\n");
  #endif 

  s = sizeof(been_read);
  ds_retrieve_mem("been_read", (char*)&been_read, &s);
  ds_delete_mem("been_read");

  s = sizeof(empty);
  ds_retrieve_mem("empty", (char*)&empty, &s);
  ds_delete_mem("empty");

  s = sizeof(open_fds);
  ds_retrieve_mem("open_fds", (char*)&open_fds, &s);
  ds_delete_mem("open_fds");

  s = sizeof(r_bytes);
  ds_retrieve_mem("r_bytes", (char*)&r_bytes, &s);
  ds_delete_mem("r_bytes");

  s = sizeof(w_bytes);
  ds_retrieve_mem("w_bytes", (char*)&w_bytes, &s);
  ds_delete_mem("w_bytes");

  s = sizeof(buffer);
  ds_retrieve_mem("buffer", (char*)&buffer, &s);
  ds_delete_mem("buffer");

  s = sizeof(owner);
  ds_retrieve_mem("owner", (char*)&owner, &s);
  ds_delete_mem("owner");

  return OK;
}

/* Restores the global states in this driver. 
   @param void.
   @return status of the function call. */
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

/* Restores the global states in this driver. 
   @param void.
   @return status of the function call. */
PRIVATE int sef_cb_init(int type, sef_init_info_t *info) {
  int do_announce_driver, i;

  /* If you want to announce the driver. */
  do_announce_driver = TRUE;

  #ifdef DEBUG
  printf("[debug] sef_cb_init() ");
  #endif 

  /* Start out with no secret written. */
  been_read = FALSE; 
  empty = TRUE; 
  
  /* Start with nobody accessing the device. */
  open_fds = 0;
  
  /* Set these read/write bytes to 0 for sanity. */
  r_bytes = 0;
  w_bytes = 0;

  /* Initialize all the buffer to NULL. */
  for (i=0; i < SECRET_SIZE; i++) {
    buffer[i] = '\0';
  }
  
  /* Initialize the owner to be -1... Nobody has written yet, so there is
     no reason to set this to a particular value. */
  owner = -1;

  switch(type) {
    case SEF_INIT_FRESH:
      #ifdef DEBUG
      printf("called with SEF_INIT_FRESH\n");
      #endif 

      break;

    case SEF_INIT_LU:
      /* Restore the state. */
      lu_state_restore();
      do_announce_driver = FALSE;

      #ifdef DEBUG
      printf("called with SEF_INIT_LU\n");
      #endif 

      break;

    case SEF_INIT_RESTART:
      #ifdef DEBUG
      printf("called with SEF_INIT_RESTART\n");
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

/* TODO: comments*/
PUBLIC int main(int argc, char *argv) {
    /* Perform initialization.  */
    sef_local_startup();

    /* Run the main loop. */
    driver_task(&secret_tab, DRIVER_STD);

    return OK;
}
