/**
 *  @title      :   sr_container_helpers.c
 *  @author     :   Shabir Abdul Samadh (shabirmean@cs.mcgill.ca)
 *  @purpose    :   COMP310/ECSE427 Operating Systems (Assingment 3) - Phase 2
 *  @description:   Fill in the following methods
 *                      switch_child_root()
 *                      setup_child_capabilities()
 *                      setup_syscall_filters()
 *                  No other changes are needed to this file
*/
#include "sr_container.h"

int switch_child_root(const char *new_root, const char *put_old)
{
    /**
     *  ------------------------ TODO ------------------------
     *  Simply use the "pivot_root()" system call to switch child's root to the new root
     *  ------------------------------------------------------
     * */ 
    return 0;
}

/**
 * ------------------------ TODO ------------------------
 *      This method is for altering child capabilities
 *      Complete this method as described in the assingment handout to disable a list of capabilities
 * ------------------------------------------------------
 **/ 
int setup_child_capabilities()
{
    /**
     *  Follow these steps to check if you successfully set the capabilities
     *      Copy the binary 'capsh' found inside the [/sbin] folder of the docker container
     *          into the [/sbin] folder of the 'rootfs' you downloaded to run containers
     *              cp /sbin/capsh $ROOTFS/sbin/
     *  
     *  Now if you run 'capsh --print' without this method implemented the output for [Bounding set]
     *      will indicate many capabilities. But after properly implementing this method if you run the same
     *      command inside your container you will see a smaller set of capabilities for [Bounding set]
     **/
    return 0;
}

/**
 * ------------------------ TODO ------------------------
 *      This method is for restricting system_calls from within the container
 *      Complete this method as described in the assingment handout to restrict a list of system calls
 * ------------------------------------------------------
 **/ 
int setup_syscall_filters()
{
    return 0;
}

int setup_child_mounts(struct child_config *config)
{
    fprintf(stderr, "####### > child process remounting root with MS_PRIVATE...");
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL))
    {
        fprintf(stderr, "invocation to mount() failed! %m\n");
        return -1;
    }
    fprintf(stderr, "successfull remount.\n");

    fprintf(stderr, "####### > a bind mount is done on a newly created tmp directory...");
    char mount_dir[] = "/tmp/tmp.XXXXXX";
    if (!mkdtemp(mount_dir))
    {
        fprintf(stderr, "could not make a directory for mounting!\n");
        return -1;
    }

    if (mount(config->mount_dir, mount_dir, NULL, MS_BIND | MS_PRIVATE, NULL))
    {
        fprintf(stderr, "attempt to bind mount crashed!\n");
        return -1;
    }

    char inner_mount_dir[] = "/tmp/tmp.XXXXXX/oldroot.XXXXXX";
    memcpy(inner_mount_dir, mount_dir, sizeof(mount_dir) - 1);
    if (!mkdtemp(inner_mount_dir))
    {
        fprintf(stderr, "attempt to make inner directory failed!\n");
        return -1;
    }

    fprintf(stderr, "####### > changing root with pivot_root...");
    if (switch_child_root(mount_dir, inner_mount_dir))
    {
        fprintf(stderr, "failed!\n");
        return -1;
    }
    fprintf(stderr, "inner mount done.\n");

    char *old_root_dir = basename(inner_mount_dir);
    char old_root[sizeof(inner_mount_dir) + 1] = {"/"};
    strcpy(&old_root[1], old_root_dir);

    fprintf(stderr, "####### > unmounting old_root %s...", old_root);
    if (chdir("/"))
    {
        fprintf(stderr, "could not change directory with chdir()! %m\n");
        return -1;
    }
    if (umount2(old_root, MNT_DETACH))
    {
        fprintf(stderr, "attempt to umount() failed! %m\n");
        return -1;
    }
    if (rmdir(old_root))
    {
        fprintf(stderr, "invocation to rmdir() failed! %m\n");
        return -1;
    }
    
    if (mount(NULL, "/proc", "proc", 0, NULL)) {
		fprintf(stderr, "attempt to mount proc failed!\n");
		return -1;
	}
    fprintf(stderr, "successfully setup child mounts.\n");
    return 0;
}

int setup_cgroup_controls(struct child_config *config, struct cgroups_control **cgrps)
{
    fprintf(stderr, "####### > setting up SRContainer cgroups...");
    for (struct cgroups_control **cgrp = cgrps; *cgrp; cgrp++)
    {
        char dir[PATH_MAX] = {0};
        fprintf(stderr, "%s...", (*cgrp)->control);
        if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s",
                     (*cgrp)->control, config->hostname) == -1)
        {
            return -1;
        }
        if (mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR))
        {
            fprintf(stderr, "directory creation with mkdir() %s failed: %m\n", dir);
            return -1;
        }
        for (struct cgroup_setting **setting = (*cgrp)->settings; *setting; setting++)
        {
            char path[PATH_MAX] = {0};
            int fd = 0;
            if (snprintf(path, sizeof(path), "%s/%s", dir,
                         (*setting)->name) == -1)
            {
                fprintf(stderr, "invocation to snprintf() failed: %m\n");
                return -1;
            }
            if ((fd = open(path, O_WRONLY)) == -1)
            {
                fprintf(stderr, "invocation to open() %s failed: %m\n", path);
                return -1;
            }
            if (write(fd, (*setting)->value, strlen((*setting)->value)) == -1)
            {
                fprintf(stderr, "write() attempt %s failed: %m\n", path);
                close(fd);
                return -1;
            }
            close(fd);
        }
    }

    fprintf(stderr, "####### > setting up rlimit...");
    if (setrlimit(RLIMIT_NOFILE,
                  &(struct rlimit){
                      .rlim_max = FD_COUNT,
                      .rlim_cur = FD_COUNT,
                  }))
    {
        fprintf(stderr, "setrlimit() failed: %m\n");
        return 1;
    }
    fprintf(stderr, "successfully set up cgroup controls.\n");
    return 0;
}

int free_cgroup_controls(struct child_config *config, struct cgroups_control **cgrps)
{
    fprintf(stderr, "####### > cleaning cgroups...");
    for (struct cgroups_control **cgrp = cgrps; *cgrp; cgrp++)
    {
        char dir[PATH_MAX] = {0};
        char task[PATH_MAX] = {0};
        int task_fd = 0;
        if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s",
                     (*cgrp)->control, config->hostname) == -1 ||
            snprintf(task, sizeof(task), "/sys/fs/cgroup/%s/tasks",
                     (*cgrp)->control) == -1)
        {
            fprintf(stderr, "invocation to snprintf() failed: %m\n");
            return -1;
        }
        if ((task_fd = open(task, O_WRONLY)) == -1)
        {
            fprintf(stderr, "invocation to open() %s failed: %m\n", task);
            return -1;
        }
        if (write(task_fd, "0", 2) == -1)
        {
            fprintf(stderr, "write() attempt %s failed: %m\n", task);
            close(task_fd);
            return -1;
        }
        close(task_fd);
        if (rmdir(dir))
        {
            fprintf(stderr, "invocation to rmdir() %s failed: %m", dir);
            return -1;
        }
    }
    fprintf(stderr, "cgroup resources free'ed successfully.\n");
    return 0;
}

int setup_child_uid_map(pid_t child_pid, int fd)
{
    int uid_map = 0;
    int has_userns = -1;
    if (read(fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns))
    {
        fprintf(stderr, "read() attempt from child failed!\n");
        return -1;
    }
    if (has_userns)
    {
        char path[PATH_MAX] = {0};
        for (char **file = (char *[]){"uid_map", "gid_map", 0}; *file; file++)
        {
            if (snprintf(path, sizeof(path), "/proc/%d/%s", child_pid, *file) > sizeof(path))
            {
                fprintf(stderr, "invocation to snprintf() failed. %m\n");
                return -1;
            }
            fprintf(stderr, "setting UID/GID mapping %s...", path);
            if ((uid_map = open(path, O_WRONLY)) == -1)
            {
                fprintf(stderr, "invocation to open() failed: %m\n");
                return -1;
            }
            if (dprintf(uid_map, "0 %d %d\n", USERNS_OFFSET, USERNS_COUNT) == -1)
            {
                fprintf(stderr, "invocation to dprintf() failed: %m\n");
                close(uid_map);
                return -1;
            }
            close(uid_map);
        }
    }
    if (write(fd, &(int){0}, sizeof(int)) != sizeof(int))
    {
        fprintf(stderr, "write() failed inside child: %m\n");
        return -1;
    }
    return 0;
}

int setup_child_userns(struct child_config *config)
{
    fprintf(stderr, "####### > attempting a new user namespace...");
    int has_userns = !unshare(CLONE_NEWUSER);
    if (write(config->fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns))
    {
        fprintf(stderr, "write() inside child failed: %m\n");
        return -1;
    }
    int result = 0;
    if (read(config->fd, &result, sizeof(result)) != sizeof(result))
    {
        fprintf(stderr, "read() attempt inside child failed: %m\n");
        return -1;
    }
    if (result)
        return -1;
    if (has_userns)
    {
        fprintf(stderr, "all done successfully.\n");
    }
    else
    {
        fprintf(stderr, "some error occurred?? progressing anyways...\n");
    }
    fprintf(stderr, "####### > switching to UID %d / GID %d...", config->uid, config->uid);
    if (setgroups(1, &(gid_t){config->uid}) ||
        setresgid(config->uid, config->uid, config->uid) ||
        setresuid(config->uid, config->uid, config->uid))
    {
        fprintf(stderr, "%m\n");
        return -1;
    }
    fprintf(stderr, "setting up usernamespace done.\n");
    return 0;
}