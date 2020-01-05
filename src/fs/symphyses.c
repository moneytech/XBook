/*
 * file:		fs/interface.c
 * auther:		Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/vmarea.h>

#include <drivers/ide.h>
#include <drivers/ramdisk.h>

#include <book/blk-dev.h>
#include <fs/bofs/bofs.h>
#include <fs/bofs/drive.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/file.h>
#include <fs/bofs/device.h>
#include <fs/bofs/task.h>
#include <fs/bofs/pipe.h>
#include <fs/bofs/fifo.h>

#include <share/file.h>

#include <fs/fs.h>
/* 同步磁盘上的数据到文件系统 */
#define SYNC_DISK_DATA

/* 数据是在软盘上的 */
#define DATA_ON_FLOPPY
#define FLOPPY_DATA_ADDR	0x80050000

/* 数据是在软盘上的 */
#define DATA_ON_IDE

/* 要写入文件系统的文件 */
#define FILE_ID 3

#if FILE_ID == 1
	#define FILE_NAME "root:/init"
	#define FILE_SECTORS 40
#elif FILE_ID == 2
	#define FILE_NAME "root:/shell"
	#define FILE_SECTORS 100
#elif FILE_ID == 3
	#define FILE_NAME "root:/test"
	#define FILE_SECTORS 100
#elif FILE_ID == 4
	#define FILE_NAME "root:/test2"
	#define FILE_SECTORS 50
#endif

#define FILE_SIZE (FILE_SECTORS * SECTOR_SIZE)

PRIVATE void WriteDataToFS()
{
	#ifdef SYNC_DISK_DATA
	char *buf = kmalloc(FILE_SECTORS * SECTOR_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		Panic("kmalloc for buf failed!\n");
	}
	
	/* 把文件加载到文件系统中来 */
	int fd = SysOpen(FILE_NAME, O_CREAT | O_RDWR | O_EXEC);
    if (fd < 0) {
        printk("file open failed!\n");
		return;
    }
	
	memset(buf, 0, FILE_SECTORS * SECTOR_SIZE);
	
	/* 根据数据存在的不同形式选择不同的加载方式 */
	#ifdef DATA_ON_FLOPPY
    char *data = (char *)FLOPPY_DATA_ADDR;

    memcpy(buf, data, FILE_SIZE);

    #endif

	
	#ifdef DATA_ON_IDE

	#endif
	
	if (SysWrite(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("write failed!\n");
	}
    
	SysLseek(fd, 0, SEEK_SET);
	if (SysRead(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("read failed!\n");
	}

	kfree(buf);
	SysClose(fd);
	printk("load file sucess!\n");
    /* 主动同步到磁盘 */
    BlockSync();

    //Panic("test");
	#endif
}

/**
 * MakeDeviceFile - 生成设备文件
 * 
 */
PRIVATE void MakeDeviceFile()
{
    /* 创建设备目录 */
    if (SysMakeDir(SYS_PATH_DEV)) {
        printk("make tmp dir failed!\n");
        return;
    }
    SysSync();
    /* 扫描设备 */
    struct Device *device;
    char devType;
    char devpath[64];

    /* 设备扇区数 */
    size_t devSectors;

    ListForEachOwner(device, &allDeviceListHead, list) {
        //printk("device:%s type:%d devno:%x\n", device->name, device->type, device->devno);
        devSectors = 0;
        memset(devpath, 0, 64);

        strcat(devpath, SYS_PATH_DEV);
        strcat(devpath, "/");
        strcat(devpath, device->name);

        /* 需要打开设备，才可以操作 */
        DeviceOpen(device->devno, 0);
        
        switch (device->type)
        {
        case DEV_TYPE_CHAR:
            devType = D_CHAR;
            break;
        case DEV_TYPE_BLOCK:
            devType = D_BLOCK;
            
            if (device->name[0] == 'h' && device->name[1] == 'd') {
                /* hdx - ide磁盘*/

                if (DeviceIoctl(device->devno, IDE_IO_SECTORS, (int)&devSectors)) {
                    printk("ide io sectors failed!\n");
                }
               
            } else if (device->name[0] == 'r' && device->name[1] == 'd') {
                /* rdx - ramdisk磁盘*/

                if (DeviceIoctl(device->devno, RAMDISK_IO_SECTORS, (int)&devSectors)) {
                    printk("ramdisk io sectors failed!\n");
                }
            }
            //printk("sectors %d block size %d dev size %d\n", devSectors, blockSize, devSize);
            break;
        default:
            devType = D_UNKNOWN;
            break;
        }

        if (SysMakeDev(devpath, devType, MAJOR(device->devno), MINOR(device->devno), devSectors)) {
            printk("make dev failed!\n");
        }
        /* 使用完后关闭设备 */
        DeviceClose(device->devno);
    }

    /* 强制同步，使得内存中的内容可以进入磁盘 */
    SysSync();

    //SysListDir("tmp:/dev");

    /* 打开文件 */
    /*int fd = SysOpen("tmp:/dev/console0", O_RDWR);
    if (fd < 0)
        printk("open failed!\n");
    printk("open fd %d!\n", fd);
    
    char buf[32];
    strcpy(buf, "hello, I am come from China.\n");

    SysWrite(fd, buf, strlen(buf));

    fd = SysOpen("tmp:/dev/console1", O_RDWR);
    if (fd < 0)
        printk("open failed!\n");
    printk("open fd %d!\n", fd);
     
    SysWrite(fd, buf, strlen(buf));

    fd = SysOpen("tmp:/dev/hda", O_RDWR);
    if (fd < 0)
        printk("open failed!\n");
    
    int blksize = 0;

    if (SysIoctl(fd, IDE_IO_BLKZE, (int)&blksize)) {
        printk("fd %d do ioctl failed!\n", fd);
    }
    printk("blksz %d\n", blksize);

    char *sbuf = kmalloc(blksize, GFP_KERNEL);
    if (sbuf == NULL) {
        Panic("kmalloc for sbuf failed!\n");
    }
    SysLseek(fd, 2, SEEK_SET);
    if (SysRead(fd, sbuf, 1)) {
        printk("read sbuf failed!\n");
    }
    
    printk("%x %x %x %x\n", sbuf[0], sbuf[128], sbuf[256], sbuf[511]);
    memset(sbuf, 0xff, 512);
    
    SysLseek(fd, -2, SEEK_END);
    if (SysWrite(fd, sbuf, 1)) {
        printk("read sbuf failed!\n");
    }
    memset(sbuf, 0, 512);
    
    if (SysRead(fd, sbuf, 1)) {
        printk("read sbuf failed!\n");
    }
    printk("%x %x %x %x\n", sbuf[0], sbuf[128], sbuf[256], sbuf[511]);
    
    SysClose(fd);*/

    //Spin("test");
}


EXTERN struct List taskGlobalList;

/**
 * MakeTaskFile - 生成任务文件
 * 
 */
PRIVATE void MakeTaskFile()
{
    /* 创建设备目录 */
    if (SysMakeDir(SYS_PATH_TASK)) {
        printk("make tsk dir failed!\n");
        return;
    }
    SysSync();

    /* 扫描任务 */
    struct Task *task;

    ListForEachOwner (task, &taskGlobalList, globalList) {
        //printk("task name:%s pid:%d status:%d\n", task->name, task->pid, task->status);
        AddTaskToFS(task->name, task->pid);
    }

    //SysListDir("tmp:/tsk");

    //Spin("test");
}

/* 磁盘符链表头 */
EXTERN struct List driveListHead;

/**
 * MakeDriveFile - 生成磁盘符文件
 * 磁盘符文件就是普通文件，只是用来表明在系统中有哪些磁盘符。
 */
PRIVATE void MakeDriveFile()
{
    /* 创建设备目录 */
    if (SysMakeDir(SYS_PATH_DRIVE)) {
        printk("make drv dir failed!\n");
        return;
    }
    SysSync();

    //printk("list drives\n");
    
    char drvpath[64];

    int fd;
    struct BOFS_Drive *drive;
    ListForEachOwner(drive, &driveListHead, list) {
        //printk("drive: %s\n", drive->name); 
        memset(drvpath, 0, 64);
        strcat(drvpath, SYS_PATH_DRIVE);
        strcat(drvpath, "/");
        strcat(drvpath, drive->name);

        fd = SysOpen(drvpath, O_CREAT | O_RDONLY);
        if (fd < 0) {
            //printk("create file %s failed!\n", drive->name);
        } else {
            SysClose(fd);
        }
    }

    //SysListDir("tmp:/drv");

    //Spin("test");
}

/**
 * MakeFifoFile - 生成管道文件
 */
PRIVATE void MakeFifoFile()
{
    /* 创建设备目录 */
    if (SysMakeDir(SYS_PATH_PIPE)) {
        printk("make pipe dir failed!\n");
        return;
    }
    SysSync();
    /*
    if (SysMakeFifo("sys:/pip/fifo.pip", O_RDWR)) {
        printk("make fifo file failed!\n");
    }
    SysSync();
    
    int fd = SysOpen("sys:/pip/fifo.pip", O_RDONLY);
    if (fd < 0) {
        printk("open pipe file failed!\n");
    }

    char buf[32];
    SysRead(fd, buf, 32);
    //SysWrite(fd, buf, 32);

    close(fd);*/
}

/**
 * ConfigFiles - 配置文件
 * 
 */
PRIVATE void ConfigFiles()
{
    /* 初始化表 */
    
    /* 创建配置目录 */
    //SysMakeDir("root:/config");

    /* 初始化配置 */
    /*int fd = SysOpen("root:/config/init.cfg", O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("open root:/config/init.cfg failed!\n");
    }
    
    char cfg[32];
    memset(cfg, 0, 32);
    strcpy(cfg, "init");
    SysWrite();
    */

}


PRIVATE void Test();

/**
 * InitFileSystem - 初始化文件系统
 * 
 */
PUBLIC void InitFileSystem()
{
	PART_START("File System");

    InitBoFS();
    
    Test();

    WriteDataToFS();

    MakeDeviceFile();
    MakeTaskFile();
    MakeDriveFile();
    MakeFifoFile();

    ConfigFiles();
/*
    int fd[2];
    if (SysPipe(fd) < 0) {
        printk("pipe failed!\n");
    }

    printk("fd0 %d, fd1 %d\n", fd[0], fd[1]);

    char buf[20] = {0};
    strcpy(buf, "hello, world!\n");
    SysWrite(fd[1], buf, strlen(buf));

    memset(buf, 0, 20);
    SysRead(fd[0], buf, 20);
    printk(buf);

    SysClose(fd[0]);
    SysClose(fd[1]);*/
    //Spin("InitFileSystem");
	PART_END();
}

PUBLIC void SysListDir(const char *pathname)
{
    DIR *dir = SysOpenDir(pathname);
    if (dir == NULL) {
        Panic("open dir failed!");
    }
    SysRewindDir(dir);

    dirent de;
    do {
        if (SysReadDir(dir, &de))
            break;
        printk("name:%s inode:%d type:%x\n", de.name, de.inode, de.type);
    } while (1);
    SysCloseDir(dir);
}

PRIVATE void Test()
{
    /*SysMakeDir("root:/test");
    SysMakeDir("root:/test/mnt");
    */
    /*
    SysRemoveDir("root:/test");
    SysRemoveDir("root:/");
    */
    /*SysMakeDir("test");

    DIR *dir = SysOpenDir("test");
    if (dir == NULL) {
        Panic("open dir failed!");
    }
    printk("open\n");

    SysRewindDir(dir);

    DIRENT de;
    do {
        if (SysReadDir(dir, &de))
            break;

        printk("name:%s inode:%d type:%d\n", de.name, de.inode, de.type);
    } while (1);

    SysCloseDir(dir);

    SysChangeName("test", "test111");*/
    
    /*
    int fd = SysOpen("root:/bin3", O_CREAT | O_RDWR);
    if (fd == -1) {
        printk("file open failed!\n");
    }

    struct stat buf;
    if(!SysStat("root:/bin", &buf))
        printk("size:%d mode:%x devno:%x inode:%d\n", 
        buf.st_size, buf.st_mode, buf.st_dev, buf.st_ino);
    if(!SysStat("root:/", &buf))
        printk("size:%d mode:%x devno:%x inode:%d\n", 
        buf.st_size, buf.st_mode, buf.st_dev, buf.st_ino);

    SysClose(fd);

    //SysUnlink("root:/bin");
    if(!SysStat("root:/bin", &buf))
        printk("size:%d mode:%x devno:%x inode:%d\n", 
        buf.st_size, buf.st_mode, buf.st_dev, buf.st_ino);

    if(SysAccess("root:/bin", F_OK)) {
        printk("file not exist!\n");
    }

    if(SysAccess("root:/bin", R_OK)) {
        printk("file not readable!\n");
    }

    if(SysAccess("root:/bin", W_OK)) {
        printk("file not writeable!\n");
    }

    printk("access exe!\n");
    if(SysAccess("root:/bin", X_OK)) {
        printk("file not executable!\n");
    }

    mode_t mode = SysGetMode("root:/bin3");
    printk("mode:%x\n", mode);

    mode |= S_IEXEC;

    SysChangeMode("root:/bin3", mode);

    mode = SysGetMode("root:/bin3");
    printk("mode:%x\n", mode);
    */

    /*char name[MAX_PATH_LEN];
    memset(name, 0, MAX_PATH_LEN);
    MakeClearAbsPath("/test/", name);
    printk(name);

    MakeClearAbsPath("test/a/./b", name);
    printk(name);

    MakeClearAbsPath("../test/a/../b", name);
    printk(name);*/

    /*if (SysMakeDir("root:/qq")) 
        printk("mkdir failde!\n");
    if (SysMakeDir("tt"))
        printk("mkdir failde!\n");
    
    if (SysRemoveDir("root:/qq"))
        printk("rmdir failde!\n");
    
    if (SysRemoveDir("tt"))
        printk("rmdir failde!\n");*/
    
/*
    char path[MAX_PATH_LEN] = {0};
    SysGetCWD(path, MAX_PATH_LEN);
    printk("cwd:%s\n", path);

    SysChangeCWD("usr");
    SysChangeCWD("/a/../abc");
    SysChangeCWD("123.txt");
    
    //SysChangeCWD("../");

    SysChangeCWD("/");

    SysChangeCWD("root:/usr");
    */
    /*
    磁盘符:路径

    绝对路径：磁盘符：路径
    相对路径：路径 -> /abc， abc， ./abc

    绝对路径：root:/abc/def/a
    相对路径：a，工作目录：root:/abc/def

    临时路径：root:/abc/def/../a/./c
    
    分离：root，/abc/def/../a/./c

    清洗路径：/abc/def/../a/./c

    文件系统识别的路径：/abc/a/c
    */
}

PUBLIC int SysPipe(int fd[2])
{
    return BOFS_Pipe(fd);
}

PUBLIC int SysMakeDev(const char *pathname, char type,
        int major, int minor, size_t size)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    char newType;

    switch (type) {
    case D_BLOCK:
        newType = DEV_TYPE_BLOCK;
        break;
    case D_CHAR:
        newType = DEV_TYPE_CHAR;
        break;
    default:
        newType = DEV_TYPE_UNKNOWN;
        break;
    }

    return BOFS_MakeDevice(finalPath, newType, MKDEV(major, minor), size, drive->sb);
}

PUBLIC int AddTaskToFS(char *name, pid_t pid)
{
    char taskpath[MAX_TASK_NAMELEN + 24];
    memset(taskpath, 0, MAX_TASK_NAMELEN + 24);
    strcat(taskpath, SYS_PATH_TASK);
    strcat(taskpath, "/");
    strcat(taskpath, name);
    strcat(taskpath, "\\");

    char numstr[8];
    memset(numstr, 0, 8);
    char *p = &numstr[0];
    itoa(&p, pid, 10);
    strcat(taskpath, numstr);
    //printk("task path %s\n", taskpath);
    if (SysMakeTsk(taskpath, pid)) {
        //printk("make task file at %s failed!\n", taskpath);
        return -1;
    }

    return 0;
}

PUBLIC int DelTaskFromFS(char *name, pid_t pid)
{
    char taskpath[MAX_TASK_NAMELEN + 24];
    memset(taskpath, 0, MAX_TASK_NAMELEN + 24);
    strcat(taskpath, SYS_PATH_TASK);
    strcat(taskpath, "/");
    strcat(taskpath, name);
    strcat(taskpath, "\\");

    char numstr[8];
    memset(numstr, 0, 8);
    char *p = &numstr[0];
    itoa(&p, pid, 10);
    strcat(taskpath, numstr);
    //printk("task path %s\n", taskpath);
    if (SysRemove(taskpath)) {
        //printk("remove task file at %s failed!\n", taskpath);
        return -1;
    }
    return 0;
}

PUBLIC int SysMakeTsk(const char *pathname, pid_t pid)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    return BOFS_MakeTask(finalPath, pid, drive->sb);
}

PUBLIC int SysMakeFifo(const char *pathname, mode_t mode)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    mode_t newMode = 0;

    if (mode & M_IREAD) {
        newMode |= BOFS_IMODE_R;
    }
    if (mode & M_IWRITE) {
        newMode |= BOFS_IMODE_W;
    }

    return BOFS_MakeFifo(finalPath, newMode, drive->sb);
}

PUBLIC int SysGetCWD(char* buf, unsigned int size)
{
    return BOFS_GetCWD(buf, size);
}

PUBLIC int SysChangeCWD(const char *pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    //printk("begin:%s process:%s finall:%s\n", pathname,  absPath, finalPath);

    /* 调用文件系统的改变工作目录 */
    return BOFS_ChangeCWD(finalPath, drive);
}

PUBLIC int SysOpen(const char *pathname, unsigned int flags)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    /* 根据文件系统接口的标志来转换成本地接口的标志 */
    
    unsigned int newFlags = 0;

    if (flags & O_RDONLY)
        newFlags |= BOFS_O_RDONLY;
    
    if (flags & O_WRONLY)
        newFlags |= BOFS_O_WRONLY;
    
    if (flags & O_RDWR)
        newFlags |= BOFS_O_RDWR;
    
    if (flags & O_CREAT)
        newFlags |= BOFS_O_CREAT;
    
    if (flags & O_TRUNC)
        newFlags |= BOFS_O_TRUNC;
    
    if (flags & O_APPEDN)
        newFlags |= BOFS_O_APPEDN;
    
    if (flags & O_EXEC)
        newFlags |= BOFS_O_EXEC;

    return BOFS_Open(finalPath, newFlags, drive->sb);
}

PUBLIC int SysLseek(int fd, unsigned int offset, char flags)
{
    /* 根据文件系统接口的标志来转换成本地接口的标志 */
    
    unsigned int newFlags = 0;
    
    if (flags == SEEK_SET)
        newFlags = BOFS_SEEK_SET;
    
    if (flags == SEEK_CUR)
        newFlags = BOFS_SEEK_CUR;
    
    if (flags == SEEK_END)
        newFlags = BOFS_SEEK_END;
    
    return BOFS_Lseek(fd, offset, newFlags);
}

PUBLIC int SysRead(int fd, void *buffer, unsigned int size)
{
    return BOFS_Read(fd, buffer, size);
}

PUBLIC int SysWrite(int fd, void *buffer, unsigned int size)
{
    return BOFS_Write(fd, buffer, size);
}

PUBLIC int SysClose(int fd)
{
	return BOFS_Close(fd);
}

PUBLIC int SysStat(const char *pathname, struct stat *buf)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    struct BOFS_Stat bstat;

    /* 获取数据 */
    if (BOFS_Stat(finalPath, &bstat, drive->sb)) {
        return -1;
    }

    mode_t newMode = 0;

    if (bstat.mode & BOFS_IMODE_R) {
        newMode |= S_IREAD;
    }
    if (bstat.mode & BOFS_IMODE_W) {
        newMode |= S_IWRITE;
    }
    if (bstat.mode & BOFS_IMODE_X) {
        newMode |= S_IEXEC;
    }
    if (bstat.mode & BOFS_IMODE_F) {
        newMode |= S_IFREG;
    }
    if (bstat.mode & BOFS_IMODE_D) {
        newMode |= S_IFDIR;
    }

    /* 转换数据 */
    buf->st_size = bstat.size;
    buf->st_mode = newMode;
    buf->st_ino = bstat.inode;
    buf->st_dev = bstat.devno;
    buf->st_rdev = bstat.devno2;
    buf->st_ctime = bstat.crttime;
    buf->st_mtime = bstat.mdftime;
    buf->st_atime = bstat.acstime;
    buf->st_blksize = bstat.blksize;
    buf->st_blocks = bstat.blocks;
    return 0;
}

PUBLIC int SysRemove(const char *pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    //printk("remove path %s\n", finalPath);
    return BOFS_Remove(finalPath, drive->sb);
}

PUBLIC int SysIoctl(int fd, int cmd, int arg)
{
    return BOFS_Ioctl(fd, cmd, arg);
}

PUBLIC int SysFcntl(int fd, int cmd, int arg)
{
    
	return 0;
}

PUBLIC int SysFsync(int fd)
{
    return BOFS_Fsync(fd);
}

PUBLIC int SysSync()
{
    return BlockSync();
}


PUBLIC int SysAccess(const char *pathname, mode_t mode)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    mode_t newMode = 0;
    
    switch (mode) {
    case F_OK:
        newMode = BOFS_F_OK;
        break;
    case X_OK:
        newMode = BOFS_X_OK;
        break;
    case W_OK:
        newMode = BOFS_W_OK;
        break;    
    case R_OK:
        newMode = BOFS_R_OK;
        break;    
    }
    
    return BOFS_Access(finalPath, newMode, drive->sb);
}

PUBLIC int SysGetMode(const char* pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
	mode_t mode = BOFS_GetMode(finalPath, drive->sb);
    //printk("get the mode:%x\n", mode);

    mode_t newMode = 0;

    if (mode & BOFS_IMODE_R) {
        newMode |= S_IREAD;
    }
    if (mode & BOFS_IMODE_W) {
        newMode |= S_IWRITE;
    }
    if (mode & BOFS_IMODE_X) {
        newMode |= S_IEXEC;
    }
    if (mode & BOFS_IMODE_F) {
        newMode |= S_IFREG;
    }
    if (mode & BOFS_IMODE_D) {
        newMode |= S_IFDIR;
    }
    //printk("get the new mode:%x\n", newMode);
    
    return newMode;
}

PUBLIC int SysChangeMode(const char* pathname, int mode)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    mode_t newMode = 0;

    if (mode & S_IREAD) {
        newMode |= BOFS_IMODE_R;
    }
    if (mode & S_IWRITE) {
        newMode |= BOFS_IMODE_W;
    }
    if (mode & S_IEXEC) {
        newMode |= BOFS_IMODE_X;
    }

    if (mode & S_IFREG) {
        newMode |= BOFS_IMODE_F;
    }

    if (mode & S_IFDIR) {
        newMode |= BOFS_IMODE_D;
    }

	return BOFS_ChangeMode(finalPath, newMode, drive->sb);
}

PUBLIC int SysMakeDir(const char *pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    return BOFS_MakeDir(finalPath, drive->sb);
}

PUBLIC int SysRemoveDir(const char *pathname)
{
	char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    return BOFS_RemoveDir(finalPath, drive->sb);
}

PUBLIC int SysChangeName(const char *pathname, char *name)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    return BOFS_ResetName(finalPath, name, drive->sb);
}

PUBLIC DIR *SysOpenDir(const char *pathname)
{
	char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return NULL;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    //printk("finall path:%s\n", finalPath);
    struct BOFS_Dir *dir = BOFS_OpenDir(finalPath, drive->sb);
    if (dir == NULL) {
        return NULL;
    } else {
        return (DIR *)dir;
    }
}

PUBLIC void SysCloseDir(DIR *dir)
{
    BOFS_CloseDir((struct BOFS_Dir *)dir);
}

PUBLIC int SysReadDir(DIR *dir, dirent *buf)
{
    struct BOFS_Dir *bdir = (struct BOFS_Dir *)dir;
    off_t off = bdir->pos;
    
    struct BOFS_DirEntry *de = BOFS_ReadDir(bdir);

    /* 读取结束 */
    if (de == NULL)
        return -1;
    
    /* 填写数据 */
    memset(buf->name, 0, NAME_MAX + 1);
    strcpy(buf->name, de->name);
    buf->type = de->type;
    buf->inode = de->inode;
    buf->off = off;

	return 0;
}

PUBLIC void SysRewindDir(DIR *dir)
{
	BOFS_RewindDir((struct BOFS_Dir *)dir);
}