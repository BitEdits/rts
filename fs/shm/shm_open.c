/****************************************************************************
 * fs/shm/shm_open.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "inode/inode.h"
#include "vfs/vfs.h"
#include "shm/shmfs.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int file_shm_open(FAR struct file *shm, FAR const char *name,
                         int oflags, mode_t mode)
{
  FAR struct inode *inode;
  struct inode_search_s desc;
  char fullpath[sizeof(CONFIG_FS_SHMFS_VFS_PATH) + CONFIG_NAME_MAX + 2];
  int ret;

  /* Make sure that a non-NULL name is supplied */

  if (!shm || !name)
    {
      return -EINVAL;
    }

  /* Remove any number of leading '/' */

  while (*name == '/')
    {
      name++;
    }

  /* Empty name supplied? */

  if (*name == '\0')
    {
      return -EINVAL;
    }

  /* Name too long? */

  if (strnlen(name, CONFIG_NAME_MAX + 1) > CONFIG_NAME_MAX)
    {
      return -ENAMETOOLONG;
    }

  /* Get the full path to the shm object */

  snprintf(fullpath, sizeof(fullpath),
           CONFIG_FS_SHMFS_VFS_PATH "/%s", name);

  /* Get the inode for this shm object */

  SETUP_SEARCH(&desc, fullpath, false);

  inode_lock();
  ret = inode_find(&desc);
  if (ret >= 0)
    {
      /* Something exists at this path.  Get the search results */

      inode = desc.node;

      /* Verify that the inode is an shm object */

      if (!INODE_IS_SHM(inode))
        {
          ret = -EINVAL;
          inode_release(inode);
          goto errout_with_sem;
        }

      /* It exists and is an shm object.  Check if the caller wanted to
       * create a new object with this name.
       */

      if ((oflags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
        {
          ret = -EEXIST;
          inode_release(inode);
          goto errout_with_sem;
        }

      /* If the shared memory object already exists, truncate it to
       * zero bytes.
       */

      if ((oflags & O_TRUNC) == O_TRUNC && inode->i_private != NULL)
        {
          shmfs_free_object(inode->i_private);
          inode->i_private = NULL;
          inode->i_size = 0;
        }
    }
  else
    {
      /* The shm does not exist. Were we asked to create it? */

      if ((oflags & O_CREAT) == 0)
        {
          /* The shm does not exist and O_CREAT is not set */

          ret = -ENOENT;
          goto errout_with_sem;
        }

      /* Create an inode in the pseudo-filesystem at this path */

      ret = inode_reserve(fullpath, mode, &inode);
      if (ret < 0)
        {
          goto errout_with_sem;
        }

      INODE_SET_SHM(inode);
      inode->u.i_ops = &g_shmfs_operations;
      inode->i_private = NULL;
      atomic_fetch_add(&inode->i_crefs, 1);
    }

  /* Associate the inode with a file structure */

  shm->f_oflags = oflags | O_NOFOLLOW;
  shm->f_inode = inode;

errout_with_sem:
  inode_unlock();
  RELEASE_SEARCH(&desc);
#ifdef CONFIG_FS_NOTIFY
  if (ret >= 0)
    {
      notify_open(fullpath, oflags);
    }
#endif

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int shm_open(FAR const char *name, int oflag, mode_t mode)
{
  FAR struct file *shm;
  int ret;
  int fd;

  fd = file_allocate(oflag | O_CLOEXEC, 0, &shm);
  if (fd < 0)
    {
      set_errno(-fd);
      return ERROR;
    }

  ret = file_shm_open(shm, name, oflag, mode);
  file_put(shm);
  if (ret < 0)
    {
      nx_close(fd);
      set_errno(-ret);
      return ERROR;
    }

  return fd;
}
