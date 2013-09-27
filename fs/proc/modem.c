#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <../arch/arm/mach-msm/smd_private.h>


static char modem_version[2][16];
// note : this declaration should be identical to the one in lk/app/aboot/jrd_version.h
#define QCSBL_VERSION_MAX_LEN 16

static struct {
  char modem_hd[QCSBL_VERSION_MAX_LEN];
  char modem_mc[QCSBL_VERSION_MAX_LEN];
  char secro[QCSBL_VERSION_MAX_LEN];
  char traca[QCSBL_VERSION_MAX_LEN];
  char study[QCSBL_VERSION_MAX_LEN];
  char tuning[QCSBL_VERSION_MAX_LEN];
} smem_version;

typedef enum {
  MODEM =0x01,
  SECRO =0x02,
  TRACA =0x04,
  STUDY =0x08,
  TUNING=0x10,
  ALL   =0x1F,
  VER_TYPE_END
}ver_type_t;

static unsigned smem_version_init = 0;

static int proc_show(struct seq_file *m, void *v,ver_type_t type)
{
  uint8_t *data=NULL;
  unsigned size;
  if ( !smem_version_init ) {
	 data = smem_get_entry(SMEM_ID_VENDOR0, &size);
	 if (!data) {
    printk( KERN_ERR "can't get SMEM_ID_VENDOR0");  
		return 1;
   }

	if (size != sizeof(smem_version)) {
    if (size == sizeof(modem_version) && type == MODEM) { // backward compat
      memcpy(modem_version, data, sizeof(modem_version));
	    seq_printf(m, "%s:%s\n", modem_version[0],modem_version[1]);
      return 0;
    }
    
    printk( KERN_ERR "size of SMEM_ID_VENDOR0 is wrong %d not %d", size, 
            sizeof(smem_version) );  
		return 1;
  }
	
  memcpy(&smem_version, data, sizeof(smem_version));
  }
  
  if(smem_version.modem_mc[0] && (type & MODEM))
	seq_printf(m, "%s:%s\n", smem_version.modem_hd,smem_version.modem_mc);
  
  if(smem_version.traca[0] && (type & TRACA))
	  seq_printf(m, "%s\n", smem_version.traca);
  if(smem_version.secro[0] && (type & SECRO))
	  seq_printf(m, "%s\n", smem_version.secro);
  if(smem_version.study[0] && (type & STUDY))
	  seq_printf(m, "%s\n", smem_version.study);
  if(smem_version.tuning[0] && (type & TUNING))
	  seq_printf(m, "%s\n", smem_version.tuning);

	return 0;
}


static int modem_proc_show(struct seq_file *m, void *v) {
  return proc_show(m, v,MODEM);
}

static int secro_proc_show(struct seq_file *m, void *v) {
  return proc_show(m, v,SECRO);
}
static int study_proc_show(struct seq_file *m, void *v) {
  return proc_show(m, v,STUDY);
}
static int tuning_proc_show(struct seq_file *m, void *v) {
  return proc_show(m, v,TUNING);
}

static int proc_open(struct inode *inode, struct file *file)
{
  //printk( KERN_DEBUG "the file path is : %s", file->f_dentry->d_iname );  
  if (strstr( file->f_dentry->d_iname, "modem")) {
	  return single_open(file, modem_proc_show, NULL);
  }else if (strstr( file->f_dentry->d_iname, "secro")) {
	  return single_open(file, secro_proc_show, NULL);
  }else if (strstr( file->f_dentry->d_iname, "study")) {
	  return single_open(file, study_proc_show, NULL);
  }else if (strstr( file->f_dentry->d_iname, "tuning")) {
	  return single_open(file, tuning_proc_show, NULL);
  }
  printk( KERN_ERR "don't know this proc/ file path %s", file->f_dentry->d_iname );  
  return 1;
}


static const struct file_operations all_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static const struct file_operations modem_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations secro_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static const struct file_operations study_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static const struct file_operations tuning_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_modem_init(void)
{
	proc_create("modem", 0, NULL, &modem_proc_fops);
	proc_create("secro", 0, NULL, &secro_proc_fops);
	//proc_create("traca", 0, NULL, &modem_proc_fops);
	proc_create("study", 0, NULL, &study_proc_fops);
	proc_create("tuning", 0, NULL, &tuning_proc_fops);
  memset(&smem_version,0,sizeof(smem_version));
  smem_version_init=0;
	return 0;
}
module_init(proc_modem_init);
